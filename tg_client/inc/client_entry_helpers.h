#pragma once

#include "client_entry.h"
#include "client_input_tasks.h"
#include "pch.h"
#include "proto_files.h"
#include "sockets.h"
#include "telegram_client_coroutine.hpp"
#include "telegram_client_pch.h"
#include "telegram_keyboard.h"

using namespace Ack;
using namespace boost::asio;

void subscribeToInfo(tcp::socket& socket, ChatIdType& chatId, TgBot::Bot& bot);
std::optional<int> joinRandomSession(tcp::socket& socket, ChatIdType& chatId, TgBot::Bot& bot);
std::optional<int> joinSpecificSession(tcp::socket& socket, ChatIdType& chatId, TgBot::Bot& bot);
std::optional<int> createSession(tcp::socket& socket, ChatIdType& chatId, TgBot::Bot& bot);