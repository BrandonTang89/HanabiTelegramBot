#ifndef CARD_H
#define CARD_H
#include <iostream>
#include <string>

using std::string;
class Card {
public:
    enum Colours { RED, BLUE, GREEN, YELLOW, WHITE, num_colours };
    
    int number{}; // 1-5
    Colours colour{};
    bool colourRevealed{false};
    bool numberRevealed{false};

    Card(int number_, Colours colour_) : number(number_), colour(colour_) {}
    
    string fullRepr() const;
    string hiddenRepr() const;


    friend std::ostream& operator<<(std::ostream& os, const Card& card);

};

#endif // CARD_H