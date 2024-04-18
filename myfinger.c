#include <stdio.h>
#include <utmp.h>
#include <pwd.h>
#include <string.h>

void getAllUsers();
void getSpecifiedUser();

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
            printf("Username: %s\n", ut->ut_user);
            printf("Terminal: %s\n", ut->ut_line);
            printf("Host: %s\n", ut->ut_host);
            printf("Login time: %d\n", ut->ut_tv.tv_sec);
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
