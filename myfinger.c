#include <stdio.h>
#include <utmp.h>
#include <pwd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

void getAllUsers();
void getSpecifiedUser();
char* formatLoginTime(time_t login_time);

int main(int argc, char** argv) {
	if (argc == 1){
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
            printf("Terminal: %s\n", ut->ut_line);
            printf("Host: %s\n", ut->ut_host);
            printf("Login time: %d\n", ut->ut_tv.tv_sec);
        }
    }
    endutent();
}

void getSpecifiedUser(char* user){
	char *target_user = user;
	struct utmp *ut;
	setutent();
    while ((ut = getutent()) != NULL) {
        if (ut->ut_type == USER_PROCESS && strncmp(ut->ut_user, target_user, UT_NAMESIZE) == 0) {
            //printf("Username: %s\n", ut->ut_user);
            printf("Terminal: %s\n", ut->ut_line);
            printf("Host: %s\n", ut->ut_host);

            //creating the login time string
            time_t login_time = ut->ut_tv.tv_sec;	//getting the login information from the file
            char *formatted_time = formatLoginTime(login_time);
		    if (formatted_time == NULL) {
		        printf("Errore durante la formattazione del login time\n");
		    }else{
			    printf("Login time: %s\n", formatted_time);
		    	free(formatted_time);
		    }
        }
    }
    endutent();
	struct passwd *pw;
	    if ((pw = getpwnam(user)) != NULL) {
	        printf("Username: %s\n", pw->pw_name);
	        printf("Home directory: %s\n", pw->pw_dir);
	        printf("Shell: %s\n", pw->pw_shell);
	    } else {
	        printf("User not found\n");
	    }
}

char* formatLoginTime(time_t login_time) {
    // Converti il tempo epoch in una struttura tm locale
    struct tm *local_time = localtime(&login_time);
    if (local_time == NULL) {
        // Errore nella conversione del tempo
        return NULL;
    }

    // Formatta la data e l'ora e memorizzale in un buffer dinamico
    char *time_buffer = malloc(100 * sizeof(char)); // Allocazione dinamica del buffer
    if (time_buffer == NULL) {
        // Errore nell'allocazione della memoria
        return NULL;
    }
    strftime(time_buffer, 100, "%Y-%m-%d %H:%M:%S", local_time);

    // Restituisci il puntatore al buffer contenente la stringa formattata
    return time_buffer;
}