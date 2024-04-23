#include <myfinger.h>

int main(int argc, char** argv) {
	if (argc == 1){
		printf("%-20s %-10s\n", "Name", "TTY");
		getAllUsers();
	}else if (argc == 2){
		char *argument = argv[1];
	    if (argument[0] == '-') {
	    	// this means that the second parameter is a command
	    	// for now there are no command available, so print all the users normally
	    	//TODO.......................................................
	        getAllUsers();
	    } else {
	        getSpecifiedUser(argv[1]);
	    }
	}else if(argc == 3){
		//this means that the command is like: myfinger.o -[lmsp] [user]
		//for now it will return only a specific user normally
		getSpecifiedUser(argv[2]);
	}else{
		return 1;
	}
	return 0;
}


void getAllUsers(){
	struct utmp *ut;
	setutent();
    while ((ut = getutent()) != NULL) {
        if (ut->ut_type == USER_PROCESS) {
        	printf("Username: %s\n", ut->ut_user);
        	//printUTMP(ut);
        }
    }
    endutent();
}

void getSpecifiedUser(const char* user){
	struct passwd *pw;
    if ((pw = getpwnam(user)) != NULL) {
    	char* login = pw->pw_name;
    	char* gecos = strdup(pw->pw_gecos); // Duplica la stringa per evitare modifiche dirette
    	char* name = strsep(&gecos, ",");
    	if (name != NULL){
    		printf("Login: %-32s Name: %s\n", login, name);
    	} else {
    		printf("Login: %-32s Name:\n", login);
    	}
        printf("Directory: %-28s Shell: %-23s\n", pw->pw_dir, pw->pw_shell);
       	int userLogged = 0;
    	struct utmp *ut;
		setutent();
		while ((ut = getutent()) != NULL) {
    		if (ut->ut_type == USER_PROCESS && strncmp(ut->ut_user, user, UT_NAMESIZE) == 0) {
    			printSpecificUTMP(ut);
    			userLogged = 1;
    		}
		}
		endutent();
		if (userLogged == 0){
			printf("Never logged in.\n");
		}

        for (int i = 0; i < 4; ++i) {
            char* field = strsep(&gecos, ",");
            if (field != NULL && strcmp(field, "") != 0) {
                switch (i) {
                    case 0:
                        printf("Office: %s\n", field);
                        break;
                    case 1:
                        printf("Work number: %s\n", formatPhoneNumber(field));
                        break;
                    case 2:
                        printf("Home number: %s\n", formatPhoneNumber(field));
                        break;
                    case 3:
                        printf("Mail: %s\n", field);
                        break;
                }
            } else {
                switch (i) {
                    case 0:
                        printf("No office.\n");
                        break;
                    case 1:
                        printf("No work number.\n");
                        break;
                    case 2:
                        printf("No home number.\n");
                        break;
                    case 3:
                        printf("No mail.\n");
                        break;
                }
            }
        }
    } else {
        printf("User not found\n");
    }
}

void printSpecificUTMP(const struct utmp *ut) {
    time_t login_time = ut->ut_tv.tv_sec;
    char *formatted_login_time = formatTime(login_time, true);
    if (formatted_login_time == NULL) {
        printf("Errore durante la formattazione del login time\n");
    } else {
        printf("%s", formatted_login_time);
        free(formatted_login_time);
    }
    printf(" on %s from %s\n", ut->ut_line, ut->ut_host);

    char *formatted_idle_time = formatTime(login_time, false);
    if (formatted_idle_time == NULL) {
        printf("Errore durante la formattazione dell'idle time\n");
    } else {
        printf("%s", formatted_idle_time);
        free(formatted_idle_time);
    }
}

char *formatTime(const time_t time_seconds, bool isLogin) {
    time_t current_time = time(NULL);
    double diff_seconds = difftime(current_time, time_seconds);

    struct tm *time_info = localtime(&time_seconds);
    char *time_buffer = malloc(100 * sizeof(char));
    if (time_buffer == NULL) {
        return NULL;
    }

    if (isLogin) {
        if (diff_seconds > 15552000) {
            // Formatta la data e l'ora con l'anno e memorizzale nel buffer
            strftime(time_buffer, 100, "On since %a %b %Y %H:%M (%Z)\n", time_info);
        } else {
            // Se sono trascorsi meno di 6 mesi, formatta la data e l'ora con ore e minuti
            strftime(time_buffer, 100, "On since %a %b %d %H:%M (%Z)\n", time_info);
        }
    } else {
        int hours = diff_seconds / 3600;
        int minutes = ((int)diff_seconds % 3600) / 60;
        sprintf(time_buffer, "  %d hour %d minutes idle\n", hours, minutes);
    }

    return time_buffer;
}

char* formatPhoneNumber(const char* phoneNumber){
if (strlen(phoneNumber) != 10) {
        printf("Numero di telefono non valido.\n");
        return NULL;
    }

    char* number_buffer = malloc(15 * sizeof(char));
    if (number_buffer == NULL) {
        printf("Errore nell'allocazione della memoria.\n");
        return NULL;
        }

    snprintf(number_buffer, 15, "%.*s-%.*s-%.*s",
             3, phoneNumber,
             3, phoneNumber + 3,
             4, phoneNumber + 6);

    return number_buffer;
}