#ifndef HELPER_H
#define HELPER_H

#include <string>
#include <stdexcept>
#include <optional>
#include "sockets.h"
#include "player.h"

inline std::optional<int> parseInt(const std::string& s) {
    try {
        return std::stoi(s);
    } catch (std::invalid_argument& e) {
        return std::nullopt;
    }
}

inline int requestInt(int lower, int higher, const string& invalidMessage, Player& player){
    std::optional<int> action{};
    while (action == std::nullopt) {
        action = parseInt(read_(player.socket));
        if (!action.has_value() || action.value() > higher || action.value() < lower){
            action = std::nullopt;
            send_(player.socket, invalidMessage);
        }
    }
    return action.value();
}

void broadcast(std::vector<Player>& players, const std::string& message) {
    for (Player& player : players) {
        send_(player.socket, message);
    }
}


#endif // HELPER_H