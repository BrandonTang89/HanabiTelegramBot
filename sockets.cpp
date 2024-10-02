#include "sockets.h"
#include <boost/asio.hpp>
#include <string>
using boost::asio::ip::tcp;
using std::string;

// Functions for Sending on Sockets
string read_(tcp::socket& socket) {
    send_(socket, ": "); // Send prompt to client

    boost::asio::streambuf buf;
    boost::asio::read_until(socket, buf, "\n");
    string data = boost::asio::buffer_cast<const char*>(buf.data());
    if (!data.empty() && data.back() == '\n') {
        data.pop_back();
    }
    return data;
}

void send_(tcp::socket& socket, const string& message) {
    const string msg = message + "\n";
    boost::asio::write(socket, boost::asio::buffer(message));
}

// Function to check if the socket is still connected
bool is_socket_connected(tcp::socket& socket) {
    if (!socket.is_open()) {
        return false;
    }

    // boost::system::error_code ec;
    // char buffer;
    // [[maybe_unused]] size_t bytes_read = socket.read_some(boost::asio::buffer(&buffer, 1), ec);

    // if (ec == boost::asio::error::eof || ec == boost::asio::error::connection_reset) {
    //     return false;
    // }

    return true;
}