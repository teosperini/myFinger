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
#include <limits.h>

void getAllUsers();
void getSpecifiedUser(const char* user);
char* formatTime(const time_t login_time, bool login);
void printSpecificUTMP(const struct utmp *ut);
char* formatPhoneNumber(const char* phoneNumber);
void handle_l();
void handle_s();

#endif /* MYFINGER_H */
