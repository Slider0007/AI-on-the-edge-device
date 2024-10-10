#ifndef TIMESNTP_H
#define TIMESNTP_H

#include <string>
#include <ctime>


std::string getCurrentTimeString(const char * frm);
std::string convertTimeToString(time_t _time, const char * frm);

bool getTimeIsSet(void);
bool getUseNtp(void);
bool getTimeIsSynced(void);
std::string getNTPSyncStatus(void);
bool getTimeWasNotSetAtBoot(void);

bool initTime();
void reconfigureTime(bool _timeSyncEnabled, std::string _timeServer, std::string _timeZone);

#endif //TIMESNTP_H