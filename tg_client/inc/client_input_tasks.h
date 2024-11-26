#pragma once

#include "pch.h"
#include "proto_files.h"
#include "telegram_client_coroutine.hpp"
#include "telegram_client_pch.h"
#include "telegram_keyboard.h"
using namespace boost::asio;
using ip::tcp;
using namespace Ack;

Task<> welcomeTask(ChatIdType chatId, TgBot::Bot& bot);
Task<std::string> getNameTask(ChatIdType chatId, TgBot::Bot& bot, MessageQueue<TgMsg>& msgQueue);
Task<ClientOperation> getOperationTask(ChatIdType chatId, TgBot::Bot& bot, MessageQueue<TgMsg>& msgQueue);
Task<int> getSpecificSessionTask(ChatIdType chatId, TgBot::Bot& bot, MessageQueue<TgMsg>& msgQueue);
Task<> waitUntilStartCommand(ChatIdType chatId, TgBot::Bot& bot, MessageQueue<TgMsg>& msgQueue);
Task<> leaderTask(tcp::socket& socket, ChatIdType& chatId, TgBot::Bot& bot, MessageQueue<TgMsg>& msgQueue, std::optional<int> sessionId);