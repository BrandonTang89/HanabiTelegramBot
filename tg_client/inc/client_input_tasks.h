#pragma once

#include "proto_files.h"
#include "telegram_client_coroutine.hpp"
#include "telegram_client_pch.h"
#include "telegram_keyboard.h"
using namespace boost::asio;
using ip::tcp;
using namespace Ack;

Task<> welcomeTask(Client client);
Task<std::string> getNameTask(Client client);
Task<ClientOperation> getOperationTask(Client client);
Task<int> getSpecificSessionTask(Client client);
Task<> waitUntilStartCommand(Client client);
Task<> leaderTask(Client client, std::optional<int> sessionId);