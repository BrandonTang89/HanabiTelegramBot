// telegram_keyboard.h

#ifndef TELEGRAM_KEYBOARD_H
#define TELEGRAM_KEYBOARD_H

#include "telegram_client_pch.h"
#include <string>
#include <vector>

// Function to create a keyboard button
TgBot::KeyboardButton::Ptr createKeyboardButton(const std::string& text);

// Function to create a one-column keyboard
void createOneColumnKeyboard(const std::vector<std::string>& buttonStrings, TgBot::ReplyKeyboardMarkup::Ptr& kb);

#endif // TELEGRAM_KEYBOARD_H