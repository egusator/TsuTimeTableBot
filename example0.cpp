#include <sleepy_discord/sleepy_discord.h>
#include "date.h"
#include "iso_week.h"
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
#include "rapidjson/filewritestream.h"

#define SECONDS_IN_HOUR 3600
#define HOURS_IN_DAY 24
#define GAP 31
#define NUMBER_OF_SUNDAY 7


using namespace std;
using namespace chrono;
using namespace rapidjson;
using namespace date;

Document documentFromFile(const string filePath) {
    
    FILE* f = fopen(filePath.c_str(), "r");
    
    char buffer[100000];
    
    if (f == NULL) {
    	cout << "Не смогли открыть файл " << filePath << endl;
    	exit(1);
    }
    
    FileReadStream streamForReadingClassTypesFile(f, buffer, sizeof(buffer));

    Document d;
    d.ParseStream(streamForReadingClassTypesFile);
    
    fclose(f);
    
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

    void onChannel(SleepyDiscord::Channel channel) { //called when new channel created

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

    void onMessage(SleepyDiscord::Message message) override { //called on any message

        if (message.startsWith("!timetable-help")) {

            WebsocketppDiscordClient::sendMessage(message.channelID, 
                "Hello. This bot is created to **send timetable from tsu-intime to your discord server.**");

            WebsocketppDiscordClient::sendMessage(message.channelID, 
                "Follow these steps: \n***1) Create text channel*** *(recommended name: <group-number>-timetable (without <>).\
                For example: ***932102-timetable***)*.\n*2) Send ***!timetable*** command to this channel*");

            return;
        }
        else if (message.startsWith("!timetable")) {

            auto channel = channels.find(message.channelID);

            string channelName(channel->second.name), groupNumber = channelName.substr(0,6);

            if (channelName.find("timetable") == std::string::npos) {
                
                sendMessage(message.channelID,
                    "Please, *rename your channel correctly*(for more info do ***!timetable-help***).");
                
            } else {
                try {

                    Document privateRequestParameters = 
                        documentFromFile("/home/ec/projects/TsuTimetableBot/jsonPrivateFiles/requestParameters.json");

                    Document weekdayString = documentFromFile("/home/ec/projects/TsuTimetableBot/jsonPrivateFiles/weekdaysJsonFile.json");

                    Document classTypeString = documentFromFile("/home/ec/projects/TsuTimetableBot/jsonPrivateFiles/classTypesJsonFile.json");

                    Document groupID = documentFromFile("/home/ec/projects/TsuTimetableBot/jsonPrivateFiles/groupID.json");

                    if (!groupID.HasMember(groupNumber.c_str())) {
                        sendMessage(message.channelID,
                        "Don't have such group in list."
                    } else {
                        //converting human time in time since epoch for HTTP request parameters begin
                        auto dp = floor<days>(system_clock::now());
                        auto ymd = year_month_day{ dp };
                        auto iso = iso_week::year_weeknum_weekday{ ymd };

                        iso_week::year_weeknum_weekday* beginningWeekDate, * endingWeekDate;

                        if ((unsigned int)iso.weekday() != NUMBER_OF_SUNDAY) {

                            beginningWeekDate =
                                new iso_week::year_weeknum_weekday(iso.year(), --iso.weeknum(), weekday(7));

                            endingWeekDate =
                                new iso_week::year_weeknum_weekday(iso.year(), iso.weeknum(), weekday(7));

                        }
                        else {

                            beginningWeekDate =
                                new iso_week::year_weeknum_weekday(iso.year(), iso.weeknum(), weekday(7));

                            endingWeekDate =
                                new iso_week::year_weeknum_weekday(iso.year(), ++iso.weeknum(), weekday(7));

                        }

                        string beginningWeekDateString = to_string((((sys_days)
                            (*beginningWeekDate)).time_since_epoch()).count() * SECONDS_IN_HOUR * HOURS_IN_DAY + GAP * SECONDS_IN_HOUR);

                        string endingWeekDateString = to_string((((sys_days)
                            (*endingWeekDate)).time_since_epoch()).count() * SECONDS_IN_HOUR * HOURS_IN_DAY + GAP * SECONDS_IN_HOUR - 1);
                        //converting end 
                        
                        //making request url string
                        string apiRequestUrlString = string(privateRequestParameters["1"].GetString()) +
                            string(groupID[groupNumber.c_str()].GetString())
                                + string(privateRequestParameters["2"].GetString()) + beginningWeekDateString +
                                        string(privateRequestParameters["3"].GetString()) + endingWeekDateString;
                        
                        //doing request (got JSON)
                        cpr::Response response = cpr::Get(cpr::Url{ apiRequestUrlString });

                        Document currentTimetable;
                        currentTimetable.Parse(response.text.c_str());
                       
                        string stringBuffer;
                        
                        //beginning of parsing JSON
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

                            stringBuffer += "*" + sendingDateStream.str() + "*" + ". " + "***" 
                                    + string(weekdayString[weekdayStringStream.str().c_str()].GetString()) + "***: \n";

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

                                sendMessage(message.channelID, stringBuffer + "\n", SleepyDiscord::Sync);
                                sleep(1000);
                                stringBuffer.clear();
                            }
                                else {
                                        
                                    stringBuffer.clear();
                                    continue;
                                }

                            

                        }
                    }
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

        Document privateBotData = documentFromFile("/home/ec/projects/TsuTimetableBot/jsonPrivateFiles/privateBotData.json" );

        MyClientClass client(privateBotData["token"].GetString(), SleepyDiscord::USER_CONTROLED_THREADS);

        client.run();
    }
