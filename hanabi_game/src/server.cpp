#include <iostream>
#include <mutex>
#include <thread>

#include "pch.h"
#include "handle_client_helpers.h"
#include "game.h"
#include "helper.h"
#include "player.h"
#include "proto_files.h"
#include "session.h"
#include "sockets.h"

using namespace boost::asio;
using namespace Ack;
using ip::tcp;
using std::string;

std::mutex sessions_mutex;
std::unordered_map<int, Session> sessions;  // session_id -> vector of sockets

void handle_client(tcp::socket socket) {
    // Greet Client
    AckMessage ack;
    ack.set_status(AckStatus::ACK_SUCCEED);
    ack.set_message("Welcome to the server!");
    std::string serialisedAck = ack.SerializeAsString();
    sendBytes(socket, serialisedAck);

    // Get name and build player object
    std::optional<string> serialisedNewConnectionOpt{readBytesCatch(socket)};
    if (!serialisedNewConnectionOpt.has_value()) {
        BOOST_LOG_TRIVIAL(info) << "Client disconnected!";
        return;
    }
    string serialisedNewConnection{serialisedNewConnectionOpt.value()};

    NewConnection newConnection;
    if (!newConnection.ParseFromString(serialisedNewConnection)) {
        BOOST_LOG_TRIVIAL(error) << "Bad connection request! dropping connection...";
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "New Connection:\n"
                             << newConnection.DebugString();

    string name{newConnection.name()};
    ClientOperation operation{newConnection.operation()};
    Player player{name, std::move(socket)};
    if (operation == ClientOperation::OP_CREATE_SESSION) {
        create_session(std::move(player));
    } else if (operation == ClientOperation::OP_JOIN_SPECIFIC_SESSION) {
        join_session(std::move(player), newConnection.session_id());
    } else {
        join_random_session(std::move(player));
    }
    return;
}

int main(int, char*[]) {
    boost::asio::io_service io_service;
    tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), 1234));  // listen for new connection
    BOOST_LOG_TRIVIAL(info) << "Server started!";
    while (true) {
        tcp::socket socket_(io_service);  // socket creation
        acceptor_.accept(socket_);        // waiting for connection

        BOOST_LOG_TRIVIAL(info) << "New Client Connected!";
        std::thread(handle_client, std::move(socket_)).detach();
    }

    return 0;
}
