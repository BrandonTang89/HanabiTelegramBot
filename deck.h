#ifndef DECK_H
#define DECK_H

#include <vector>
#include "card.h"

class Deck {  // a vector of cards, allows you to draw cards and shuffle the deck
   private:
    std::vector<Card> cards;
    int nextCardIndex{0};
    int totalCards{};
   public:
    Deck();
    void shuffle();
    Card draw();
    // Other member functions and variables
};

#endif  // DECK_H