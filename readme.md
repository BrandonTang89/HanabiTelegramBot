# Hanabi Telegram Bot
This repo aims to allow one to play the [Hanabi Game](https://boardgamegeek.com/boardgame/98778/hanabi) on a telegram bot. 

There are 2 parts to this process
- A server that runs the actual game
- A server that is the bot

The point of this dual server approach is to separate the logic for running the game from the logic for handling the telegram bot, potentially allowing for cross platform games in the future.

Between the 2 different servers, we will use JSON to communicate.

## Project Structure

- `./inc` - Contains all the header (`.h`) files.
- `./src` - Contains all the source (`.cpp`) files.