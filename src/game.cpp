#include <iostream>

#include "game.h"
#include "pch.h"
#include "card.h"
#include "deck.h"
#include "helper.h"
#include "player.h"
#include "session.h"
#include "sockets.h"

Game::Game(Session&& session)
    : players{std::move(session.players)},
      numPlayers{static_cast<int>(players.size())},
      sessionId{session.getId()},
      table(Card::Colours::numColours, 0),
      cardsPerHand{numPlayers <= 3 ? 5 : 4} {}

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
                    send_(players[i].socket, " " + std::to_string(k) + ": " + hands[i][k].hiddenRepr() + "\n");
                }
            } else {
                send_(players[i].socket, "= Player " + std::to_string(j) + "'s hand (" + players[j].name + ") :\n");
                for (int k = 0; k < 5; k++) {
                    send_(players[i].socket, " " + std::to_string(k) + ": " + hands[j][k].fullRepr() + "\n");
                }
            }
        }
    }
}

void Game::displayTokens() {
    for (int i = 0; i < numPlayers; i++) {
        send_(players[i].socket, "== Tokens:\n");
        send_(players[i].socket, "Blue: " + std::to_string(numBlueTokens) + "\n");
        send_(players[i].socket, "Black: " + std::to_string(numBlackFuseTokens) + "\n");
    }
}

void Game::display() {
    displayTokens();
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

    // main game loop
    int curPlayer = 0;
    int totalPts = 0;
    try {
        while (true) {
            // curPlayer's turn
            turn(curPlayer);
            curPlayer = (curPlayer + 1) % numPlayers;

            totalPts = 0;
            for (int i = 0; i < Card::Colours::numColours; i++) totalPts += table[i];
            if (totalPts == 25 || numBlackFuseTokens == 0) break;
        }
    } catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Error in game loop: " << e.what();
        broadcast(players, "An error occurred in the game loop! likely that someone left :(\n");
        broadcast(players, "Ending Game...\n");
        return;
    }

    broadcast(players, "Game Over!\n");
    if (totalPts == 25) {
        broadcast(players, "Congratulations! You have successfully completed the fireworks!\n");
    } else {
        broadcast(players, "You have run out of black fuse tokens!\n");
        broadcast(players, "Final Score: " + std::to_string(totalPts) + "\n");
    }
}

void Game::turn(int playerIndex) {
    display();
    Player& player = players[playerIndex];
    send_(player.socket, "It's your turn!\n");
    for (int i = 0; i < numPlayers; i++) {
        if (i != playerIndex) {
            send_(players[i].socket, "It's player " + std::to_string(playerIndex) + "'s turn! (" + players[playerIndex].name + ")\n");
        }
    }

    while (true) {
        bool turnDone = selectAction(playerIndex);
        if (turnDone) break;
    }
}

bool Game::selectAction(int playerIndex) {
    Player& player = players[playerIndex];
    send_(player.socket, "Select an action: \n");
    send_(player.socket, "0. Play a card\n");
    send_(player.socket, "1. Discard a card\n");
    if (numBlueTokens > 0) {
        send_(player.socket, "2. Give a hint\n");
    }

    int action = requestInt(0, (numBlueTokens > 0 ? 2 : 1), "Invalid action. Please try again.\n", player);
    switch (action) {
        case 0:
            return Game::playCard(playerIndex);
            break;
        case 1:
            return Game::discardCard(playerIndex);
            break;
        case 2:
            return Game::giveHint(playerIndex);
            break;
        default:
            assert(false);
            break;
    }
}

bool Game::playCard(int playerIndex) {
    Player& player = players[playerIndex];
    send_(player.socket, "Select a card to play or enter -1 to change action: \n");

    int cardIndex = requestInt(-1, static_cast<int>(hands[playerIndex].size() - 1), "invalid card selected!\n", player);
    if (cardIndex == -1) return false;  // change action

    Card card = hands[playerIndex][cardIndex];
    int expectedCardNumber = table[static_cast<int>(card.colour)] + 1;  // 1 more than the card on the table
    if (expectedCardNumber == card.number) {
        table[static_cast<int>(card.colour)] = card.number;
        hands[playerIndex][cardIndex] = deck.draw();

        if (card.number == 5 && numBlueTokens < totalBlueTokens) {
            numBlueTokens++;
        }

        broadcast(players, "Player " + std::to_string(playerIndex) + " played " + card.fullRepr() + " successfully!\n");
    } else {
        numBlackFuseTokens--;
        discardPile.push_back(card);

        broadcast(players, "Player " + std::to_string(playerIndex) + " played " + card.fullRepr() + " incorrectly!\n");
        broadcast(players, "Black Fuse Token used! Remaining: " + std::to_string(numBlackFuseTokens) + "\n");
    }
    return true;
}

