#include <iostream>
#include <mutex>
#include <thread>

#include "pch.h"
#include "proto_files.h"
#include "game.h"
#include "helper.h"
#include "player.h"
#include "session.h"
#include "sockets.h"

using namespace boost::asio;
using ip::tcp;
using namespace Ack;
using std::string;

static std::mutex sessions_mutex;
static std::unordered_map<int, Session> sessions;  // session_id -> vector of sockets

void create_session(Player leader) {
    BOOST_LOG_TRIVIAL(info) << leader << " creating new session...";

    std::unique_lock<std::mutex> lock(sessions_mutex);
    int session_id = sessions.size();
    sessions.emplace(session_id, Session(std::move(leader), session_id));

    // note that these references will not dangle on hash table resize
    Session& session = sessions[session_id];
    tcp::socket& player_socket = session.getLeader().socket;
    lock.unlock();
    BOOST_LOG_TRIVIAL(debug) << sessions[session_id] << " created";

    // Send the session id to the leader
    CreateSessionAck ack;
    ack.set_status(AckStatus::ACK_SUCCEED);
    ack.set_message("Session created successfully!");
    ack.set_session_id(session_id);
    string serialisedAck = ack.SerializeAsString();
    sendBytes(player_socket, serialisedAck);

    // Wait for all players to join
    while (true) {
        std::optional<string> serialisedStartGameMsgOpt{readBytesCatch(player_socket)};
        if (!serialisedStartGameMsgOpt.has_value()) {
            BOOST_LOG_TRIVIAL(info) << "Leader of session " << session_id << " disconnected!";
            session.invalidate();  // Invalidate the session
            lock.lock();           // Remove the session from the hashtable
            sessions.erase(session_id);
            lock.unlock();
            return;
        }

        StartGameMsg startGameMsg;
        startGameMsg.ParseFromString(serialisedStartGameMsgOpt.value());
        if (session.getNumPlayers() == 1) {
            AckMessage ack;
            ack.set_status(AckStatus::ACK_FAILED);
            ack.set_message("Not enough players to start the game!");
            sendBytes(player_socket, ack.SerializeAsString());
        } else {
            AckMessage ack;
            ack.set_status(AckStatus::ACK_SUCCEED);
            ack.set_message("You have started the game!");
            sendBytes(player_socket, ack.SerializeAsString());
            break;
        }
    }

    // // Construct the game object
    // send_(player_socket, "Starting the game...\n");
    // std::unique_ptr<Game> gptr{new Game(std::move(session))};  // allocate on heap to avoid stack overflow

    // // Remove the session from the hashtable
    // lock.lock();
    // sessions.erase(session_id);
    // lock.unlock();

    // // Start the game
    // gptr->start();
    return;
}

void join_session(Player joiner, int sessionId) {
    // TODO in the afternoon
    BOOST_LOG_TRIVIAL(debug) << joiner << " trying to join session " << sessionId;
    std::unique_lock<std::mutex> lock(sessions_mutex);
    JoinSessionAck ack;
    if (sessions.find(sessionId) == sessions.end()) {
        ack.set_status(AckStatus::ACK_FAILED);
        ack.set_message("Session does not exist!");
        string serialisedAck = ack.SerializeAsString();
        lock.unlock();
        sendBytes(joiner.socket, serialisedAck);
        return;
    }
    if (sessions[sessionId].getNumPlayers() >= Session::maxPlayers) {
        ack.set_status(AckStatus::ACK_FAILED);
        ack.set_message("Session is full!");
        string serialisedAck = ack.SerializeAsString();
        lock.unlock();
        sendBytes(joiner.socket, serialisedAck);
        return;
    }
    std::optional<std::reference_wrapper<Player>> playerOpt = sessions[sessionId].join(joiner);
    lock.unlock();
    if (!playerOpt.has_value()) {
        ack.set_status(AckStatus::ACK_FAILED);
        ack.set_message("Failed to join session! (Unknown error)");
        string serialisedAck = ack.SerializeAsString();
        sendBytes(joiner.socket, serialisedAck);
        return;
    }

    Player& player = playerOpt.value().get();
    ack.set_status(AckStatus::ACK_SUCCEED);
    ack.set_message("Successfully joined session!");
    ack.set_session_id(sessionId);
    string serialisedAck = ack.SerializeAsString();
    sendBytes(player.socket, serialisedAck);
    
    // TO DO, inform other players
    lock.lock();
    sessions[sessionId].broadcast(player.name + " has joined the session!");
    lock.unlock();
    // no more control over joiner
}

void join_random_session(Player joiner) {
    // TODO: Make this actually random rather than just joining the "first" session
    BOOST_LOG_TRIVIAL(info) << joiner << " joining random session...";
    std::unique_lock<std::mutex> lock(sessions_mutex);
    for (auto& [session_id, session] : sessions) {
        std::optional<std::reference_wrapper<Player>> joinerOpt{session.join(joiner)};
        if (joinerOpt.has_value()) {  // join the first available session
            lock.unlock();

            // Send the session id to the joiner
            JoinRandomSessionAck ack;
            ack.set_session_type(JoinRandomSessionType::JOIN_RANDOM_SESSION_EXISTING_SESSION);
            ack.set_message("Joined Existing Session!");
            ack.set_session_id(session_id);
            string serialisedAck = ack.SerializeAsString();
            sendBytes(joinerOpt.value().get().socket, serialisedAck);

            return;
        }
    }
    lock.unlock();

    // If no session is found, create a new session
    BOOST_LOG_TRIVIAL(info) << "No available sessions found! Creating a new session...";
    JoinRandomSessionAck ack;
    ack.set_session_type(JoinRandomSessionType::JOIN_RANDOM_SESSION_NEW_SESSION);
    ack.set_message("No available sessions found! Creating a new session...");
    string serialisedAck = ack.SerializeAsString();
    sendBytes(joiner.socket, serialisedAck);
    create_session(std::move(joiner));
    return;
}

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
