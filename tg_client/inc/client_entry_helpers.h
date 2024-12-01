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

void subscribeToInfo(const Client& client);
void replyBytes(const Client& client, const std::string& serialisedMsg);
std::optional<int> joinRandomSession(Client client);
std::optional<int> joinSpecificSession(Client client);
std::optional<int> createSession(Client client);