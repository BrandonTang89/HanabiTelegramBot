#include "client_input_tasks.h"

#include "client_entry_helpers.h"
#include "pch.h"
#include "proto_files.h"
#include "telegram_client_coroutine.hpp"
#include "telegram_client_pch.h"
#include "telegram_keyboard.h"
using ip::tcp;
using namespace boost::asio;
using namespace Ack;

Task<> welcomeTask(Client client) {
    auto& [chatId, _, __, ___, bot] = client;
    const std::string welcomeMessage =
        "Welcome to Hanabi!\n Enter /quit anytime to exit. \n Please enter your "
        "name:";
    bot.getApi().sendMessage(chatId, welcomeMessage);
    BOOST_LOG_TRIVIAL(debug) << "Sent welcome message to chat " << chatId;
    co_return;
}

Task<std::string> getNameTask(Client client) {
    auto& [chatId, _, msgQueue, __, bot] = client;
    TgMsg message = co_await msgQueue;
    while (message->text.empty() || message->text[0] == '/') {
        bot.getApi().sendMessage(chatId,"Name cannot be empty or begin with a /\n Please enter your name:");
        message = co_await msgQueue;
    }
    co_return std::string(message->text);
}

Task<ClientOperation> getOperationTask(Client client) {
    auto& [chatId, _, msgQueue, __, bot] = client;
    const std::vector<std::string> operationStrings = {"/createSession", "/joinSession", "/joinRandomSession"};
    const TgBot::ReplyKeyboardMarkup::Ptr operationKeyboard = createOneColumnKeyboard(operationStrings);
    bot.getApi().sendMessage(chatId,
                             "Please select an operation: \n 1. Create a new session \n 2. Join a "
                             "specific session \n 3. Join a random session",
                             nullptr, nullptr, operationKeyboard);
    TgMsg message = co_await msgQueue;
    while (true) {
        if (message->text == "/createSession") {
            co_return ClientOperation::OP_CREATE_SESSION;
        } else if (message->text == "/joinSession") {
            co_return ClientOperation::OP_JOIN_SPECIFIC_SESSION;
        } else if (message->text == "/joinRandomSession") {
            co_return ClientOperation::OP_JOIN_RANDOM_SESSION;
        } else {
            bot.getApi().sendMessage(chatId,
                                     "Invalid operation! Please select an operation: \n 1. Create a new "
                                     "session \n 2. Join a specific session \n 3. Join a random session",
                                     nullptr, nullptr, operationKeyboard);
            message = co_await msgQueue;
        }
    }
}

Task<int> getSpecificSessionTask(Client client) {
    auto& [chatId, _, msgQueue, __, bot] = client;
    bot.getApi().sendMessage(chatId, "Please enter the session ID:");
    TgMsg message = co_await msgQueue;
    while (message->text.empty() ||
           !std::ranges::all_of(message->text.begin(), message->text.end(), ::isdigit)) {
        bot.getApi().sendMessage(chatId, "Session ID should be a number! Please enter the session ID:");
        message = co_await msgQueue;
    }
    co_return std::stoi(message->text);
}

Task<> waitUntilStartCommand(Client client) {
    auto& [chatId, _, msgQueue, __, bot] = client;
    bot.getApi().sendMessage(chatId, "Please enter /start to start the game!");
    TgMsg message = co_await msgQueue;
    while (message->text != "/start") {
        bot.getApi().sendMessage(chatId, "Please enter /start to start the game!");
        message = co_await msgQueue;
    }
    co_return;
}

Task<> leaderTask(Client client, const std::optional<int> sessionId) {
    auto& [chatId, socket, msgQueue, _, bot] = client;
    // Wait until start then start the game
    while (true) {
        // Register to forward messages from the server regarding other players
        // joining to the user
        subscribeToInfo(client);

        BOOST_LOG_TRIVIAL(info) << "Waiting for start command...";
        co_await waitUntilStartCommand(client);

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
    subscribeToInfo(client);
}