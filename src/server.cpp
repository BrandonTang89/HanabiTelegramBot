#include <iostream>
#include <mutex>

#include "pch.h"
#include "game.h"
#include "helper.h"
#include "player.h"
#include "session.h"
#include "sockets.h"

#include "newConnect.pb.h"

using namespace boost::asio;
using ip::tcp;
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

    // Wait for all players to join
    while (true) {
        send_(player_socket, "Type \"start\" and hit [ENTER] when all your session members have joined!\n");
        std::optional<string> startOpt = _readCatch(player_socket);
        if (!startOpt.has_value()) {
            BOOST_LOG_TRIVIAL(info) << "Leader of session " << session_id << " disconnected!";

            session.invalidate();  // Invalidate the session

            lock.lock();  // Remove the session from the hashtable
            sessions.erase(session_id);
            lock.unlock();
            return;
        }
        string start = startOpt.value();
        if (start == "start") {
            if (session.getNumPlayers() == 1) {
                send_(player_socket, "You are the only player in the session. Please wait for others to join.\n");
            } else {
                break;
            }
        }
    }

    // Construct the game object
    send_(player_socket, "Starting the game...\n");
    std::unique_ptr<Game> gptr{new Game(std::move(session))};  // allocate on heap to avoid stack overflow

    // Remove the session from the hashtable
    lock.lock();
    sessions.erase(session_id);
    lock.unlock();

    // Start the game
    gptr->start();
    return;
}

void join_session(Player joiner) {
    BOOST_LOG_TRIVIAL(info) << joiner << " selecting session to join...";
    std::unique_lock<std::mutex> lock(sessions_mutex);
    lock.unlock();

    while (true) {
        send_(joiner.socket, "Enter the ID of the session you want to join");
        int sessionId;
        try {
            sessionId = requestInt(0, INT_MAX, "Invalid Session ID!\n", joiner);
        } catch (std::exception& e) {
            BOOST_LOG_TRIVIAL(info) << joiner << " disconnected!";
            return;
        }

        BOOST_LOG_TRIVIAL(debug) << joiner << " trying to join session " << sessionId;
        lock.lock();
        if (sessions.find(sessionId) != sessions.end() && sessions[sessionId].join(joiner)) {
            lock.unlock();
            break;
        } else {
            send_(joiner.socket, "Unable to join the session specified...\n");
        }
        lock.unlock();
    }
    return;
    // no more control over joiner
}

std::optional<string> getName(tcp::socket& socket) {
    while (true) {
        send_(socket, "Please enter your name");
        std::optional<string> name_opt{_readCatch(socket)};
        if (!name_opt.has_value()) {
            return std::nullopt;
        }
        string name{name_opt.value()};

        // name validation
        if (name.empty()) {
            send_(socket, "Name cannot be empty!\n");
        } else {
            return name_opt;
        }
    }
}

void handle_client(tcp::socket socket) {
    // Greet Client
    send_(socket, "Welcome to Hanbi!\n");

    // Get name and build player object
    std::optional<string> name_opt{getName(socket)};
    if (!name_opt.has_value()) {
        BOOST_LOG_TRIVIAL(info) << "Client disconnected!";
        return;
    }
    string name{name_opt.value()};

    BOOST_LOG_TRIVIAL(info) << "New Client: " << name;
    Player player{name, std::move(socket)};

    // Show Menu of Options
    static const string menu = "0. Create New Session\n1. Join Existing Session\n2. Quit\n";
    send_(player.socket, menu);

    // Handle Client Input
    while (true) {
        std::optional<string> choice_opt{_readCatch(player.socket)};
        if (!choice_opt.has_value()) {
            BOOST_LOG_TRIVIAL(info) << "Client disconnected!";
            return;
        }
        string choice{choice_opt.value()};

        if (choice == "0") {
            create_session(std::move(player));
            break;
        } else if (choice == "1") {
            join_session(std::move(player));
            break;
        } else if (choice == "2") {
            send_(player.socket, "Goodbye!\n");
            BOOST_LOG_TRIVIAL(info) << "Client " + name + " disconnected!";
            break;
        } else {
            send_(player.socket, "Invalid Choice!\n");
        }
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
