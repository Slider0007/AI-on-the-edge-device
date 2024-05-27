#ifndef TIMESNTP_H
#define TIMESNTP_H

#include <string>
#include <ctime>


std::string getCurrentTimeString(const char * frm);
std::string ConvertTimeToString(time_t _time, const char * frm);


bool getTimeIsSet(void);
bool getTimeWasNotSetAtBoot(void);
bool getTimeWasSetOnce(void);

std::string getNTPSyncStatus(void);
bool getUseNtp(void);
bool setupTime();
void setupTimeZone(std::string _timeZone);
void setupTimeServer(std::string _timeServer);

bool waitingForTimeSync(void);


#endif //TIMESNTP_H