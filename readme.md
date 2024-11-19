# Hanabi Telegram Bot
This repo aims to allow one to play the [Hanabi Game](https://boardgamegeek.com/boardgame/98778/hanabi) on a [telegram bot](https://core.telegram.org/bots/api). 

There are 2 parts to this process
- A server that runs the actual game (the Game Server)
- A process that runs the telegram bot (the Telegram Client)

The point of this dual server approach is to separate the logic for running the game from the logic for handling the telegram bot, potentially allowing for cross platform games in the future.

Between the 2 different servers, we will use google's [Protocol Buffers](https://protobuf.dev/) to communicate.

We will use [tgbot-cpp](https://github.com/reo7sp/tgbot-cpp) as the telegram bot framework.

## Dependencies
We require a compiler that supports C++23 (or at least C++20 for coroutines, but I did not try building with C++20). This project has only been tested with GNU GCC version 13.2.0.

We need to install [tgbot-cpp](https://github.com/reo7sp/tgbot-cpp) as well as the [C++ boost libary](https://www.boost.org/). 

To speed up compilation, we use [ccache](https://ccache.dev/) and [Ninja](https://github.com/ninja-build/ninja).
```sh
sudo apt-get install libboost-all-dev ninja-build ccache
```

## The Game Server
We spawn a new thread for each new client to allow the main thread to continue waiting on new connections. Threads that correspond to people joining lobbies will drop after they join a specific lobby. This means that the number of threads running at anytime roughly corresponds to the number of loading/active games. It might be better to use coroutines to deal with managing the various games, but that is a stretch goal.

## The Telegram Client
The telegram client process acts as a middleman between individual telegram chats and the game server. Compared with the approach of spawning a new thread to handle each game in the server, I opted to use a single thread that operated several chat sessions and uses coroutines to manage the state of each chat session. This is more lightweight compared to spawning multiple threads since the work done for each chat session is quite minimal anyway. It is a future goal to utilise multithreading via a worker pool to scale this if necessary.

Each chat session will occasionally await on messages from the player. These messages will be first processed by the main thread and then sorted into message queues for individual chat sessions. We can technically get away with using slots rather than queues initially, but in a multithreaded setting the queues would be better. 

## Project Structure
- `./inc` - Contains all the header (`.h`) files.
- `./src` - Contains all the source (`.cpp`) files.
- `./proto` - Contains all the protobuf (`.proto`) files.