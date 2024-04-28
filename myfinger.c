#include <myfinger.h>

bool l_option = false;
bool m_option = false;
bool s_option = false;
bool p_option = false;

//finger di base cerca l'utente anche tramite il suo Nome, non solo
//tramite l'username. per far si che cerchi solo per username: -m
int main(int argc, char** argv) {
    int opt;
    char** names = NULL;
    int names_count = 0;

    while ((opt = getopt(argc, argv, "lmsp")) != -1) {

        switch (opt) {
            case 'l':
                // Gestisci l'opzione -l
                printf("Opzione -l selezionata\n");
                l_option = true;
                break;
            case 'm':
                // Gestisci l'opzione -m
               	printf("Opzione -m selezionata\n");
                m_option = true;
                break;
            case 's':
                // Gestisci l'opzione -s
                printf("Opzione -s selezionata\n");
                s_option = true;
                break;
            case 'p':
                // Gestisci l'opzione -p
                printf("Opzione -p selezionata\n");
                p_option = true;
                break;
            case '?':
                // Gestisci opzioni non riconosciute o mancanti argomenti
                if (optopt == 'm') {
                    fprintf(stderr, "L'opzione -m richiede un argomento.\n");
                } else if (isprint(optopt)) {
                    fprintf(stderr, "usage: finger [-lmps] [login ...]\n");

                } else {
                    fprintf(stderr, "Carattere sconosciuto '\\x%x'.\n", optopt);
                }
                return 1;
            default:
                // Gestisci altri errori
                abort();
        }
/////////TODO////////////////////////////////TODO////////////
    }
	for(int i = 1; i < argc; ++i){
	    if (strncmp(argv[i], "-", 1) == 0){
	        continue;
	    } else{
	   		names = (char**)realloc(names, (names_count + 1) * sizeof(char*));
			if (names == NULL) {
				fprintf(stderr, "Errore durante l'allocazione di memoria per i nomi.\n");
				return 1;
			}
			names[names_count++] = argv[i];
	        //getSpecifiedUser(argv[i]);
	        //printf("\n");
	        //invece salvarsi tutti i nomi
	    }
	}
    if(l_option){
        printf("ciao\n");
        //handle_l(); //funzione che utilizza i parametri globali m_option e p_option
    } else if(s_option){
    	printf("bru\n");
        //handle_s(); //funzione che utilizza il parametro globale m_option
    }
    printf("Nomi inseriti dall'utente:\n");
    for (int i = 0; i < names_count; ++i) {
        printf("%s\n", names[i]);
    }
    /*
    char** users_input = NULL;
    users_input = (char**)malloc(argc * sizeof(char*));

    for(int i = 1; i < argc; ++i){
        if (i == opt){
            continue;
        } else{
            users_input[i] = argv;
        }
    }
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
        char letter = 'a';
		printf("finger: invalid option -- '%c'\nusage: finger [-lmps] [login ...]\n", letter);
	}
    */
	return 0;
}


void getAllUsers(){
	struct utmp* ut;
	setutent();
    while ((ut = getutent()) != NULL) {
        if (ut->ut_type == USER_PROCESS) {
        	printf("Username: %s\n", ut->ut_user);
        }
    }
    endutent();
}

void getSpecifiedUser(const char* user){
	struct passwd* pw;
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
    	struct utmp* ut;
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
		//per gli asterischi controllare accesso alla home directory access(pw_dir, W_OK);

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
                        printf("Mail: TODO %s\n", field);
                        ///////////////TODO/////////////////////////////////
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
                        printf("No mail TODO.\n");
                        ///////////////TODO/////////////////////////////////
                        break;
                }
            }
        }
    } else {
        printf("User not found\n");
    }
}

void printSpecificUTMP(const struct utmp* ut) {
    time_t login_time = ut->ut_tv.tv_sec;
    char* formatted_login_time = formatTime(login_time, true);
    if (formatted_login_time == NULL) {
        printf("Errore durante la formattazione del login time\n");
    } else {
        printf("%s", formatted_login_time);
        free(formatted_login_time);
    }

    printf(" on %s", ut->ut_line);
    char* formatted_idle_time = formatTime(login_time, false);

    if (formatted_idle_time != NULL) {

        const char* host = ut->ut_host;
        if(strcmp(host, "") != 0){
            printf(" from %s\n", ut->ut_host);
            printf("%s", formatted_idle_time);
        } else {
            printf("  %s\t(messages off)\n", formatted_idle_time);
        }

    } else {
        printf("Errore durante la formattazione dell'idle time\n");
    }
    //non stampo from, ma stampo al posto di from il tempo di idle, e a capo message off
    free(formatted_idle_time);
}


char* formatTime(const time_t time_seconds, bool isLogin) {
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
            strftime(time_buffer, 100, "On since %a %b %Y %H:%M (%Z)", time_info);
        } else {
            // Se sono trascorsi meno di 6 mesi, formatta la data e l'ora con ore e minuti
            strftime(time_buffer, 100, "On since %a %b %d %H:%M (%Z)", time_info);
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
