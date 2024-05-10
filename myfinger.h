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

#define MAX_USERS 100  // Numero massimo di utenti
#define MAX_NAME_LENGTH 32  // Lunghezza massima del nome utente

void getActiveUsers();
void getSpecifiedUser(const char* user);
void getSpecifiedAll();
char* formatTime(const time_t login_time, bool login);
void printSpecificUTMP(const struct utmp* ut);
void printSpecificPW(const struct passwd* pw);
char* formatPhoneNumber(const char* phoneNumber);
bool checkAsterisk(const char* line);
void printShort(const struct passwd* pw, const struct utmp* ut);
void printLong(const struct passwd* pw, const struct utmp* ut);
void printStartL(const struct passwd* pw);
void printEndL(const struct passwd* pw);
void handle_no_names();
void handle_names(char** names, int names_count);

#endif /* MYFINGER_H */
