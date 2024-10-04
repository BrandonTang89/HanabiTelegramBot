#ifndef GAME_H
#define GAME_H
#include <vector>
#include <iostream>

#include "player.h"
#include "session.h"
#include "deck.h"
#include "card.h"


// Your code goes here
class Game {
   private:
    std::vector<Player> players;
    const int numPlayers;
    const int sessionId;
    std::vector<int> table; // table[static_cast<Colour>(i)] = the top value of the card for this colour
    const int cardsPerHand;
    static constexpr int totalBlueTokens{8};
    static constexpr int totalBlackFuseTokens{3};
    Deck deck{};
    std::vector<std::vector<Card>> hands;
    std::vector<Card> discardPile{}; // initially empty

   public:
    int numBlueTokens{};
    int numBlackFuseTokens{};


    Game(Session&& session);
    void start();
    void displayHands();
    void displayTable();
    void displayTokens();
    void display();

    void turn(int playerIndex);
    bool selectAction(int playerIndex); // returns true if the action completes successfully
    bool playCard(int playerIndex);    
    bool discardCard(int playerIndex);
    bool giveHint(int playerIndex);

    friend std::ostream& operator<<(std::ostream& os, const Game& game);
};

#endif  // GAME_H