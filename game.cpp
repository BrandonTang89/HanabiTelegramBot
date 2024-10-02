#include "game.h"
#include <boost/log/trivial.hpp>


Game::Game(Session&& session)
    : players{std::move(session.players)}, numPlayers{static_cast<int>(players.size())}, sessionId{session.getId()} {}

void Game::displayHands() {
    for (int i = 0; i < numPlayers; i++) {
        for (int j = 0; j < numPlayers; j++) {
            assert(static_cast<int>(hands[i].size()) == cardsPerHand);
            if (i == j) {
                send_(players[i].socket, "Your hand:\n");
                for (int k = 0; k < 5; k++) {
                    send_(players[i].socket, hands[i][k].hiddenRepr() + "\n");
                }
            } else {
                send_(players[i].socket, "Player " + std::to_string(j) + "'s (" + players[j].name + ")  hand:\n");
                for (int k = 0; k < 5; k++) {
                    send_(players[i].socket, hands[j][k].fullRepr() + "\n");
                }
            }
        }
    }
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
    
    BOOST_LOG_TRIVIAL(debug) << "Dealt cards to players";
    displayHands();
}