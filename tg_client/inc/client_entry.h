#ifndef CLIENT_ENTRY_H
#define CLIENT_ENTRY_H

#include <queue>
#include <stack>
#include <string>
#include "telegram_client_coroutine.hpp"
#include "telegram_client_pch.h"
#include <tgbot/tgbot.h>

Task clientEntry(ChatIdType chatId, std::queue<TgMsg>& messageQueue, std::stack<Task>& coroutineStack, TgBot::Bot& bot);


#endif // CLIENT_ENTRY_H