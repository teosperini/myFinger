#ifndef MYFINGER_H
#define MYFINGER_H

#include <stdio.h>
#include <utmp.h>
#include <pwd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <limits.h>

void getActiveUsers();
void getSpecifiedUser(const char* user);
void getSpecifiedAll();
char* formatTime(const time_t login_time, bool login);
void printSpecificUTMP(const struct utmp* ut);
char* formatPhoneNumber(const char* phoneNumber);
void getSpecifiedUserPW(const char* user);
bool checkAsterisk(const char* line);
void printShort(const struct passwd* pw, const struct utmp* ut);
void printLong(const struct passwd* pw, const struct utmp* ut);
void handle_no_names();
void handle_names(char** names, int names_count);

#endif /* MYFINGER_H */
