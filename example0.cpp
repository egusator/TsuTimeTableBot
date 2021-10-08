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
#include <exception>
#include <typeinfo>
#include <stdexcept>
using namespace std;
using namespace date;
using namespace rapidjson;

int random(int a, int b)
{
    srand(time(NULL));
    if (a > 0) return a + rand() % (b - a);
    else return a + rand() % (abs(a) + b);
}

Document documentFromFile(const string filePath) {
    FILE* f = fopen(filePath.c_str(), "rb");
    char buffer[100000];
    FileReadStream streamForReadingClassTypesFile(f, buffer, sizeof(buffer));
    Document d;
    d.ParseStream(streamForReadingClassTypesFile);
    return d; 
}

Document documentFromRequest(const string urlString) {
    cpr::Response response = cpr::Get(cpr::Url{ urlString });
    Document d;
    d.Parse(response.text.c_str());
    return d;
}

class MyClientClass : public SleepyDiscord::WebsocketppDiscordClient {

public:

    using SleepyDiscord::WebsocketppDiscordClient::WebsocketppDiscordClient;

    map<string, string> groupID{ {"932101", "42974d5d-ffca-11eb-8169-005056bc249c"},
                                  {"932102", "42974d59-ffca-11eb-8169-005056bc249c"},
                                  {"932103", "42974d55-ffca-11eb-8169-005056bc249c"} };
    std::unordered_map<
        SleepyDiscord::Snowflake<SleepyDiscord::Channel>::RawType,
        SleepyDiscord::Channel
    > channels;
    class noSuchGroup :public exception {} noSuchGroupForReal;
    void onChannel(SleepyDiscord::Channel channel) {
        channels.insert({ channel.ID, channel });
        if (channel.name.find("timetable") != std::string::npos) {
            string helloMessage("It seems like this channel is made for me... If i'm write, send here !timetable"); 
            WebsocketppDiscordClient::sendMessage(channel.ID, helloMessage);
        }
        else return;
    }
    void onServer(SleepyDiscord::Server server) override { //called when server is available
        for (SleepyDiscord::Channel& channel : server.channels) {
            channels.insert({ channel.ID, channel });
        }
       
    }
    void onMessage(SleepyDiscord::Message message) override {

        if (message.startsWith("!timetable-help")) {
            WebsocketppDiscordClient::sendMessage(message.channelID, "Hello. This bot is created to **send timetable from tsu-intime to your discord server.**");
            WebsocketppDiscordClient::sendMessage(message.channelID, "Follow these steps: \n***1) Create text channel*** *(recommended name: <group-number>-timetable (without <>). For example: ***932102-timetable***)*.\n*2) Send ***!timetable*** command to this channel*");
            return;
        }
        else if (message.startsWith("!расписание") || message.startsWith("!timetable")) {
            auto channel = channels.find(message.channelID);
            string channelName(channel->second.name), groupNumber = channelName.substr(0,6);
            if (channelName.find("timetable") == std::string::npos) {
                sendMessage(message.channelID, "Please, *rename your channel correctly*(for more info do ***!timetable-help***).");
            } else {
                try {

                    Document privateRequestParameters = documentFromFile("C:\\Users\\79138\\projects\\TsuTimeTableBot\\jsonPrivateFiles\\requestParameters.json");
                    Document weekdayString = documentFromFile("C:\\Users\\79138\\projects\\TsuTimeTableBot\\jsonPrivateFiles\\weekdaysJsonFile.json");
                    Document classTypeString = documentFromFile("C:\\Users\\79138\\projects\\TsuTimeTableBot\\jsonPrivateFiles\\classTypesJsonFile.json");
                    Document groupID = documentFromFile("C:\\Users\\79138\\projects\\TsuTimeTableBot\\jsonPrivateFiles\\groupID.json");
                    if (!groupID.HasMember(groupNumber.c_str())) {
                        throw noSuchGroupForReal; 
                    } else {
                        string apiRequestUrlString = string(privateRequestParameters["1"].GetString()) + string(groupID[groupNumber.c_str()].GetString())
                            + string(privateRequestParameters["2"].GetString()) + string(privateRequestParameters["startingTime"].GetString()) +
                            string(privateRequestParameters["3"].GetString()) + string(privateRequestParameters["endingTime"].GetString());
                        Document currentTimetable = documentFromRequest(apiRequestUrlString);
                        string stringBuffer;
                        for (Value::ConstMemberIterator  weekDaysItr = currentTimetable["data"].MemberBegin();
                            weekDaysItr != currentTimetable["data"].MemberEnd(); ++weekDaysItr) {
                            string currentDayStringBuffer(weekDaysItr->name.GetString());
                            auto currentDay = stoi(currentDayStringBuffer);
                            auto sysTime = chrono::system_clock::from_time_t(currentDay);
                            auto currentDayFormat = year_month_day(floor<days>(sysTime));
                            auto weekdayOfCurrentDay = weekday(currentDayFormat);
                            ostringstream sendingDateStream, weekdayStringStream;
                            weekdayStringStream << weekdayOfCurrentDay;
                            sendingDateStream << currentDayFormat;
                            stringBuffer += "*" + sendingDateStream.str() + "*" + ". " + "***" + string(weekdayString[weekdayStringStream.str().c_str()].GetString()) + "***: \n";

                                if (!weekDaysItr->value["schedule"].Empty()) {
                                for (Value::ConstValueIterator scheduleItr = weekDaysItr->value["schedule"].Begin();
                                    scheduleItr != weekDaysItr->value["schedule"].End(); ++scheduleItr) {

                                    string classTitle(((*scheduleItr)["title"].GetString())),
                                        startingTime(((*scheduleItr)["starts"].GetString())),
                                        endingTime(((*scheduleItr)["ends"].GetString())),
                                        teacherName(((*scheduleItr)["teacher"]["name"].GetString())),
                                        auditoryName(((*scheduleItr)["auditory"]["name"].GetString())),
                                        classType(classTypeString[(*scheduleItr)["type"].GetString()].GetString());



                                    stringBuffer += "```css\n[" + startingTime + "] - [" + endingTime + "]" + "; " + auditoryName + "\n" + classTitle +
                                        " (" + classType + ") \n" + teacherName + "\n```";
                                }
                            }
                            else stringBuffer += "```css\n[NO CLASSES THIS DAY =)]\n```";
                            sendMessage(message.channelID, stringBuffer + "\n", SleepyDiscord::Sync);
                            sleep(1000);
                            stringBuffer.clear();
                        }
                    }
                }
                catch(noSuchGroup) {
                    sendMessage(message.channelID, "*I'm sorry,  i* ***haven't your group in the group list***. Please, *contact my creator* ***(discord: w1resh4rk#1676)***.");
                }
                catch (exception& e) {
                    sendMessage(message.channelID, "*Had some problems...* Please, *contact my creator* ***(discord: w1resh4rk#1676)***.");
                    cout << e.what() << endl; 
                }
                return;
            }
        }
    }
};
    int main() {
        Document privateBotData = documentFromFile("C:\\Users\\79138\\projects\\TsuTimeTableBot\\jsonPrivateFiles\\privateBotData.json" );
        MyClientClass client(privateBotData["token"].GetString(), SleepyDiscord::USER_CONTROLED_THREADS);
        client.run();
    }
