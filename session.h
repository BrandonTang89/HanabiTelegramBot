#ifndef SESSION_H
#define SESSION_H

#include <mutex>
#include <optional>
#include <vector>

class Player; // Forward Declaration

class Session {
   public:
    std::vector<Player> players; // leader is at index 0
    int sessionId;
    int numPlayers;
    std::mutex session_mutex;

    static constexpr int maxPlayers = 5;

    Session() {}
    Session(Player leader_, int sessionId_);
    Session(Session&& source) noexcept;
    Session(const Session&) = delete;
    Session& operator=(Session&& source) = delete;
    Session& operator=(const Session&) = delete;

    bool join(Player& player);  // returns true if and only if the join succeed
    Player& getLeader();
    int getNumPlayers() const;
    int getId() const;

    // Representation for Debugging
    friend std::ostream& operator<<(std::ostream& os, const Session& sess);
};

#endif  // SESSION_H