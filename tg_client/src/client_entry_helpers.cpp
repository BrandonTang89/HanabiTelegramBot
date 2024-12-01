#include "client_entry_helpers.h"

#include "client_entry.h"
#include "pch.h"
#include "proto_files.h"
#include "sockets.h"
#include "telegram_client_coroutine.hpp"
#include "telegram_client_pch.h"

using namespace Ack;
using namespace boost::asio;
void subscribeToInfo(const Client& client) {
    BOOST_LOG_TRIVIAL(trace) << "registered to info " << client.chatId;
    uint32_t* lengthPtr = new uint32_t;  // we need to use a raw pointer here to avoid lifetime issues
    async_read(client.socket, boost::asio::buffer(lengthPtr, sizeof(*lengthPtr)), [lengthPtr, client](const boost::system::error_code& ec, std::size_t bytes_transferred) {
        auto& [chatId, socket, _, __, bot] = client;
        BOOST_LOG_TRIVIAL(info) << "Received " << bytes_transferred << " bytes";
        if (ec) {
            BOOST_LOG_TRIVIAL(error) << "Error in async_read_some: " << ec.message();
            return;
        }
        // BOOST_LOG_TRIVIAL(trace) << "Parsing length...";
        const uint32_t length = ntohl(*lengthPtr);
        delete lengthPtr;  // free the memory
        std::vector<char> buffer(length);
        boost::asio::read(socket, boost::asio::buffer(buffer.data(), buffer.size()));
        // BOOST_LOG_TRIVIAL(trace) << "Received info message!";
        InfoMessage infoMsg;
        infoMsg.ParseFromString(string(buffer.data(), buffer.size()));

        if (infoMsg.has_signal() && infoMsg.signal() == INFOSIGNAL_BREAK) {
            client.clientEvent.set();
            return;
        }

        bot.getApi().sendMessage(chatId, infoMsg.message());
        subscribeToInfo(client);
    });
}

void replyBytes(const Client& client, const std::string& serialisedMsg) {
    sendBytes(client.socket, serialisedMsg);
    subscribeToInfo(client);
}

std::optional<int> joinRandomSession(Client client) {  // try to join random session, returns the sessionId if successful
    auto& [chatId, socket, _, __, bot] = client;
    JoinRandomSessionAck joinRandomSessionAck;
    const std::string serialisedJoinRandomSessionAck = readBytes(socket);
    joinRandomSessionAck.ParseFromString(serialisedJoinRandomSessionAck);
    bot.getApi().sendMessage(chatId, joinRandomSessionAck.message());

    if (joinRandomSessionAck.session_type() == JOIN_RANDOM_SESSION_NEW_SESSION) {
        return std::nullopt;
    } else {
        std::optional<int> sessionId = joinRandomSessionAck.session_id();
        bot.getApi().sendMessage(chatId, "Your session ID is: " + std::to_string(sessionId.value()));
        subscribeToInfo(client);
        return sessionId;
    }
}

std::optional<int> joinSpecificSession(Client client) {
    auto& [chatId, socket, _, __, bot] = client;
    JoinSessionAck joinSessionAck;
    const std::string serialisedJoinSessionAck = readBytes(socket);
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

    subscribeToInfo(client);

    return sessionId;
}

std::optional<int> createSession(Client client) {
    auto& [chatId, socket, _, __, bot] = client;
    CreateSessionAck createSessionAck;
    BOOST_LOG_TRIVIAL(info) << "Waiting for server response...";
    const std::string serialisedCreateSessionAck = readBytes(socket);
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