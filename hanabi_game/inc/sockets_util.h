#pragma once
#include <stdexcept>

#include "player.h"
#include "proto_files.h"
#include "sockets.h"

inline std::optional<int> parseInt(const std::string& s) {
    try {
        return std::stoi(s);
    } catch (std::invalid_argument& e) {
        return std::nullopt;
    }
}

inline ClientResponse requestQuery(Player& player) {
    InfoMessage infoMsg;
    infoMsg.set_message("QUERY");
    infoMsg.set_signal(INFOSIGNAL_BREAK);
    sendBytes(player.socket, infoMsg.SerializeAsString());

    const std::optional<std::string> responseOpt = readBytesCatch(player.socket);
    if (!responseOpt.has_value()) {
        throw std::runtime_error("Error reading from socket");
    }
    ClientResponse response;
    response.ParseFromString(responseOpt.value());
    return response; // copy elision
}

inline int requestInt(const int lower, const int higher, const string& invalidMessage, Player& player) {
    std::optional<int> action{};
    while (action == std::nullopt) {
        ClientResponse response = requestQuery(player);
        action = response.optionselected();

        if (action.value() > higher || action.value() < lower) {
            action = std::nullopt;
            sendInfo(player.socket, invalidMessage);
        }
    }
    return action.value();
}

inline void broadcast(std::vector<Player>& players, const std::string& message) {
    for (Player& player : players) {
        sendInfo(player.socket, message);
    }
}