#ifndef MYFINGER_H
#define MYFINGER_H

#include <stdio.h>
#include <utmp.h>
#include <pwd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

void getAllUsers();
void getSpecifiedUser(char* user);
char* formatLoginTime(time_t login_time);
void printUTMP(struct utmp *ut);

#endif /* MYFINGER_H */
