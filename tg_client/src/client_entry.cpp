#include "client_entry.h"

#include "Ack.pb.h"
#include "CreateSessionAck.pb.h"
#include "JoinRandomSessionAck.pb.h"
#include "JoinSessionAck.pb.h"
#include "NewConnect.pb.h"
#include "StartGame.pb.h"
#include "pch.h"
#include "sockets.h"
#include "telegram_client_coroutine.hpp"
#include "telegram_client_pch.h"
#include "telegram_keyboard.h"
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
    bot.getApi().sendMessage(chatId, "Please select an operation: \n 1. Create a new session \n 2. Join a specific session \n 3. Join a random session", nullptr, 0, operationKeyboard);
    TgMsg message = co_await msgQueue;
    while (true) {
        if (message->text == "/createSession") {
            co_return ClientOperation::OP_CREATE_SESSION;
        } else if (message->text == "/joinSession") {
            co_return ClientOperation::OP_JOIN_SPECIFIC_SESSION;
        } else if (message->text == "/joinRandomSession") {
            co_return ClientOperation::OP_JOIN_RANDOM_SESSION;
        } else {
            bot.getApi().sendMessage(chatId, "Invalid operation! Please select an operation: \n 1. Create a new session \n 2. Join a specific session \n 3. Join a random session", nullptr, 0, operationKeyboard);
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

Task<> clientEntry(ChatIdType chatId, MessageQueue<TgMsg>& msgQueue, TgBot::Bot& bot) {
    // Connects to the server as a new client
    // Create a socket connected to the server at localhost:1234
    // Move down eventually when the project matures (now here for debug)

    io_service io_service;
    ip::tcp::socket socket(io_service);
    socket.connect(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 1234));
    BOOST_LOG_TRIVIAL(info) << "Client of id " << chatId << " connected to server";

    // Read welcome acknowledgement from server
    AckMessage ack;
    std::string serialisedAck = readBytes(socket);
    ack.ParseFromString(serialisedAck);
    if (ack.status() != AckStatus::ACK_SUCCEED) {
        BOOST_LOG_TRIVIAL(error) << "Failed to connect to server!";
        co_return;
    }

    co_await msgQueue;                  // remove the starting message if any
    co_await welcomeTask(chatId, bot);  // Display welcome message to user

    // Get Name
    std::string name = co_await getNameTask(chatId, bot, msgQueue);
    bot.getApi().sendMessage(chatId, "Your name is: " + name);

    // Get Operation
    ClientOperation operation = co_await getOperationTask(chatId, bot, msgQueue);
    std::optional<int> sessionId;
    if (operation == ClientOperation::OP_JOIN_SPECIFIC_SESSION) {
        sessionId = co_await getSpecificSessionTask(chatId, bot, msgQueue);
    }

    BOOST_LOG_TRIVIAL(info) << "Name: " << name << " Operation: " << operation << " Session ID: " << sessionId.value_or(-1);

    // Create a NewConnection message
    NewConnection newConnection;
    newConnection.set_name(name);
    newConnection.set_operation(operation);
    if (sessionId.has_value()) {
        newConnection.set_session_id(sessionId.value());
    }

    // Send the NewConnection message to the server
    std::string serialisedNewConnection = newConnection.SerializeAsString();
    sendBytes(socket, serialisedNewConnection);
    bot.getApi().sendMessage(chatId, "Connecting to the server...");

    // Read the acknowledgement from the server
    bool isLeader = (operation == ClientOperation::OP_CREATE_SESSION);
    if (operation == ClientOperation::OP_JOIN_RANDOM_SESSION) {
        JoinRandomSessionAck joinRandomSessionAck;
        std::string serialisedJoinRandomSessionAck = readBytes(socket);
        joinRandomSessionAck.ParseFromString(serialisedJoinRandomSessionAck);
        bot.getApi().sendMessage(chatId, joinRandomSessionAck.message());

        if (joinRandomSessionAck.session_type() == JOIN_RANDOM_SESSION_NEW_SESSION) {
            isLeader = true;
        } else {
            isLeader = false;
            sessionId = joinRandomSessionAck.session_id();
            bot.getApi().sendMessage(chatId, "Your session ID is: " + std::to_string(sessionId.value()));
        }
    }

    if (operation == ClientOperation::OP_JOIN_SPECIFIC_SESSION) {
        JoinSessionAck joinSessionAck;
        std::string serialisedJoinSessionAck = readBytes(socket);
        joinSessionAck.ParseFromString(serialisedJoinSessionAck);
        bot.getApi().sendMessage(chatId, joinSessionAck.message());

        if (joinSessionAck.status() == AckStatus::ACK_SUCCEED) {
            sessionId = joinSessionAck.session_id();
        } else {
            BOOST_LOG_TRIVIAL(info) << "User failed to join session!";
            co_return;
        }
        bot.getApi().sendMessage(chatId, "Your session ID is: " + std::to_string(sessionId.value()));
    }

    if (isLeader) {
        CreateSessionAck createSessionAck;
        BOOST_LOG_TRIVIAL(info) << "Waiting for server response...";
        std::string serialisedCreateSessionAck = readBytes(socket);
        createSessionAck.ParseFromString(serialisedCreateSessionAck);
        bot.getApi().sendMessage(chatId, createSessionAck.message());

        if (createSessionAck.status() == AckStatus::ACK_SUCCEED) {
            sessionId = createSessionAck.session_id();
            bot.getApi().sendMessage(chatId, "You are the session leader.");
        } else {
            BOOST_LOG_TRIVIAL(error) << "Failed to create session!";
            co_return;
        }

        bot.getApi().sendMessage(chatId, "Your session ID is: " + std::to_string(sessionId.value()));
        // Wait until start then start

        while (true) {
            co_await waitUntilStartCommand(chatId, bot, msgQueue);
            StartGameMsg startGameMsg;
            sendBytes(socket, startGameMsg.SerializeAsString());
            AckMessage startGameAck;
            std::string serialisedStartGameAck = readBytes(socket);
            startGameAck.ParseFromString(serialisedStartGameAck);

            BOOST_LOG_TRIVIAL(info) << "Start Game Acknowledgement received!";
            if (startGameAck.status() == AckStatus::ACK_SUCCEED) {
                BOOST_LOG_TRIVIAL(info) << sessionId.value() << " has started..";
                bot.getApi().sendMessage(chatId, "Game started!");
                break;
            } else {
                bot.getApi().sendMessage(chatId, "Failed to start game!");
                bot.getApi().sendMessage(chatId, startGameAck.message());
            }
        }
    }

    while (true) {
        // TgBot::Message::Ptr message = co_await AwaitableMessage<TgBot::Message::Ptr>(messageQueue);
        TgMsg text = co_await msgQueue;
        bot.getApi().sendMessage(chatId, "Your message is: " + text->text);
    }
    co_return;
}
