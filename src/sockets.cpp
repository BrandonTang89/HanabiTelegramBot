#include "sockets.h"

#include <string>
#include "pch.h"
#include "base64.hpp"
using boost::asio::ip::tcp;
using std::string;

// Send bytes with length prepended
void sendBytes(tcp::socket& socket, const string& message) {
    uint32_t length = htonl(message.size());
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(&length, sizeof(length)));
    buffers.push_back(boost::asio::buffer(message));
    boost::asio::write(socket, buffers);
}

string readBytes(tcp::socket& socket) {
    uint32_t length;
    boost::asio::read(socket, boost::asio::buffer(&length, sizeof(length)));
    length = ntohl(length);

    std::vector<char> buffer(length);
    boost::asio::read(socket, boost::asio::buffer(buffer.data(), buffer.size()));

    return string(buffer.data(), buffer.size());
}

std::optional<string> readBytesCatch(tcp::socket& socket) noexcept {
    try {
        return readBytes(socket);
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Error reading bytes: " << e.what();
        return std::nullopt;
    }
}

void send_(tcp::socket& socket, const string& message) noexcept {
    try {
        const string msg = message;
        BOOST_LOG_TRIVIAL(trace) << "Sending: " << message;
        boost::asio::write(socket, boost::asio::buffer(message));
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Error sending message: " << e.what();
        // Handle the error as needed
    }
}


// === Deprecated ===
// Send strings delimited by \n
string read_(tcp::socket& socket) {
    boost::asio::streambuf buf;
    boost::asio::read_until(socket, buf, "\n");
    string data = boost::asio::buffer_cast<const char*>(buf.data());
    if (!data.empty() && data.back() == '\n') {
        data.pop_back();
    }

    BOOST_LOG_TRIVIAL(trace) << "Received: " << data;
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

// Send bytes encoded as base64, delimited by \n
void sendBase64(tcp::socket& socket, const string& message) {
    string base64Message = base64::to_base64(message);
    send_(socket, base64Message + '\n');
}
string readBase64(tcp::socket& socket) {
    string base64Message = read_(socket);
    return base64::from_base64(base64Message);
}

std::optional<string> readBase64Catch(tcp::socket& socket) noexcept {
    try {
        return readBase64(socket);
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Error reading message: " << e.what();
        return std::nullopt;
    }
}

// Function to check if the socket is still connected
bool is_socket_connected(tcp::socket& socket) {
    if (!socket.is_open()) {
        return false;
    }
    return true;
}
