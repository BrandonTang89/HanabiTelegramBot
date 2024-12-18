#include "session.h"
#include <cassert>

#include "pch.h"
#include "player.h"
#include "proto_files.h"
#include "sockets.h"

// Constructor
Session::Session(Player leader_, int sessionId_) : sessionId{sessionId_} {
    players.reserve(maxPlayers);
    players.emplace_back(std::move(leader_));
    numPlayers = 1;
}

// Move Constructor
Session::Session(Session&& source) noexcept
    : sessionId{source.sessionId}, numPlayers{source.numPlayers} {
    players.reserve(maxPlayers);
    for (auto& player : source.players) {
        players.emplace_back(std::move(player));
    }
}

// Join Method
std::optional<std::reference_wrapper<Player>> Session::join(Player& player) {
    std::lock_guard<std::mutex> guard(session_mutex);
    if (static_cast<int>(players.size()) == 0) {
        BOOST_LOG_TRIVIAL(fatal) << sessionId << " had no leader!";
        return std::nullopt;  // empty session
    }
    if (!is_socket_connected(players[0].socket)) {
        BOOST_LOG_TRIVIAL(info) << sessionId << " has disconnected leader";
        return std::nullopt;
    }
    if (numPlayers >= maxPlayers) {
        BOOST_LOG_TRIVIAL(info) << sessionId << " has reached max number of players";
        return std::nullopt;
    }
    players.emplace_back(std::move(player));
    numPlayers++;
    BOOST_LOG_TRIVIAL(debug) << players.back() << " successfully joined session " << sessionId;

    return players.back();
}

void Session::broadcast(const string& message) {
    InfoMessage infoMsg;
    infoMsg.set_message(message);

    std::lock_guard<std::mutex> guard(session_mutex);
    BOOST_LOG_TRIVIAL(debug) << "Broadcasting message to all players in session " << sessionId << ": " << message;
    for (auto& player : players) {
        sendBytes(player.socket, infoMsg.SerializeAsString());
    }
}

void Session::broadcast_status() {
    InfoMessage infoMsg;
std:
    string message = "Session " + std::to_string(sessionId) + " has " + std::to_string(numPlayers) + " players:\n";
    for (auto& player : players) {
        message += player.name + "\n";
    }

    infoMsg.set_message(message);

    std::lock_guard<std::mutex> guard(session_mutex);
    for (auto& player : players) {
        sendBytes(player.socket, infoMsg.SerializeAsString());
    }
}

// Overloaded << Operator for Session Class
std::ostream& operator<<(std::ostream& os, const Session& sess) {
    assert(static_cast<int>(sess.players.size()) > 0);
    os << "Session{leader: " << sess.players[0] << ", sessionId: " << sess.sessionId << ", numPlayers: " << sess.numPlayers << "}";
    return os;
}

Player& Session::getLeader() {
    assert(static_cast<int>(players.size()) > 0);
    return players[0];
}

int Session::getNumPlayers() const {
    return numPlayers;
}

int Session::getId() const {
    return sessionId;
}

void Session::invalidate() {
    std::unique_lock<std::mutex> lock(session_mutex);
    BOOST_LOG_TRIVIAL(info) << "Session " << sessionId << " is now closed...";
    lock.unlock();
    broadcast("--> This session is now closed..\n");
}
