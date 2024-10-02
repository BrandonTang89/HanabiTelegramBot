#include "deck.h" 
#include <cassert>
#include <boost/log/trivial.hpp>

Deck::Deck(){
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
    std::random_shuffle(cards.begin(), cards.end());
}

Card Deck::draw() {
    assert(nextCardIndex < totalCards);
    Card c = cards[nextCardIndex++];
    // BOOST_LOG_TRIVIAL(trace) << "Drew card: " << c;
    return c;
}

