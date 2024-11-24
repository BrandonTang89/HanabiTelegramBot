#pragma once
#include <string>

using std::string;
class Card {
public:
    enum Colours { RED, BLUE, GREEN, YELLOW, WHITE, numColours, EMPTY }; // empty card for when deck runs out of cards
    static string getColourString(Colours colour);
    
    int number{}; // 1-5
    Colours colour{};
    bool colourRevealed{false};
    bool numberRevealed{false};

    Card(int number_, Colours colour_) : number(number_), colour(colour_) {}
    
    string fullRepr() const;
    string hiddenRepr() const;

    friend std::ostream& operator<<(std::ostream& os, const Card& card);

};