#ifndef MYFINGER_H
#define MYFINGER_H

#include <stdio.h>
#include <utmp.h>
#include <utmpx.h>
#include <pwd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <limits.h>

#define MIAO "/var/log/wtmp"
#define MAX_USERS 100  // Numero massimo di utenti
#define MAX_NAME_LENGTH 32  // Lunghezza massima del nome utente

typedef struct {
    time_t time;  //ut->ut_ut_tv.tv_sec
    char tty[10];   //ut->ut_line
    char host[10];  //ut->ut_host
} UserUTMP;

void handle_no_names();
void handle_names(char** names, int names_count);

void getActiveUsers();
void getSpecifiedUser(const char* user, char** copies);

bool checkPresence(const char* name, char** copies);
bool checkAsterisk(const char* line);

void printStartL(const struct passwd* pw);
void printLong(UserUTMP* user, bool wtmp);
void printEndL(const struct passwd* pw);
void printStartS();
void printShort(const struct passwd* pw, UserUTMP* userUTMP, bool wtmp);

char* formatPhoneNumber(const char* phoneNumber);
char* formatTime(const time_t login_time, bool login);
char* formatShortTime(const time_t time_seconds, bool isLogin);
#endif /* MYFINGER_H */
