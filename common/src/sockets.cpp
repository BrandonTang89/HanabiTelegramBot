#include "sockets.h"

#include <string>
#include "pch.h"
#include "proto_files.h"
using boost::asio::ip::tcp;
using std::string;

// Send bytes with length prepended
void sendBytes(tcp::socket& socket, const string& message) {
    uint32_t length = htonl(message.size());
    std::vector<boost::asio::const_buffer> buffers;
    buffers.emplace_back(boost::asio::buffer(&length, sizeof(length)));
    buffers.emplace_back(boost::asio::buffer(message));

    BOOST_LOG_TRIVIAL(trace) << "Sending: " << message;
    boost::asio::write(socket, buffers);
    BOOST_LOG_TRIVIAL(trace) << "Sent " << message.size() << " bytes";
}

string readBytes(tcp::socket& socket) {
    uint32_t length;
    boost::asio::read(socket, boost::asio::buffer(&length, sizeof(length)));
    length = ntohl(length);

    std::vector<char> buffer(length);
    boost::asio::read(socket, boost::asio::buffer(buffer.data(), buffer.size()));

    BOOST_LOG_TRIVIAL(trace) << "Received: " << string(buffer.data(), buffer.size());
    return {buffer.data(), buffer.size()};
}

std::optional<string> readBytesCatch(tcp::socket& socket) noexcept {
    try {
        return readBytes(socket);
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Error reading bytes: " << e.what();
        return std::nullopt;
    }
}

void sendInfo(tcp::socket& socket, const string& message, InfoSignal signal) {
    InfoMessage info;
    info.set_signal(signal);
    info.set_message(message);
    sendBytes(socket, info.SerializeAsString());
}

// === Deprecated ===
// Send strings delimited by \n
void send_(tcp::socket& socket, const string& message) noexcept {
    try {
        BOOST_LOG_TRIVIAL(trace) << "Sending: " << message;
        boost::asio::write(socket, boost::asio::buffer(message));
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Error sending message: " << e.what();
        // Handle the error as needed
    }
}

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

// Function to check if the socket is still connected
bool is_socket_connected(tcp::socket& socket) {
    if (!socket.is_open()) {
        return false;
    }
    return true;
}
