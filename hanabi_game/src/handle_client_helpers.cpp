#include <mutex>
#include <thread>

#include "game.h"
#include "helper.h"
#include "pch.h"
#include "player.h"
#include "proto_files.h"
#include "session.h"
#include "sockets.h"
#include "handle_client_helpers.h"

using namespace boost::asio;
using namespace Ack;
using ip::tcp;
using std::string;

extern std::mutex sessions_mutex;
extern std::unordered_map<int, Session> sessions;  // session_id -> vector of sockets

void create_session(Player leader) {
    BOOST_LOG_TRIVIAL(info) << leader << " creating new session...";

    std::unique_lock<std::mutex> lock(sessions_mutex);
    int session_id = static_cast<int>(sessions.size());
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
    const string serialisedAck = ack.SerializeAsString();
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
            ack.set_status(AckStatus::ACK_FAILED);
            ack.set_message("Not enough players to start the game!");
            sendBytes(player_socket, ack.SerializeAsString());
        } else {
            ack.set_status(AckStatus::ACK_SUCCEED);
            ack.set_message("You have started the game!");
            sendBytes(player_socket, ack.SerializeAsString());
            break;
        }
    }

    // // Construct the game object
    InfoMessage infoMsg;
    infoMsg.set_message("Game started!");
    lock.lock();
    session.broadcast(infoMsg.SerializeAsString());
    lock.unlock();

    const std::unique_ptr<Game> game_ptr{new Game(std::move(session))};  // allocate on heap to avoid stack overflow

    // // Remove the session from the hashtable
    lock.lock();
    sessions.erase(session_id);
    lock.unlock();

    // // Start the game
    game_ptr->start();
}

void join_session(Player joiner, int sessionId) {
    // TODO in the afternoon
    BOOST_LOG_TRIVIAL(debug) << joiner << " trying to join session " << sessionId;
    std::unique_lock<std::mutex> lock(sessions_mutex);
    JoinSessionAck ack;
    if (!sessions.contains(sessionId)) {
        ack.set_status(AckStatus::ACK_FAILED);
        ack.set_message("Session does not exist!");
        const string serialisedAck = ack.SerializeAsString();
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
    const std::optional<std::reference_wrapper<Player>> playerOpt = sessions[sessionId].join(joiner);
    lock.unlock();
    if (!playerOpt.has_value()) {
        ack.set_status(AckStatus::ACK_FAILED);
        ack.set_message("Failed to join session! (Unknown error)");
        const string serialisedAck = ack.SerializeAsString();
        sendBytes(joiner.socket, serialisedAck);
        return;
    }

    Player& player = playerOpt.value().get();
    ack.set_status(AckStatus::ACK_SUCCEED);
    ack.set_message("Successfully joined session!");
    ack.set_session_id(sessionId);
    const string serialisedAck = ack.SerializeAsString();
    sendBytes(player.socket, serialisedAck);

    lock.lock();
    sessions[sessionId].broadcast(player.name + " has joined the session!");
    sessions[sessionId].broadcast_status();
    lock.unlock();
}

void join_random_session(Player joiner) {
    // TODO: Make this actually random rather than just joining the "first" session
    BOOST_LOG_TRIVIAL(info) << joiner << " joining random session...";
    std::unique_lock<std::mutex> lock(sessions_mutex);
    for (auto& [session_id, session] : sessions) {
        std::optional<std::reference_wrapper<Player>> playerOpt{session.join(joiner)};
        if (playerOpt.has_value()) {  // join the first available session
            lock.unlock();

            // Send the session id to the joiner
            JoinRandomSessionAck ack;
            ack.set_session_type(JoinRandomSessionType::JOIN_RANDOM_SESSION_EXISTING_SESSION);
            ack.set_message("Joined Existing Session!");
            ack.set_session_id(session_id);
            string serialisedAck = ack.SerializeAsString();
            sendBytes(playerOpt.value().get().socket, serialisedAck);

            // Inform other players
            Player& player = playerOpt.value().get();
            lock.lock();
            sessions[session_id].broadcast(player.name + " has joined the session!");
            sessions[session_id].broadcast_status();
            lock.unlock();

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
}