#pragma once
#include <tgbot/tgbot.h>
#include "telegram_client_coroutine.hpp"

typedef TgBot::Message::Ptr TgMsg;
typedef std::int64_t ChatIdType;
using namespace boost::asio::ip;

struct Client { // struct of references of the client context to pass around
  ChatIdType& chatId;
  tcp::socket& socket;
  MessageQueue<TgMsg>& msgQueue;
  SignallingEvent& clientEvent;
  TgBot::Bot& bot;
};