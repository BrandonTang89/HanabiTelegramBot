#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <iostream>
#include <mutex>
#include <thread>

#include "helper.h"
#include "player.h"
#include "session.h"
#include "sockets.h"
#include "game.h"

using namespace boost::asio;
using ip::tcp;
using std::string;

std::mutex sessions_mutex;
std::unordered_map<int, Session> sessions;  // session_id -> vector of sockets

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
        string start = read_(player_socket);
        if (start == "start") {
            if (session.getNumPlayers() == 1) {
                send_(player_socket, "You are the only player in the session. Please wait for others to join.\n");
            } else {
                break;
            }
        }
    }

    // Start the game
    send_(player_socket, "Starting the game...\n");

    std::unique_ptr<Game> gptr {new Game(std::move(session))}; // allocate on heap to avoid stack overflow
    gptr->start();
    return;
}

void join_session(Player joiner) {
    BOOST_LOG_TRIVIAL(info) << joiner << " selecting session to join...";
    std::unique_lock<std::mutex> lock(sessions_mutex);
    lock.unlock();

    while (true) {
        send_(joiner.socket, "Enter the ID of the session you want to join");
        string sessionIdS = read_(joiner.socket);
        int sessionId = parseInt(sessionIdS);

        if (sessionId < 0) {
            BOOST_LOG_TRIVIAL(trace) << joiner << " sent ill-formatted session id";
            send_(joiner.socket, "Session IDs are positive integers.\n");
            continue;
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

void handle_client(tcp::socket socket) {
    // Greet Client
    send_(socket, "Welcome to Hanbi!\n");

    // Get name and build player object
    send_(socket, "Please enter your name");
    string name{read_(socket)};

    Player player{name, std::move(socket)};

    // Show Menu of Options
    static const string menu = "1. Create New Session\n2. Join Existing Session\n3. Quit\n";
    send_(player.socket, menu);

    // Handle Client Input
    while (true) {
        string choice = read_(player.socket);
        if (choice == "1") {
            create_session(std::move(player));

            break;
        } else if (choice == "2") {
            join_session(std::move(player));
            break;
        } else if (choice == "3") {
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