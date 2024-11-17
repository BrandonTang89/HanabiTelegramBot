#include "pch.h"
#include "sockets.h"
#include "newConnect.pb.h"

using namespace boost::asio;
void newClient(){
    // Connects to the server as a new client

    // Create a socket connected to the server at localhost:1234
    io_service io_service;
    ip::tcp::socket socket(io_service);
    socket.connect(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 1234));

    // // Read welcome message from server
    std::string welcome = read_(socket);
    std::cout << welcome << std::endl;

    std::string name = "Alice";

    NewConnection newConnection;
    newConnection.set_name(name);
    newConnection.set_operation(ClientOperation::OP_CREATE_SESSION);
    newConnection.set_session_id(0);

    // Send the NewConnection message to the server
    std::string serialized;
    newConnection.SerializeToString(&serialized);
    send_(socket, serialized);
}


int main(){
    newClient();
    std::cout << "-- Telegram Client Manager Closing --" << std::endl;
    return 0;
}
