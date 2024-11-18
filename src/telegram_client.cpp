#include <tgbot/tgbot.h>

#include <coroutine>
#include <cstdlib>

#include "loadenv.h"
#include "newConnect.pb.h"
#include "pch.h"
#include "sockets.h"
#include "telegram_client_coroutine.hpp"

using ChatIdType = std::int64_t;
using namespace boost::asio;

ClientCoroutine clientEntry(ChatIdType chatId, std::queue<std::string>& messageQueue, TgBot::Bot& bot) {
    // Connects to the server as a new client
    // Create a socket connected to the server at localhost:1234
    io_service io_service;
    ip::tcp::socket socket(io_service);
    socket.connect(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 1234));

    // // Read welcome message from server
    std::string welcome = read_(socket);
    std::cout << welcome << std::endl;

    std::string name = "Telegram User " + std::to_string(chatId);

    NewConnection newConnection;
    newConnection.set_name(name);
    newConnection.set_operation(ClientOperation::OP_CREATE_SESSION);
    newConnection.set_session_id(0);

    // Send the NewConnection message to the server
    std::string serialized;
    newConnection.SerializeToString(&serialized);
    send_(socket, serialized);

    printf("Sent NewConnection message to server\n");
    while (true) {
        // TgBot::Message::Ptr message = co_await Awaitable<TgBot::Message::Ptr>(messageQueue);
        std::string text = co_await Awaitable<std::string>(messageQueue);
        bot.getApi().sendMessage(chatId, "Your message is: " + text);
    }
    co_return;
}

class ChatSessions {
    TgBot::Bot& bot;
    std::unordered_map<ChatIdType, ClientCoroutine> chatCoroutines;
    std::unordered_map<ChatIdType, std::queue<std::string>> chatMessageQueues;

   public:
    ChatSessions(TgBot::Bot& bot) : bot(bot) {}

    bool hasSession(ChatIdType chatId) const {
        return chatCoroutines.find(chatId) != chatCoroutines.end();
    }

    void createNewSession(ChatIdType chatId, TgBot::Bot& bot) {
        chatCoroutines.emplace(chatId, ClientCoroutine(clientEntry(chatId, chatMessageQueues[chatId], bot)));
    }

    void passMessage(ChatIdType chatId, std::string message) {
        chatMessageQueues[chatId].push(message);
        chatCoroutines.at(chatId).handle.resume();
    }
};

int main() {
    loadEnv();
    TgBot::Bot bot(getenv("TELEGRAM_BOT_API_TOKEN"));
    ChatSessions chatSessions(bot);

    bot.getEvents().onAnyMessage([&bot, &chatSessions](TgBot::Message::Ptr message) {
        if (!chatSessions.hasSession(message->chat->id)) {
            BOOST_LOG_TRIVIAL(info) << "Creating new chat session for chat " << message->chat->id;
            chatSessions.createNewSession(message->chat->id, bot);
        }
        chatSessions.passMessage(message->chat->id, message->text);
    });

    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            // printf("Long poll started\n");
            longPoll.start();
        }
    } catch (TgBot::TgException& e) {
        printf("error: %s\n", e.what());
    }
    return 0;
}
