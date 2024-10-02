#ifndef GAME_H
#define GAME_H
#include <vector>

#include "player.h"
#include "session.h"
#include "sockets.h"
#include "deck.h"
#include "card.h"


// Your code goes here
class Game {
   private:
    std::vector<Player> players;
    const int numPlayers;
    const int sessionId;
    const int cardsPerHand{5};
    Deck deck{};
    std::vector<std::vector<Card>> hands;

   public:
    int numBlueTokens{};
    int numBlackFuseTokens{};


    Game(Session&& session);
    void start();
    void displayHands();
};

#endif  // GAME_H