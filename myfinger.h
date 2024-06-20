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

#define MAX_USERS 100
#define MAX_NAME_LENGTH 32

typedef struct {
    time_t time;  //ut->ut_ut_tv.tv_sec
    char tty[10];   //ut->ut_line
    char host[10];  //ut->ut_host
} UserUTMP;

void handle_active_users();
void handle_specified_users(char** names, int size);

void lookup_user_info(const char* user, char** copies);

bool check_presence(const char* name, char** copies);
bool check_write_status(const char* line);

void print_start_l(const struct passwd* pw);
void print_l(UserUTMP* user, bool wtmp);
void print_end_l(const struct passwd* pw);
void print_start_s();
void print_s(const struct passwd* pw, UserUTMP* userUTMP, bool wtmp);

char* format_phone_number(const char* phoneNumber);
char* format_time(const time_t login_time, bool login);
char* format_short_time(const time_t time_seconds, bool isLogin);
#endif /* MYFINGER_H */
