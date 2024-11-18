#ifndef CLIENT_ENTRY_H
#define CLIENT_ENTRY_H

#include <queue>
#include <string>
#include "telegram_client_coroutine.hpp"
#include <tgbot/tgbot.h>

using ChatIdType = std::int64_t;
ClientCoroutine clientEntry(ChatIdType chatId, std::queue<std::string>& messageQueue, TgBot::Bot& bot);


#endif // CLIENT_ENTRY_H