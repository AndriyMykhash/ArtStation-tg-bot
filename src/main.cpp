// #include <iostream>
// #include <string>
// #include <fstream>
// #include <vector>
// #include <csignal>
// #include <cstdio>
// #include <cstdlib>
// #include <exception>
// #include <string>
// #include <vector>
// #include <algorithm>

#include <tgbot/tgbot.h>
#include <boost/algorithm/string.hpp>

#include "JsonParser.h"
#include "UrlGrabber.h"
#include "JsonReceiver.h"

using namespace TgBot;

void createKeyboard(const std::map<std::string, std::string> &buttonStringsMap,
         InlineKeyboardMarkup::Ptr &keyboard, size_t column_size, size_t max_word_len )
{
    std::vector<InlineKeyboardButton::Ptr> row;

    size_t i = 0;
    for (auto it = buttonStringsMap.begin(); it != buttonStringsMap.end(); it++)
    {
        std::string button_text = it->first;
        std::replace(button_text.begin(), button_text.end(), '_', ' ');

        std::string button_com = boost::algorithm::to_lower_copy(it->first);
        std::replace(button_com.begin(), button_com.end(), '-', '_');
        std::replace(button_com.begin(), button_com.end(), '&', '_');
        std::replace(button_com.begin(), button_com.end(), ' ', '_');

        InlineKeyboardButton::Ptr checkButton(new InlineKeyboardButton);
        checkButton->text = button_text;
        checkButton->callbackData = button_com;

        /// TODO: change keyboard build algorithm
        if (((i != 0) && (i % column_size) == 0) && row.size() != 0)
        {
            keyboard->inlineKeyboard.push_back(row);
            row.clear();
        }
        
        row.push_back(checkButton);
        
        if (it->first.size() > max_word_len && row.size() != 0)
        {
            keyboard->inlineKeyboard.push_back(row);
            row.clear();
        }
        i++;
    }
}

/// TODO: add (int argc, char*[] argv) for json path
int main(int argc, char *argv[])
{
    curl_global_init(CURL_GLOBAL_ALL);
    JsonParser p("/home/andrii/temp_project/ArtStation-tg-bot/bot_configs.json");
    p.parse();

    Bot bot(p.getToken());

    std::map<std::string, std::string> buttonStringsMap = p.getNameAndUrls();

    InlineKeyboardMarkup::Ptr keyboard(new InlineKeyboardMarkup);
    createKeyboard(buttonStringsMap, keyboard, 3, 14);

    /// error: std::bad_alloc - if wrap this 'for' in function
    for (auto it = buttonStringsMap.begin(); it != buttonStringsMap.end(); it++)
    {
        std::string com = boost::algorithm::to_lower_copy(it->first);
        std::replace(com.begin(), com.end(), ' ', '_');
        std::replace(com.begin(), com.end(), '-', '_');
        std::replace(com.begin(), com.end(), '&', '_');

        // bot.getEvents().onCommand(com, [&bot, it](Message::Ptr message)
        //                           { bot.getApi().sendMessage(message->chat->id, it->second); });

        bot.getEvents().onCallbackQuery([&bot, &keyboard, com, it, &p](CallbackQuery::Ptr query)
                                        {
            if (StringTools::startsWith(query->data, com)) {
                bot.getApi().sendChatAction(query->message->chat->id,"upload_photo");
                JsonReceiver rec;
                std::string str;
                rec(it->second, str);
                UrlGrabber grab;
                grab.setParseStr(str);
                grab.setRegexExpression(p.getHashImageRegex());
                auto uu = grab.getUrls();

                for(auto iter : uu)
                {
                    bot.getApi().sendMessage(query->message->chat->id, iter, false, 0,0,"HTML");
                }
                /*
                ///TODO: add sending photoes utility
                ///TODO: Add Thread poll that created once and only receive url, return
                
                    struct art
                    {
                        photo_url,
                        artist,
                        number of artist work
                    }
                

                // bot.getApi().sendMessage(query->message->chat->id, msg, true, 0,0,"HTML");
                ///TODO: Remove second append
                */
                std::string msg = it->first;
                std::replace( msg.begin(), msg.end(), '_', ' ');
                ///TODO: Add link to artist and Name 
                msg.append(", <a href=\"").append(it->second).append("\">Artist</a> /menu\n");
                bot.getApi().sendMessage(query->message->chat->id, msg, true, 0,0,"HTML");

            } });
    }
    
    ///TODO: write bot description
    bot.getEvents().onCommand("start", [&bot, &keyboard](Message::Ptr message)
                              { bot.getApi().sendMessage(message->chat->id, "Choose theme:", false, 0, keyboard); });
    bot.getEvents().onCommand("menu", [&bot, &keyboard](Message::Ptr message)
                              { bot.getApi().sendMessage(message->chat->id, "Choose theme:", false, 0, keyboard); });

    signal(SIGINT, [](int s)
           {
        printf("SIGINT got\n");
        exit(0); 
        curl_global_cleanup();
        });

    try
    {
        bot.getApi().deleteWebhook();

        TgLongPoll longPoll(bot);
        while (true)
        {
            longPoll.start();
        }
    }
    catch (std::exception &e)
    {
        printf("error: %s\n", e.what());
    }

    return 0;
}