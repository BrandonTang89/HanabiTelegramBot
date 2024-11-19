#include "player.h"

Player::Player(string name_, tcp::socket&& socket_) : name{std::move(name_)}, socket{std::move(socket_)} {}

Player::Player(Player&& source) noexcept : name{std::move(source.name)}, socket{std::move(source.socket)} {}

Player& Player::operator=(Player&& source) noexcept {
    if (this != &source) {
        name = std::move(source.name);
        socket = std::move(source.socket);
    }
    return *this;
}

std::ostream& operator<<(std::ostream& os, const Player& player) {
    os << "Player{name: " << player.name << "}";
    return os;
}
