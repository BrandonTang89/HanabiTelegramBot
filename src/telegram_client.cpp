#include <tgbot/tgbot.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "newConnect.pb.h"
#include "pch.h"
#include "sockets.h"

using namespace boost::asio;

void loadEnv() {
    // Get the directory of the current source file
    std::filesystem::path sourceDir = std::filesystem::path(__FILE__).parent_path().parent_path();  // at the root of the project
    // Construct the path to the .env file relative to the source file
    std::filesystem::path envFilePath = sourceDir / ".env";

    std::ifstream envFile(envFilePath);
    if (!envFile.is_open()) {
        std::cerr << "Failed to open .env file at " << envFilePath << std::endl;
        return;
    }

    std::string line;
    while (std::getline(envFile, line)) {
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            setenv(key.c_str(), value.c_str(), 1);
        }
    }
    envFile.close();
}

void newClient(long long chatId) {
    // Connects to the server as a new client

    // Create a socket connected to the server at localhost:1234
    io_service io_service;
    ip::tcp::socket socket(io_service);
    socket.connect(ip::tcp::endpoint(ip::address::from_string("127.0.0.1"), 1234));

    // // Read welcome message from server
    std::string welcome = read_(socket);
    std::cout << welcome << std::endl;

    std::string name = "Telegram User " + std::to_string(chatId);

    NewConnection newConnection;
    newConnection.set_name(name);
    newConnection.set_operation(ClientOperation::OP_CREATE_SESSION);
    newConnection.set_session_id(0);

    // Send the NewConnection message to the server
    std::string serialized;
    newConnection.SerializeToString(&serialized);
    send_(socket, serialized);
}

int main() {
    loadEnv();
    TgBot::Bot bot(getenv("TELEGRAM_BOT_API_TOKEN"));
    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
        bot.getApi().sendMessage(message->chat->id, "Hi!");
    });
    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        printf("User wrote %s\n", message->text.c_str());
        if (StringTools::startsWith(message->text, "/start")) {
            return;
        }
        bot.getApi().sendMessage(message->chat->id, "Your message is: " + message->text);
        newClient(message->chat->id);
    });
    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            printf("Long poll started\n");
            longPoll.start();
        }
    } catch (TgBot::TgException& e) {
        printf("error: %s\n", e.what());
    }
    return 0;
}

// int main() {
//     loadEnv();
//     newClient();
//     std::cout << "-- Telegram Client Manager Closing --" << std::endl;
//     return 0;
// }
