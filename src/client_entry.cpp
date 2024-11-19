#include "client_entry.h"

#include "ack.pb.h"
#include "newConnect.pb.h"
#include "pch.h"
#include "sockets.h"
#include "telegram_client_coroutine.hpp"
#include "telegram_keyboard.h"

using namespace boost::asio;

ClientCoroutine clientEntry(ChatIdType chatId, std::queue<std::string>& messageQueue, TgBot::Bot& bot) {
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
    std::string dummy = co_await Awaitable<std::string>(messageQueue);

    // Display welcome message to user
    std::string welcomeMessage = "Welcome to Hanabi! Enter /quit anytime to exit."; 
    TgBot::ReplyKeyboardMarkup::Ptr nameKeyboard(new TgBot::ReplyKeyboardMarkup);
    createOneColumnKeyboard({"/enterName", "/quit"}, nameKeyboard);
    bot.getApi().sendMessage(chatId, welcomeMessage, nullptr, nullptr, nameKeyboard);

    // Wait for user to select to enter name
    std::string cmd = co_await Awaitable<std::string>(messageQueue);
    BOOST_LOG_TRIVIAL(info) << "Client of id " << chatId << " received command: " << cmd;
    
    if (cmd == "/quit") {
        BOOST_LOG_TRIVIAL(info) << "Client of id " << chatId << " quit!";
        co_return;
    }

    std::string name = "Telegram User " + std::to_string(chatId);
    BOOST_LOG_TRIVIAL(info) << "Client of id " << chatId << " sending NewConnection message to server";

    NewConnection newConnection;
    newConnection.set_name(name);
    newConnection.set_operation(ClientOperation::OP_CREATE_SESSION);
    newConnection.set_session_id(0);

    // Send the NewConnection message to the server
    std::string serialized = newConnection.SerializeAsString();
    sendBytes(socket, serialized);

    printf("Sent NewConnection message to server\n");
    while (true) {
        // TgBot::Message::Ptr message = co_await Awaitable<TgBot::Message::Ptr>(messageQueue);
        std::string text = co_await Awaitable<std::string>(messageQueue);
        bot.getApi().sendMessage(chatId, "Your message is: " + text);
    }
    co_return;
}