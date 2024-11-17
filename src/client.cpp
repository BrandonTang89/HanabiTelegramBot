#include "pch.h"
#include "sockets.h"
#include "newConnect.pb.h"

using namespace boost::asio;
void newClient(){
    // connects to the server as a new client

    // Create a socket connected to the server at localhost:1234
    io_service io_service;
    ip::tcp::socket socket(io_service);
    socket.connect(ip::tcp::endpoint(ip::address::from_string("localhost"), 1234));

    // Read welcome message from server
    std::string welcome = read_(socket);
    std::cout << welcome;
}


int main(){
    newClient();
    return 0;
}
