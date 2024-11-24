#include "client_entry.h"

#include <stack>

#include "ack.pb.h"
#include "newConnect.pb.h"
#include "pch.h"
#include "sockets.h"
#include "telegram_client_coroutine.hpp"
#include "telegram_client_pch.h"
#include "telegram_keyboard.h"
using namespace boost::asio;

#include <iostream>

Task welcomeTask(ChatIdType chatId, TgBot::Bot& bot) {
    std::string welcomeMessage = "Welcome to Hanabi!\n Enter /quit anytime to exit. \n Please enter your name:";
    bot.getApi().sendMessage(chatId, welcomeMessage);
    BOOST_LOG_TRIVIAL(debug) << "Sent welcome message to chat " << chatId;
    co_return;
}

Task clientEntry(ChatIdType chatId, MessageQueue<TgMsg>& msgQueue, TgBot::Bot& bot) {
    // Connects to the server as a new client
    // Create a socket connected to the server at localhost:1234
    io_service io_service;
    ip::tcp::socket socket(io_service);
    socket.connect(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 1234));
    BOOST_LOG_TRIVIAL(info) << "Client of id " << chatId << " connected to server";

    // Read welcome acknowledgement from server
    Ack ack;
    std::string serialisedAck = readBytes(socket);
    ack.ParseFromString(serialisedAck);
    if (ack.status() != AckStatus::ACK_SUCCEED) {
        BOOST_LOG_TRIVIAL(error) << "Failed to connect to server!";
        co_return;
    }

    // Remove the starting message from the queue
    TgMsg dummy = co_await msgQueue;

    // Display welcome message to user
    co_await welcomeTask(chatId, bot);  // add the coroutine to the stack to allow message passing

    // Wait for user to select to enter name
    // if (cmd == "/quit") {
    //     BOOST_LOG_TRIVIAL(info) << "Client of id " << chatId << " quit!";
    //     bot.getApi().sendMessage(chatId, "Goodbye!");
    //     co_return;
    // }

    // === Get the Name ===
    // std::string name = co_await AwaiterForMessage<std::string>(messageQueue);
    // while (name.empty() || name[0] == '/') {
    //     bot.getApi().sendMessage(chatId, "Name cannot be empty or begin with a /\n Please enter your name:");
    //     name = co_await AwaiterForMessage<std::string>(messageQueue);
    // }
    // BOOST_LOG_TRIVIAL(info) << "Client of id " << chatId << " received name: " << name;

    // // === Get the Operation ===
    // std::vector<std::string> operationStrings = {"/createSession", "/joinSession", "/joinRandomSession"};
    // TgBot::ReplyKeyboardMarkup::Ptr operationKeyboard(new TgBot::ReplyKeyboardMarkup);
    // createOneColumnKeyboard(operationStrings, operationKeyboard);
    // bot.getApi().sendMessage(chatId, "Please select an operation: \n 1. Create a new session \n 2. Join a specific session \n 3. Join a random session", nullptr, 0, operationKeyboard);
    // std::string operation = co_await AwaiterForMessage<std::string>(messageQueue);
    // while (operation.empty() || find(operationStrings.begin(), operationStrings.end(), operation) == operationStrings.end()) {
    //     bot.getApi().sendMessage(chatId, "Invalid operation! Please select an operation: \n 1. Create a new session \n 2. Join a specific session \n 3. Join a random session", nullptr, 0, operationKeyboard);
    //     operation = co_await AwaitableMessage<std::string>(messageQueue);
    // }
    // BOOST_LOG_TRIVIAL(info) << "Client of id " << chatId << " received operation: " << operation;

    // // === Get the Session ID ===
    // std::optional<int> sessionId;
    // if (operation == "/joinSession") {
    //     bot.getApi().sendMessage(chatId, "Please enter the session ID:");
    //     std::string sessionIdStr = co_await AwaitableMessage<std::string>(messageQueue);
    //     while (sessionIdStr.empty() || !std::all_of(sessionIdStr.begin(), sessionIdStr.end(), ::isdigit)) {
    //         bot.getApi().sendMessage(chatId, "Session ID should be a number! Please enter the session ID:");
    //         sessionIdStr = co_await AwaitableMessage<std::string>(messageQueue);
    //     }
    //     sessionId = std::stoi(sessionIdStr);
    //     BOOST_LOG_TRIVIAL(info) << "Client of id " << chatId << " received session ID: " << sessionId.value();
    // }

    // BOOST_LOG_TRIVIAL(info) << "Client of id " << chatId << " sending NewConnection message to server";
    // NewConnection newConnection;
    // newConnection.set_name(name);
    // newConnection.set_operation(ClientOperation::OP_CREATE_SESSION);
    // newConnection.set_session_id(0);

    // // Send the NewConnection message to the server
    // std::string serialized = newConnection.SerializeAsString();
    // sendBytes(socket, serialized);

    // printf("Sent NewConnection message to server\n");
    while (true) {
        // TgBot::Message::Ptr message = co_await AwaitableMessage<TgBot::Message::Ptr>(messageQueue);
        TgMsg text = co_await msgQueue;
        bot.getApi().sendMessage(chatId, "Your message is: " + text->text);
    }
    co_return;
}
