#include "client_entry.h"

#include "telegram_client_coroutine.hpp"
#include "newConnect.pb.h"
#include "pch.h"
#include "sockets.h"
using namespace boost::asio;

ClientCoroutine clientEntry(ChatIdType chatId, std::queue<std::string>& messageQueue, TgBot::Bot& bot) {
    // Connects to the server as a new client
    // Create a socket connected to the server at localhost:1234
    io_service io_service;
    ip::tcp::socket socket(io_service);
    socket.connect(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 1234));

    // // Read welcome message from server
    std::string welcome = read_(socket);
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