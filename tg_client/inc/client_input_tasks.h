#pragma once

#include "proto_files.h"
#include "telegram_client_coroutine.hpp"
#include "telegram_client_pch.h"
#include "telegram_keyboard.h"
using namespace boost::asio;
using ip::tcp;
using namespace Ack;

Task<> welcomeTask(ChatIdType chatId, const TgBot::Bot& bot);
Task<std::string> getNameTask(ChatIdType chatId, const TgBot::Bot& bot, MessageQueue<TgMsg>& msgQueue);
Task<ClientOperation> getOperationTask(ChatIdType chatId, const TgBot::Bot& bot, MessageQueue<TgMsg>& msgQueue);
Task<int> getSpecificSessionTask(ChatIdType chatId, const TgBot::Bot& bot, MessageQueue<TgMsg>& msgQueue);
Task<> waitUntilStartCommand(ChatIdType chatId, const TgBot::Bot& bot, MessageQueue<TgMsg>& msgQueue);
Task<> leaderTask(tcp::socket& socket, ChatIdType& chatId, TgBot::Bot& bot, MessageQueue<TgMsg>& msgQueue, const std::optional<int> sessionId);