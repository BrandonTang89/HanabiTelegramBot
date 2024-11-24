#pragma once
#include <vector>
#include <random>
struct Card; // Forward declaration

class Deck {  // a vector of cards, allows you to draw cards and shuffle the deck
   private:
    std::vector<Card> cards;
    int nextCardIndex{0};
    int totalCards{};
    std::mt19937_64 mt{}; // Mersenne Twister pseudo-random number generator seeded with random device
   public:
    Deck();
    void shuffle();
    Card draw();

    // Other member functions and variables
};