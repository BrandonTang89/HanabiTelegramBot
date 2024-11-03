#include "sockets.h"

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <string>
using boost::asio::ip::tcp;
using std::string;

// Functions for Sending on Sockets
string read_(tcp::socket& socket) {
    send_(socket, ": ");  // Send prompt to client

    boost::asio::streambuf buf;
    boost::asio::read_until(socket, buf, "\n");
    string data = boost::asio::buffer_cast<const char*>(buf.data());
    if (!data.empty() && data.back() == '\n') {
        data.pop_back();
    }

    // BOOST_LOG_TRIVIAL(trace) << "Received: " << data;
    return data;
}

std::optional<string> _readCatch(tcp::socket& socket) noexcept {
    try {
        return read_(socket);
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Error reading message: " << e.what();
        return std::nullopt;
    }
}

void send_(tcp::socket& socket, const string& message) noexcept {
    try {
        const string msg = message + "\n";
        boost::asio::write(socket, boost::asio::buffer(message));
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Error sending message: " << e.what();
        // Handle the error as needed
    }
}

// Function to check if the socket is still connected
bool is_socket_connected(tcp::socket& socket) {
    if (!socket.is_open()) {
        return false;
    }
    return true;
}