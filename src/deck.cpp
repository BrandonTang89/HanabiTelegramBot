#include "../inc/deck.h" 
#include "../inc/card.h"
#include <cassert>
#include "../inc/pch.h"

Deck::Deck(){
    std::random_device rd;
    mt.seed(rd());

    for (int colNum = 0; colNum < Card::numColours; colNum++) {
        Card::Colours colour = static_cast<Card::Colours>(colNum);

        // 3 of number 1
        cards.push_back(Card(1, colour));
        cards.push_back(Card(1, colour));
        cards.push_back(Card(1, colour));

        // 2 of numbers 2-4
        for (int num = 2; num <= 4; num++) {
            cards.push_back(Card(num, colour));
            cards.push_back(Card(num, colour));
        }

        // 1 of number 5
        cards.push_back(Card(5, colour));
    }

    totalCards = static_cast<int>(cards.size());
    shuffle();
}

void Deck::shuffle() {
    std::shuffle(cards.begin(), cards.end(), mt);
}

Card Deck::draw() {
    if (nextCardIndex < totalCards) return cards[nextCardIndex++];
    Card c = Card(0, Card::Colours::EMPTY);
    c.colourRevealed = true;
    c.numberRevealed = true;
    return c;
}

