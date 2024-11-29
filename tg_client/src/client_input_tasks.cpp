#include "pch.h"
#include "proto_files.h"
#include "telegram_client_coroutine.hpp"
#include "telegram_client_pch.h"
#include "telegram_keyboard.h"
#include "client_input_tasks.h"
#include "client_entry_helpers.h"
using ip::tcp;
using namespace boost::asio;
using namespace Ack;

Task<> welcomeTask(ChatIdType chatId, TgBot::Bot& bot) {
    std::string welcomeMessage = "Welcome to Hanabi!\n Enter /quit anytime to exit. \n Please enter your name:";
    bot.getApi().sendMessage(chatId, welcomeMessage);
    BOOST_LOG_TRIVIAL(debug) << "Sent welcome message to chat " << chatId;
    co_return;
}

Task<std::string> getNameTask(ChatIdType chatId, TgBot::Bot& bot, MessageQueue<TgMsg>& msgQueue) {
    TgMsg message = co_await msgQueue;
    while (message->text.empty() || message->text[0] == '/') {
        bot.getApi().sendMessage(chatId, "Name cannot be empty or begin with a /\n Please enter your name:");
        message = co_await msgQueue;
    }
    co_return std::string(message->text);
}

Task<ClientOperation> getOperationTask(ChatIdType chatId, TgBot::Bot& bot, MessageQueue<TgMsg>& msgQueue) {
    std::vector<std::string> operationStrings = {"/createSession", "/joinSession", "/joinRandomSession"};
    TgBot::ReplyKeyboardMarkup::Ptr operationKeyboard(new TgBot::ReplyKeyboardMarkup);
    createOneColumnKeyboard(operationStrings, operationKeyboard);
    bot.getApi().sendMessage(
        chatId,
        "Please select an operation: \n 1. Create a new session \n 2. Join a specific session \n 3. Join a random session",
        nullptr, 0, operationKeyboard);
    TgMsg message = co_await msgQueue;
    while (true) {
        if (message->text == "/createSession") {
            co_return ClientOperation::OP_CREATE_SESSION;
        } else if (message->text == "/joinSession") {
            co_return ClientOperation::OP_JOIN_SPECIFIC_SESSION;
        } else if (message->text == "/joinRandomSession") {
            co_return ClientOperation::OP_JOIN_RANDOM_SESSION;
        } else {
            bot.getApi().sendMessage(
                chatId,
                "Invalid operation! Please select an operation: \n 1. Create a new session \n 2. Join a specific session \n 3. Join a random session",
                nullptr, 0, operationKeyboard);
            message = co_await msgQueue;
        }
    }
}

Task<int> getSpecificSessionTask(ChatIdType chatId, TgBot::Bot& bot, MessageQueue<TgMsg>& msgQueue) {
    bot.getApi().sendMessage(chatId, "Please enter the session ID:");
    TgMsg message = co_await msgQueue;
    while (message->text.empty() || !std::all_of(message->text.begin(), message->text.end(), ::isdigit)) {
        bot.getApi().sendMessage(chatId, "Session ID should be a number! Please enter the session ID:");
        message = co_await msgQueue;
    }
    co_return std::stoi(message->text);
}

Task<> waitUntilStartCommand(ChatIdType chatId, TgBot::Bot& bot, MessageQueue<TgMsg>& msgQueue) {
    bot.getApi().sendMessage(chatId, "Please enter /start to start the game!");
    TgMsg message = co_await msgQueue;
    while (message->text != "/start") {
        bot.getApi().sendMessage(chatId, "Please enter /start to start the game!");
        message = co_await msgQueue;
    }
    co_return;
}

Task<> leaderTask(tcp::socket& socket, ChatIdType& chatId, TgBot::Bot& bot, MessageQueue<TgMsg>& msgQueue, std::optional<int> sessionId) {
    // Wait until start then start the game
    while (true) {
        // Register to forward messages from the server regarding other players joining to the user
        subscribeToInfo(socket, chatId, bot);

        BOOST_LOG_TRIVIAL(info) << "Waiting for start command...";
        co_await waitUntilStartCommand(chatId, bot, msgQueue);

        socket.cancel();  // cancel the async read
        StartGameMsg startGameMsg;
        sendBytes(socket, startGameMsg.SerializeAsString());
        AckMessage startGameAck;
        std::string serialisedStartGameAck = readBytes(socket);
        startGameAck.ParseFromString(serialisedStartGameAck);

        BOOST_LOG_TRIVIAL(info) << "Start Game Acknowledgement received!";
        if (startGameAck.status() == AckStatus::ACK_SUCCEED) {
            BOOST_LOG_TRIVIAL(info) << "Session: " << sessionId.value() << " has started..";
            break;
        } else {
            bot.getApi().sendMessage(chatId, "Failed to start game!");
            bot.getApi().sendMessage(chatId, startGameAck.message());
        }
    }
    subscribeToInfo(socket, chatId, bot);
}