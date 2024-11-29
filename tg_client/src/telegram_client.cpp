#include "client_entry.h"
#include "loadenv.h"
#include "pch.h"
#include "telegram_client_coroutine.hpp"
#include "telegram_client_pch.h"

class ChatSessions {
   private:
    TgBot::Bot& bot;                  // telegram bot to send messages
    boost::asio::io_context& io_ctx;  // io_ctx for async operations
    std::unordered_map<ChatIdType, Task<void>> chatCoroutines;
    std::unordered_map<ChatIdType, MessageQueue<TgMsg>> chatMessageQueues;

   public:
    ChatSessions(TgBot::Bot& bot_, boost::asio::io_context& io_ctx_) : bot(bot_), io_ctx(io_ctx_) {}

    bool hasSession(const ChatIdType chatId) const {
        if (!chatCoroutines.contains(chatId)) return false;
        if (!chatCoroutines.at(chatId).handle_) return false;
        if (chatCoroutines.at(chatId).handle_.done()) return false;
        return true;
    }

    void createNewSession(const ChatIdType chatId) {
        // new clients are handled via clientEntry coroutine
        chatMessageQueues.insert_or_assign(chatId, MessageQueue<TgMsg>());
        chatCoroutines.insert_or_assign(chatId, clientEntry(chatId, chatMessageQueues.at(chatId), bot, io_ctx));
        chatCoroutines.at(chatId).handle_.resume();  // start the coroutine
    }

    void passMessage(const ChatIdType chatId, TgMsg message) {
        chatMessageQueues.at(chatId).pushMessage(std::move(message));
    }

    void deleteSession(const ChatIdType chatId) {
        chatCoroutines.erase(chatId);
        chatMessageQueues.erase(chatId);
    }
};

int main() {
    loadEnv();
    TgBot::Bot bot(getenv("TELEGRAM_BOT_API_TOKEN"));
    boost::asio::io_context io_ctx;
    ChatSessions chatSessions(bot, io_ctx);

    bot.getEvents().onAnyMessage([&bot, &chatSessions](const TgMsg& message) {
        if (!chatSessions.hasSession(message->chat->id)) {
            BOOST_LOG_TRIVIAL(info) << "Creating new chat session for chat " << message->chat->id;
            chatSessions.createNewSession(message->chat->id);
        }

        if (message->text == "/quit") {
            BOOST_LOG_TRIVIAL(info) << "Client of id " << message->chat->id << " quit!";
            bot.getApi().sendMessage(message->chat->id, "Goodbye!");
            chatSessions.deleteSession(message->chat->id);
            return;
        }

        // BOOST_LOG_TRIVIAL(trace) << "Passing message to chat session for chat " << message->chat->id;
        // BOOST_LOG_TRIVIAL(trace) << "Message: " << message->text;
        chatSessions.passMessage(message->chat->id, message);
    });

    // Run the io_ctx in a separate thread
    std::thread ioThread([&io_ctx]() {
        try {
            while (true) {
                io_ctx.poll();
                io_ctx.restart();
            }

        } catch (std::exception& e) {
            std::cerr << "Exception in io_ctx: " << e.what() << std::endl;
        }
    });
    ioThread.detach();

    // Main thread is in charge of handling telegram events
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
