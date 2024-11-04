#ifndef PLAYER_H
#define PLAYER_H

#include "pch.h"

using namespace boost::asio;
using ip::tcp;
using std::string;

class Player {
   public:
    string name;
    tcp::socket socket;

    Player(string name_, tcp::socket socket_);

    // Move Constructor and Assignment Operations
    Player(Player&& source) noexcept;
    Player& operator=(Player&& source) noexcept;

    // Prevent Copying
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    // Overloaded << for debug
    friend std::ostream& operator<<(std::ostream& os, const Player& player);
};

#endif  // PLAYER_H