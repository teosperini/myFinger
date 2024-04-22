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
    	char* name;
    	if ((name = pw->pw_gecos) != NULL){
    		printf("Login: %-32s Name:\n", pw->pw_name);
    	} else {
    		printf("Login: %-32s Name: %s\n", pw->pw_name, strtok(pw->pw_gecos, ","));
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
		/*
		char* mail;
		if ((mail = pw->pw_) != NULL){
    	printf("");
    	} else {
    		printf("No mail.");
    	}
		//Aggiungere mail e plan "No mail." "No Plan."
		*/
    } else {
        printf("User not found\n");
    }
}


void printSpecificUTMP(const struct utmp *ut){
	//creating the login time string
    time_t login_time = ut->ut_tv.tv_sec;	//getting the login information from the file
    char *formatted_time = formatLoginTime(login_time);
    if (formatted_time == NULL) {
        printf("Errore durante la formattazione del login time\n");
    }else{
	    printf("%s", formatted_time);
    	free(formatted_time);
    }
    printf(" on %s from %s\n", ut->ut_line, ut->ut_host);
    ut->ut_info;
    //printf("BOH: %d\n", ut->ut_type);
}


char* formatLoginTime(const time_t login_seconds) {
	// TODO:
	// Controllo se il tempo trascorso dall'ultimo accesso è maggiore di 6 mesi
	//time_t t = time(NULL);

	//struct tm1 *current_time = localtime(&t);
    // Converti il tempo epoch in una struct tm
    struct tm *login_time = localtime(&login_seconds);

    //if (current_time == NULL || login_time == NULL) {
    if (login_time == NULL) {
        // Errore nella conversione del tempo
        return NULL;
    }

    // Formatta la data e l'ora e memorizzale in un buffer dinamico
    char *time_buffer = malloc(120 * sizeof(char)); // Allocazione dinamica del buffer
    if (time_buffer == NULL) {
        // Errore nell'allocazione della memoria
        return NULL;
    }
    strftime(time_buffer, 120, "On since %a %b %d %H:%M (%Z)", login_time);

    //DA COMPLETARE PER QUANDO SI UTILIZZANO GLI ANNI strftime(time_buffer, 120, "On since %a %b %Y %m %d %H:%M:%S", local_time);

    // Restituisci il puntatore al buffer contenente la stringa formattata
    return time_buffer;
}


