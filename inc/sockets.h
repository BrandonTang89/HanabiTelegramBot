#ifndef SOCKETSH
#define SOCKETSH

#include "pch.h"

using boost::asio::ip::tcp;
using std::string;

string read_(tcp::socket& socket);
string readBase64(tcp::socket& socket);
std::optional<string> readBase64Catch(tcp::socket& socket) noexcept;
std::optional<string> _readCatch(tcp::socket& socket) noexcept;
void send_(tcp::socket& socket, const string& message) noexcept;
void sendBase64(tcp::socket& socket, const string& message);
bool is_socket_connected(tcp::socket& socket);

#endif // SOCKETSH