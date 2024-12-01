#include "client_entry.h"

#include "client_input_tasks.h"
#include "client_entry_helpers.h"
#include "pch.h"
#include "proto_files.h"
#include "sockets.h"
#include "telegram_client_coroutine.hpp"
#include "telegram_client_pch.h"
using namespace boost::asio;
using namespace Ack;


Task<> clientEntry(ChatIdType chatId, MessageQueue<TgMsg>& msgQueue, TgBot::Bot& bot, io_context& io_ctx) {
    // Connects to the server as a new client
    // Create a socket connected to the server at localhost:1234
    // Move down eventually when the project matures (now here for debug)
    ip::tcp::socket socket(io_ctx);
    socket.connect(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 1234));
    BOOST_LOG_TRIVIAL(info) << "Client of id " << chatId << " connected to server";

    // Create a new client to pass to children tasks/functions
    SignallingEvent clientEvent;
    Client client = {chatId, socket, msgQueue, clientEvent, bot};

    AckMessage ack;
    std::string serialisedAck = readBytes(socket);
    ack.ParseFromString(serialisedAck);
    if (ack.status() != AckStatus::ACK_SUCCEED) {
        BOOST_LOG_TRIVIAL(error) << "Failed to connect to server!";
        co_return;
    }

    co_await msgQueue;                  // remove the starting message if any
    co_await welcomeTask(client);  // Display welcome message to user

    // Get Name
    // DEBUG ONLY
    // std::string name = co_await getNameTask(chatId, bot, msgQueue);
    std::string name = "Brandon";
    bot.getApi().sendMessage(chatId, "Your name is: " + name);

    // Get Operation
    // DEBUG ONLY
    // ClientOperation operation = co_await getOperationTask(chatId, bot, msgQueue);
    ClientOperation operation = ClientOperation::OP_JOIN_RANDOM_SESSION;

    std::optional<int> sessionId;
    if (operation == ClientOperation::OP_JOIN_SPECIFIC_SESSION) {
        sessionId = co_await getSpecificSessionTask(client);
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
    const std::string serialisedNewConnection = newConnection.SerializeAsString();
    sendBytes(socket, serialisedNewConnection);
    bot.getApi().sendMessage(chatId, "Connecting to the server...");

    // Read response from server
    bool isLeader = (operation == ClientOperation::OP_CREATE_SESSION);
    if (operation == ClientOperation::OP_JOIN_RANDOM_SESSION) {
        sessionId = joinRandomSession(client);
        if (!sessionId.has_value()) {
            isLeader = true;
        }
    }

    if (operation == ClientOperation::OP_JOIN_SPECIFIC_SESSION) {
        sessionId = joinSpecificSession(client);
        if (!sessionId.has_value()) {
            co_return;
        }
    }

    if (isLeader) {
        sessionId = createSession(client);
        if (!sessionId.has_value()) {
            co_return;
        }

        co_await leaderTask(client, sessionId);
    }

    // === Game has started
    // All execution traces that lead to here have the socket subscribing to the info messages
    while (true) {
        // co_await clientEvent; // wait for the signal that it is my turn

        const TgMsg text = co_await msgQueue;
        bot.getApi().sendMessage(chatId, "Your message is: " + text->text);
    }

    co_return;
}