bool Game::discardCard(int playerIndex) {
    Player& player = players[playerIndex];
    send_(player.socket, "Select a card to discard or enter -1 to change action: \n");

    int cardIndex = requestInt(-1, static_cast<int>(hands[playerIndex].size() - 1), "invalid card selected!\n", player);
    if (cardIndex == -1) return false;  // change action

    Card card = hands[playerIndex][cardIndex];

    broadcast(players, "Player " + std::to_string(playerIndex) + " discarded " + card.fullRepr() + "\n");

    numBlueTokens++;
    discardPile.push_back(card);
    hands[playerIndex][cardIndex] = deck.draw();

    return true;
}

bool Game::giveHint(int playerIndex) {
    Player& player = players[playerIndex];
    send_(player.socket, "Select a player to give hint to, or -1 to go back: \n");
    std::optional<int> hinteeIndexO = std::nullopt;
    while (!hinteeIndexO.has_value()) {
        hinteeIndexO = requestInt(-1, numPlayers - 1, "Invalid player selected!\n", player);
        if (hinteeIndexO == playerIndex) {
            hinteeIndexO = std::nullopt;
            send_(player.socket, "You cannot give a hint to yourself!\n");
        }
    }
    if (hinteeIndexO.value() == -1) return false;  // change action

    send_(player.socket, "Select which information to give, or -1 to pick another action:\n");
    send_(player.socket, "0. Colour\n");
    send_(player.socket, "1. Number\n");
    int hintType = requestInt(-1, 1, "Invalid hint type selected!\n", player);

    if (hintType == -1) return false;  // change action

    // Give Colour Hint
    if (hintType == 0) {
        send_(player.socket, "Select a colour to hint or -1 to pick another action: \n");
        for (int i = 0; i < Card::Colours::numColours; i++) {
            send_(player.socket, std::to_string(i) + ". " + Card::getColourString(static_cast<Card::Colours>(i)) + "\n");
        }

        while (true) {
            int colourIndex = requestInt(-1, Card::Colours::numColours - 1, "Invalid colour selected!\n", player);
            if (colourIndex == -1) return false;
            Card::Colours colour = static_cast<Card::Colours>(colourIndex);

            std::vector<std::reference_wrapper<Card>> targettedCards;
            for (Card& card : hands[hinteeIndexO.value()]) {
                if (card.colour == colour) {
                    targettedCards.push_back(card);
                }
            }

            if (targettedCards.empty()) {
                send_(player.socket, "Player " + std::to_string(hinteeIndexO.value()) + " has no cards of colour " + Card::getColourString(colour) + ", give another hint\n");
            } else {
                broadcast(players, "Player " + std::to_string(playerIndex) + " gave a hint to Player " + std::to_string(hinteeIndexO.value()) + " about the colour " + Card::getColourString(colour) + "\n");
                numBlueTokens--;
                for (Card& card : targettedCards) {
                    card.colourRevealed = true;
                }
                break;
            }
        }
        return true;
    }
    // Give number hint
    else {
        send_(player.socket, "Select a number to hint or -1 to pick another action: \n");
        for (int i = 1; i <= 5; i++) {
            send_(player.socket, std::to_string(i) + "\n");
        }

        while (true) {
            int number = requestInt(-1, 5, "Invalid number selected!\n", player);
            if (number == 0) {
                send_(player.socket, "Number cannot be 0!\n");
                continue;
            }
            if (number == -1) return false;

            std::vector<std::reference_wrapper<Card>> targettedCards;
            for (Card& card : hands[hinteeIndexO.value()]) {
                if (card.number == number) {
                    targettedCards.push_back(card);
                }
            }

            if (targettedCards.empty()) {
                send_(player.socket, "Player " + std::to_string(hinteeIndexO.value()) + " has no cards of number " + std::to_string(number) + ", give another hint\n");
            } else {
                broadcast(players, "Player " + std::to_string(playerIndex) + " gave a hint to Player " + std::to_string(hinteeIndexO.value()) + " about the number " + std::to_string(number) + "\n");
                numBlueTokens--;
                for (Card& card : targettedCards) {
                    card.numberRevealed = true;
                }
                break;
            }
        }
        return true;
    }

    return true;
}

std::ostream& operator<<(std::ostream& os, const Game& game) {
    os << "Game{ID: " << game.sessionId << "}";
    return os;
}
