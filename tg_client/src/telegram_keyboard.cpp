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
void createOneColumnKeyboard(const std::vector<std::string>& buttonStrings, TgBot::ReplyKeyboardMarkup::Ptr& kb) {
    for (size_t i = 0; i < buttonStrings.size(); ++i) {
        std::vector<TgBot::KeyboardButton::Ptr> row;
        TgBot::KeyboardButton::Ptr button(createKeyboardButton(buttonStrings[i]));
        row.push_back(button);
        kb->keyboard.push_back(row);
    }
}