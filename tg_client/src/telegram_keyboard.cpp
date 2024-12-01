#include "telegram_keyboard.h"
#include "telegram_client_pch.h"

TgBot::KeyboardButton::Ptr createKeyboardButton(const std::string& text) {
    TgBot::KeyboardButton::Ptr button(new TgBot::KeyboardButton);
    button->text = text;
    button->requestContact = false;
    button->requestLocation = false;
    button->requestPoll = nullptr;
    button->requestChat = nullptr;

    return button;
}
TgBot::ReplyKeyboardMarkup::Ptr createOneColumnKeyboard(const std::vector<std::string>& buttonStrings) {
    const TgBot::ReplyKeyboardMarkup::Ptr kb(new TgBot::ReplyKeyboardMarkup);
    for (const auto& buttonString : buttonStrings) {
        std::vector<TgBot::KeyboardButton::Ptr> row;
        TgBot::KeyboardButton::Ptr button(createKeyboardButton(buttonString));
        row.push_back(button);
        kb->keyboard.push_back(row);
    }
    return kb;
}