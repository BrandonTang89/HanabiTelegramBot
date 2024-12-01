#include "client_entry_helpers.h"

#include "client_entry.h"
#include "pch.h"
#include "proto_files.h"
#include "sockets.h"
#include "telegram_client_coroutine.hpp"
#include "telegram_client_pch.h"

using namespace Ack;
using namespace boost::asio;
void subscribeToInfo(tcp::socket& socket, ChatIdType& chatId, TgBot::Bot& bot) {
    BOOST_LOG_TRIVIAL(trace) << "registered to info " << chatId;
    uint32_t* lengthPtr = new uint32_t;  // we need to use a raw pointer here to avoid lifetime issues
    async_read(socket, boost::asio::buffer(lengthPtr, sizeof(*lengthPtr)), [&, lengthPtr](const boost::system::error_code& ec, std::size_t bytes_transferred) {
        BOOST_LOG_TRIVIAL(info) << "Received " << bytes_transferred << " bytes";
        if (ec) {
            BOOST_LOG_TRIVIAL(error) << "Error in async_read_some: " << ec.message();
            return;
        }

        // BOOST_LOG_TRIVIAL(trace) << "Parsing length...";
        uint32_t length = ntohl(*lengthPtr);
        delete lengthPtr;  // free the memory
        std::vector<char> buffer(length);
        boost::asio::read(socket, boost::asio::buffer(buffer.data(), buffer.size()));
        // BOOST_LOG_TRIVIAL(trace) << "Received info message!";
        InfoMessage infoMsg;
        infoMsg.ParseFromString(string(buffer.data(), buffer.size()));
        bot.getApi().sendMessage(chatId, infoMsg.message());

        subscribeToInfo(socket, chatId, bot);
    });
}

std::optional<int> joinRandomSession(tcp::socket& socket, ChatIdType& chatId, TgBot::Bot& bot) {  // try to join random session, returns the sessionId if successful
    JoinRandomSessionAck joinRandomSessionAck;
    std::string serialisedJoinRandomSessionAck = readBytes(socket);
    joinRandomSessionAck.ParseFromString(serialisedJoinRandomSessionAck);
    bot.getApi().sendMessage(chatId, joinRandomSessionAck.message());

    if (joinRandomSessionAck.session_type() == JOIN_RANDOM_SESSION_NEW_SESSION) {
        return std::nullopt;
    } else {
        std::optional<int> sessionId = joinRandomSessionAck.session_id();
        bot.getApi().sendMessage(chatId, "Your session ID is: " + std::to_string(sessionId.value()));
        subscribeToInfo(socket, chatId, bot);
        return sessionId;
    }
}

std::optional<int> joinSpecificSession(tcp::socket& socket, ChatIdType& chatId, TgBot::Bot& bot) {
    JoinSessionAck joinSessionAck;
    std::string serialisedJoinSessionAck = readBytes(socket);
    joinSessionAck.ParseFromString(serialisedJoinSessionAck);
    bot.getApi().sendMessage(chatId, joinSessionAck.message());

    std::optional<int> sessionId;
    if (joinSessionAck.status() == AckStatus::ACK_SUCCEED) {
        sessionId = joinSessionAck.session_id();
    } else {
        BOOST_LOG_TRIVIAL(info) << "User failed to join session!";
        return std::nullopt;
    }
    bot.getApi().sendMessage(chatId, "Your session ID is: " + std::to_string(sessionId.value()));

    subscribeToInfo(socket, chatId, bot);

    return sessionId;
}

std::optional<int> createSession(tcp::socket& socket, ChatIdType& chatId, const TgBot::Bot& bot) {
    CreateSessionAck createSessionAck;
    BOOST_LOG_TRIVIAL(info) << "Waiting for server response...";
    std::string serialisedCreateSessionAck = readBytes(socket);
    createSessionAck.ParseFromString(serialisedCreateSessionAck);
    bot.getApi().sendMessage(chatId, createSessionAck.message());

    if (createSessionAck.status() == AckStatus::ACK_SUCCEED) {
        std::optional<int> sessionId = createSessionAck.session_id();
        bot.getApi().sendMessage(chatId, "You are the session leader.");
        bot.getApi().sendMessage(chatId, "Your session ID is: " + std::to_string(sessionId.value()));
        return sessionId;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Failed to create session!";
        return std::nullopt;
    }
    // Wait until start then start
}