#include "client_entry.h"
#include "loadenv.h"
#include "telegram_client_coroutine.hpp"
#include "telegram_client_pch.h"

class ChatSessions {
    TgBot::Bot& bot;
    std::unordered_map<ChatIdType, ClientCoroutine> chatCoroutines;
    std::unordered_map<ChatIdType, std::queue<std::string>> chatMessageQueues;

   public:
    ChatSessions(TgBot::Bot& bot) : bot(bot) {}

    bool hasSession(ChatIdType chatId) const {
        if (chatCoroutines.find(chatId) == chatCoroutines.end() ) return false;
        if (!chatCoroutines.at(chatId).handle) return false;
        if (chatCoroutines.at(chatId).handle.done()) return false;
        return true;
    }

    void createNewSession(ChatIdType chatId, TgBot::Bot& bot) {
        // new clients are handled via clientEntry coroutine
        chatCoroutines.insert_or_assign(chatId, ClientCoroutine(clientEntry(chatId, chatMessageQueues[chatId], bot)));
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

        // BOOST_LOG_TRIVIAL(trace) << "Passing message to chat session for chat " << message->chat->id;
        // BOOST_LOG_TRIVIAL(trace) << "Message: " << message->text;
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
