#ifndef SOCKETSH
#define SOCKETSH

#include <boost/asio.hpp>
#include <string>

using boost::asio::ip::tcp;
using std::string;

string read_(tcp::socket& socket);
void send_(tcp::socket& socket, const string& message);
bool is_socket_connected(tcp::socket& socket);

#endif // SOCKETSH