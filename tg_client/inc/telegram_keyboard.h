// telegram_keyboard.h
#pragma once

#include "telegram_client_pch.h"
#include <string>
#include <vector>

// Function to create a keyboard button
TgBot::KeyboardButton::Ptr createKeyboardButton(const std::string& text);

// Function to create a one-column keyboard
TgBot::ReplyKeyboardMarkup::Ptr createOneColumnKeyboard(const std::vector<std::string>& buttonStrings);
