#pragma once
#include "pch.h"

using boost::asio::ip::tcp;
using std::string;

void sendBytes(tcp::socket& socket, const string& message);
string readBytes(tcp::socket& socket);
std::optional<string> readBytesCatch(tcp::socket& socket) noexcept;

void send_(tcp::socket& socket, const string& message) noexcept;
string read_(tcp::socket& socket);
std::optional<string> _readCatch(tcp::socket& socket) noexcept;

bool is_socket_connected(tcp::socket& socket);