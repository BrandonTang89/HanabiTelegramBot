#pragma once
#include <queue>
#include <stack>
#include <string>
#include "telegram_client_coroutine.hpp"
#include "telegram_client_pch.h"
#include <tgbot/tgbot.h>

Task<> clientEntry(ChatIdType chatId, MessageQueue<TgMsg>& msgQueue, TgBot::Bot& bot, boost::asio::io_context& io_ctx);