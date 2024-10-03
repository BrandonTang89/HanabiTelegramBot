#include <string>
#include <iostream>
#include <cassert>
#include "card.h"

using std::string;

string Card::getColourString(Card::Colours colour) {
    switch (colour) {
        case Card::RED:
            return "Red   ";
        case Card::BLUE:
            return "Blue  ";
        case Card::GREEN:
            return "Green ";
        case Card::YELLOW:
            return "Yellow";
        case Card::WHITE:
            return "White ";
        case Card::EMPTY:
            return "      ";
        default:
            assert(false); 
            return "???";
    }
}


string Card::fullRepr() const{
    string colourStr = getColourString(colour);
    string numberStr = std::to_string(number);
    
    if (colourRevealed && numberRevealed) {
        return "Card([" + colourStr + ", " + numberStr + "])";
    } else if (colourRevealed) {
        return "Card([" + colourStr + "], ?" + numberStr + "?)";
    } else if (numberRevealed) {
        return "Card(?" + numberStr + "?, [" + colourStr + "])";
    } else {
        return "Card(?" + numberStr + ", " + colourStr + "?)";
    }
}

string Card::hiddenRepr() const {
    string colourStr = getColourString(colour);
    string numberStr = std::to_string(number);
    
    if (colourRevealed && numberRevealed) {
        return "Card([" + colourStr + ", " + numberStr + "])";
    } else if (colourRevealed) {
        return "Card([" + colourStr + "], \?\?)";
    } else if (numberRevealed) {
        return "Card(\?\?, [" + colourStr + "])";
    } else {
        return "Card(\?\?, \?\?)";
    }
}

std::ostream& operator<<(std::ostream& os, const Card& card) {
    os << card.fullRepr();
    return os;
}