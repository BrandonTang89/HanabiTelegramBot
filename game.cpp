#include <boost/log/trivial.hpp>
#include <iostream>

#include "game.h"
#include "card.h"

Game::Game(Session&& session)
    : players{std::move(session.players)},
      numPlayers{static_cast<int>(players.size())},
      sessionId{session.getId()},
      table(Card::Colours::numColours, 0) {}

void Game::displayTable() {
    for (int i = 0; i < numPlayers; i++) {
        send_(players[i].socket, "== Table:\n");
        for (int j = 0; j < Card::Colours::numColours; j++) {
            send_(players[i].socket, Card::getColourString(static_cast<Card::Colours>(j)) + ": " + std::to_string(table[j]) + "\n");
        }
    }
}

void Game::displayHands() {
    for (int i = 0; i < numPlayers; i++) {
        for (int j = 0; j < numPlayers; j++) {
            assert(static_cast<int>(hands[i].size()) == cardsPerHand);
            if (i == j) {
                send_(players[i].socket, "= Your hand:\n");
                for (int k = 0; k < 5; k++) {
                    send_(players[i].socket, " " + hands[i][k].hiddenRepr() + "\n");
                }
            } else {
                send_(players[i].socket, "= Player " + std::to_string(j) + "'s hand (" + players[j].name + ") :\n");
                for (int k = 0; k < 5; k++) {
                    send_(players[i].socket, " " + hands[j][k].fullRepr() + "\n");
                }
            }
        }
    }
}

void Game::display() {
    displayTable();
    displayHands();
}

void Game::start() {
    for (auto& player : players) {
        send_(player.socket, "Game Started!\n");
    }

    hands.assign(numPlayers, std::vector<Card>{});
    // Deal cards
    for (int i = 0; i < numPlayers; i++) {
        for (int j = 0; j < cardsPerHand; j++) {
            hands[i].push_back(deck.draw());
        }
    }
    numBlueTokens = totalBlueTokens;
    numBlackFuseTokens = totalBlackFuseTokens;

    BOOST_LOG_TRIVIAL(debug) << *this << " has been set-up!";

    display();
}

std::ostream& operator<<(std::ostream& os, const Game& game) {
    os << "Game{ID: " << game.sessionId << "}";
    return os;
}