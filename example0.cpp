#include "sleepy_discord/sleepy_discord.h"
#include "date.h"
#include <vector>
#include <string>
#include <cpr/cpr.h>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/rapidjson.h"
#include <iostream>
#include "rapidjson/filereadstream.h"
#include <map>
#include <rapidjson/istreamwrapper.h>
#include <fstream>
#include <cstdio>
#include <chrono>
#include <streambuf>
#include <sstream>
using namespace std;
using namespace date;
using namespace rapidjson;

int random(int a, int b)
{
    srand(time(NULL));
    if (a > 0) return a + rand() % (b - a);
    else return a + rand() % (abs(a) + b);
}

class MyClientClass : public SleepyDiscord::WebsocketppDiscordClient {

public:

    using SleepyDiscord::WebsocketppDiscordClient::WebsocketppDiscordClient;

    map<string, string> groupID{ {"932101", "42974d5d-ffca-11eb-8169-005056bc249c"},
                                  {"932102", "42974d59-ffca-11eb-8169-005056bc249c"},
                                  {"932103", "42974d55-ffca-11eb-8169-005056bc249c"} };


    class wrongUserInput : public exception {};
    void onChannel(SleepyDiscord::Channel channel) {
        if (channel.name.find("timetable") != std::string::npos) {
            string helloMessage("Хей! Похоже, что этот канал был создан для того, чтобы я смог отправить сюда расписание! Если это так, то отправьте сюда команду !timetable или !расписание"); 
            WebsocketppDiscordClient::sendMessage(channel.ID, helloMessage);
        }
        else return;

    }
    void onMessage(SleepyDiscord::Message message) override {

        if (message.startsWith("!timetable-hel") || message.startsWith("!расписание-помощ")) {

            // sendMessage(message.channelID, "Привет. Этот бот создан, чтобы отправлять расписание из tsu-intime в ваш discord-сервер.");
             //sendMessage(message.channelID, "Сделайте следующее: \n1) Создайте текстовый канал(рекомендуемое название: <номер-группы>-расписание(без<>). \
                                                 Пример: 932102-timetable).\n2) Отправьте команду !timetable или !расписание в этот канал");
            WebsocketppDiscordClient::sendMessage(message.channelID, "Hello. This bot is created to send timetable from tsu-intime to your discord server.");
            WebsocketppDiscordClient::sendMessage(message.channelID, "Follow these steps: \n1) Create text channel(recommended name: <group-number>-timetable (without <>). \
                                            For example: 932102-timetable).\n2) Send !timetable command to this channel");
            return;
        }
        else if (message.startsWith("!расписание") || message.startsWith("!timetable")) {
            FILE* fileForReadingCache932102 = fopen("C:\\timetableCache\\timetableCache932102.json", "rb");
            char bufferForCache932102[100000];
            FileReadStream streamForReadingCache932102(fileForReadingCache932102, bufferForCache932102, sizeof(bufferForCache932102));
           

            Document currentTimetableFor932102;
            currentTimetableFor932102.ParseStream(streamForReadingCache932102);

            FILE* fileForClassTypes = fopen("C:\\timetableCache\\classTypesJsonFile.json", "rb");
            char bufferForClassTypes[100000];
            FileReadStream streamForReadingClassTypesFile(fileForClassTypes, bufferForClassTypes, sizeof(bufferForClassTypes));
            Document classTypeString;
            classTypeString.ParseStream(streamForReadingClassTypesFile);
            
            FILE* fileForWeekdays = fopen("C:\\timetableCache\\weekdaysJsonFile.json", "rb");
            char bufferForWeekdays[100000];
            FileReadStream streamForWeekdays(fileForWeekdays, bufferForWeekdays, sizeof(bufferForWeekdays));
            Document weekdayString;
            weekdayString.ParseStream(streamForWeekdays);

            string stringBuffer;
            for (Value::ConstMemberIterator  weekDaysItr = currentTimetableFor932102["data"].MemberBegin();
                weekDaysItr != currentTimetableFor932102["data"].MemberEnd(); ++weekDaysItr) {
                string currentDayStringBuffer(weekDaysItr->name.GetString());
                auto currentDay = stoi(currentDayStringBuffer);
                auto sysTime = chrono::system_clock::from_time_t(currentDay);
                auto currentDayFormat = year_month_day(floor<days>(sysTime));
                auto weekdayOfCurrentDay = weekday(currentDayFormat); 
                ostringstream sendingDateStream, weekdayStringStream; 
                weekdayStringStream << weekdayOfCurrentDay;
                sendingDateStream << currentDayFormat;
                
                stringBuffer += "*"+sendingDateStream.str()+"*" + ". " + "***" + string(weekdayString[weekdayStringStream.str().c_str()].GetString()) + "***: \n";
              
                if (!weekDaysItr->value["schedule"].Empty()) {
                    for (Value::ConstValueIterator scheduleItr = weekDaysItr->value["schedule"].Begin();
                        scheduleItr != weekDaysItr->value["schedule"].End(); ++scheduleItr) {
                        string classTitle(((*scheduleItr)["title"].GetString())),
                            startingTime(((*scheduleItr)["starts"].GetString())),
                            endingTime(((*scheduleItr)["ends"].GetString())),
                            teacherName(((*scheduleItr)["teacher"]["name"].GetString())),
                            auditoryName(((*scheduleItr)["auditory"]["name"].GetString())),
                            classType(classTypeString[((*scheduleItr)["type"].GetString())].GetString());
                          
                       
                          
                        stringBuffer += "```css\n[" + startingTime + "] - [" + endingTime + "]" + "; " + auditoryName + "\n" + classTitle +
                             " (" + classType  +") \n"+ teacherName +"\n```";
                    }
                }
                else stringBuffer += "```css\n[NO CLASSES THIS DAY =)]\n```"; 
                
                WebsocketppDiscordClient::sendMessage(message.channelID, stringBuffer + "\n");
                stringBuffer.clear();
            }
            fclose(fileForReadingCache932102);
            fclose(fileForClassTypes);
            return;
        }
    }
};
    int main() {




        MyClientClass client("ODkxOTQzODA4NTkyNDA4NTk2.YVFtZw.Kl-iZdleIwacgvxmEA1Y0yBz4Zc", SleepyDiscord::USER_CONTROLED_THREADS);
        client.run();
    }
