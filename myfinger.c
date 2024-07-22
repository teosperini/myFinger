#include "myfinger.h"

// The four possible finger options
bool l_option = false;
bool m_option = false;
bool s_option = false;
bool p_option = false;

// Flag needed to print the header in the short format
bool header = false;

int main(int argc, char** argv) {
    int opt;
    char** names = malloc(sizeof(char*));   // The array names will contain the names given in input
    int names_count = 0;                        // Index and counter for the array names

    while ((opt = getopt(argc, argv, "lmsp")) != -1) {
        switch (opt) {
            case 'l':
                // Handle l option
                l_option = true;
                break;
            case 'm':
                // Handle m option
                m_option = true;\
                break;
            case 's':
                // Handle s option
                s_option = true;
                break;
            case 'p':
                // Handle p option
                p_option = true;
                break;
            case '?':
                // Handle unknown options error
                fprintf(stderr, "usage: myfinger [-lmps] [login ...]\n");
                return 1;
            default:
                // Handle other errors
                abort();
        }
    }

    // Filling the names array
    for(int i = optind; i < argc; ++i){
        bool try = false;
        // Checking if the current name is already present in the array
        for(int j = 0; j < names_count; ++j){
            if(strcmp(names[j], argv[i]) == 0){
                try = true;
            }
        }
        // If the name is already present, skip the cycle
        if(try){
            continue;
        }
        // If it is not, the array gets resized and the name gets added
        names = (char**)realloc(names, (names_count + 1) * sizeof(char*));
        if (names == NULL) {
            fprintf(stderr, "An error occurred during the memory allocation for the names array.\n");
            return 1;
        }
        names[names_count++] = argv[i];
    }

    /*  possible cases:
        - no names: look for active users
            -s default
            -m no effect
            -l if specified
            -p if specified and -l is true
        - with names: look for the specified users
            -l default
            -m if speciified
            -s if specified
            -p if specified and -s is false (or -l && -s == true)
    */

    if (names_count == 0){
        handle_active_users();
    } else {
        handle_specified_users(names, names_count);
    }
    return 0;
}

/*
 * This function look for the current active users,
 * then proceeds to call the core function over them
 * If no format option is specified, it defaults to short
 */
void handle_active_users(){
    s_option = true;
    struct utmp* ut;
    setutent(); // Set the pointer at the beginning of the utmp file
    char encounteredUsers[MAX_USERS][MAX_NAME_LENGTH];  // Array to save the encountered users
    int numEncounteredUsers = 0;                        // Index of the array
    while ((ut = getutent()) != NULL) {                 // Iterating over the active entites
        if (ut->ut_type == USER_PROCESS) {              // Checking if the entity is an user process
            bool userEncountered = false;

            char user[UT_NAMESIZE];
            strncpy(user, ut->ut_user, UT_NAMESIZE);

            for (int i = 0; i < numEncounteredUsers; ++i) {     // Checking if the user is already
                if (strcmp(encounteredUsers[i], user) == 0) {   // present in the array
                    userEncountered = true;
                    break;
                }
            }
            if (!userEncountered) {         // If the user is a new user, print its finger
                strncpy(encounteredUsers[numEncounteredUsers], user, UT_NAMESIZE);
                ++numEncounteredUsers;
                userEncountered = false;
            }
        }
    }
    endutent();
    for (int i = 0; i < numEncounteredUsers; ++i) {
        lookup_user_info(encounteredUsers[i], NULL);
    }
}

/*
 * This function calls the core function over all the names given by the user prompt
 * If no format option is specified, it defaults to long
 */
void handle_specified_users(char** names, int size){
    char** copies = malloc(sizeof(char*)*size+2);   // Creating an array of copies
    if (copies == NULL) {
        fprintf(stderr, "Error allocating memory for adding a name\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < size; i++) {
        lookup_user_info(names[i], copies);
    }
    free(names);
    free(copies);
}


/*
 * This is the core function of the program
 * This function looks for the informations about the users firstly in the PASSWD file, then in the
 * UTMP file
 */
void lookup_user_info(const char* user, char** copies) {
    struct passwd *pw;
    setpwent();
                                    // Function-global boolean that is false when no entry
    bool user_found = false;        // in the passwd is found matching the user's username
                                    // It is used to print the "no such user" label
    while((pw = getpwent()) != NULL) {          // Iterating over every entry of the passwd file
        char* gecos = strdup(pw->pw_gecos);
        char* name = strsep(&gecos, ",");
    
        bool passwd_found = false;

        if(strcmp(user, pw->pw_name) == 0) {    // Check if the current passwd entry perfectly
            passwd_found = true;                // matches the user's unique username
        }
        if(!m_option) {                         // If the m option is false then proceeds to look
            char* singleName;                   // for the user's username in the first gecos field
            if(strcasecmp(user, name) == 0) {
                passwd_found = true;
            }
            while((singleName = strsep(&name, " "))) {
                if(strcasecmp(user, singleName) == 0) {
                    passwd_found = true;
                }
            }
        }

        // If at least one comparision was successful, then look for the user's utmp/wtmp informations
        if(passwd_found) {
            passwd_found = false;
            user_found = true;      // This sets true confirms (out of the while) that the user has
                                    // been found
            if(check_presence(pw->pw_name, copies)) {    // If the name has already been printed
                continue;                               // skip the cycle
            }

            if(!s_option || (l_option && s_option)) {   // Print the first section of the finger output
                print_start_l(pw);                        // For the long option, it is printed every user
            } else {
                print_start_s();                          // For the short option, it is printed once
            }

            // Creating and allocating memory for the UserUTMP struct that will contain the information
            // needed to print about the user
            UserUTMP* userUTMP = malloc(sizeof(UserUTMP));
            if (userUTMP == NULL) {
                fprintf(stderr, "Failed to allocate memory for userUTMP\n");
                exit(EXIT_FAILURE);
            }
            memset(userUTMP, 0, sizeof(UserUTMP));

            // Scanning the utmp file in search of the user
            struct utmp* ut;
            setutent();
            bool utmp_found = false;
            bool wtmp_print = true;
            while ((ut = getutent()) != NULL) {
                if (ut->ut_type == USER_PROCESS && strncmp(ut->ut_user, pw->pw_name, UT_NAMESIZE) == 0) {
                    // Filling the userUTMP struct with the needed utmp informations
                    userUTMP->time = ut->ut_tv.tv_sec;
                    strncpy(userUTMP->tty, ut->ut_line, sizeof(userUTMP->tty));
                    strncpy(userUTMP->host, ut->ut_host, sizeof(userUTMP->host));

                    if(!s_option || (l_option && s_option)) {
                        print_l(userUTMP, !wtmp_print);
                    } else {
                        print_s(pw, userUTMP, !wtmp_print);
                    }

                    utmp_found = true;
                }
            }
            endutent();

            // If the user is not found in the utmp file, look in the wtmp file
            if (!utmp_found) {
                struct utmp wt;
                int wtmp_fd;
                time_t last_login_time = 0;

                // Trying to open the wtmp file
                if ((wtmp_fd = open(WTMP_FILE, O_RDONLY)) == -1) {
                    perror("Error during the opening of the wtmp file");
                    exit(EXIT_FAILURE);
                }

                // Manually scanning the wtmp file (there is no library that automatize such operationn)
                while (read(wtmp_fd, &wt, sizeof(struct utmp) ) == sizeof(struct utmp)) {
                    if (strncmp(wt.ut_name, pw->pw_name, UT_NAMESIZE) == 0 && wt.ut_type == USER_PROCESS) {
                        if (wt.ut_tv.tv_sec > last_login_time) {
                            // Filling the userUTMP struct with the needed wtmp informations
                            last_login_time = wt.ut_tv.tv_sec;
                            strncpy(userUTMP->tty, wt.ut_line, sizeof(userUTMP->tty));
                            strncpy(userUTMP->host, wt.ut_host, sizeof(userUTMP->host));
                            userUTMP->time = wt.ut_tv.tv_sec;
                        }
                    }
                }
                if (last_login_time != 0) {
                    // If the wtmp entry for the user is found, print it
                    if(!s_option || (l_option && s_option)) {
                        print_l(userUTMP, wtmp_print);
                    } else {
                        print_s(pw, userUTMP, wtmp_print);
                    }
                    // Else print the informations with the "never logged in" label
                } else {
                    if(!s_option || (l_option && s_option)) {
                        print_l(NULL, false);
                    } else {
                        print_s(pw, NULL, false);
                    }
                }

                close(wtmp_fd);
            }

            // In case of the long format, print the end
            if(!s_option || (l_option && s_option)) {
                print_end_l(pw);
            }
            free(userUTMP);
        }
    }
    endpwent();
    if(!user_found) {
        printf("myfinger: %s: no such user.\n", user);
    }
}

/*
 * This function checks if the current user is already been printed
 */
bool check_presence(const char* name, char** copies) {
    if(copies != NULL){
        int count = 0;
        while(copies[count] != NULL) {
            count++;
        }
        for (int i = 0; i < count; i++) {
            if (strcmp(name, copies[i]) == 0) {
                return true;
            }
        }
        copies[count] = strdup(name);
        if (copies[count] == NULL) {
            fprintf(stderr, "Error duplicating the name in the check presence function\n");
            exit(EXIT_FAILURE);
        }
        copies[count + 1] = NULL;
    }
    return false;
}

/*
 * If the long option is enabled, this function prints the common information for the user
 */
void print_start_l(const struct passwd* pw){
    // Getting the information from the gecos field
    char* login = pw->pw_name;
    char* gecos = strdup(pw->pw_gecos);
    if(gecos != NULL){
        char* name = strsep(&gecos, ",");
        if(name == NULL) {
            name = strdup("");
        }

        char* office = strsep(&gecos, ",");
        if(office == NULL) {
            office = strdup("");
        }

        char* workNumber = strsep(&gecos, ",");
        if(workNumber == NULL) {
            workNumber = strdup("");
        } else {
            workNumber = format_phone_number(workNumber);
        }

        char* homeNumber = strsep(&gecos, ",");
        if(homeNumber == NULL) {
            homeNumber = strdup("");
        } else {
            homeNumber = format_phone_number(homeNumber);
        }

        // Printing the correctly fomatted information
        if(strcmp(name, "") == 0){
            printf("Login: %-32s Name:\n", login);
        } else {
            printf("Login: %-32s Name: %s\n", login, name);
        }
        printf("Directory: %-28s Shell: %-23s\n", pw->pw_dir, pw->pw_shell);

        char* finalString =  (char*)malloc(250 * sizeof(char));
        if(finalString == NULL){
            fprintf(stderr, "Errore durante l'allocazione di memoria per l'utente.\n");
            return;
        }

        if(strcmp(office, "") == 0){
            strcpy(finalString, "Office Phone: ");
        } else {
            strcpy(finalString, "Office: ");
            strcat(finalString, office);
        }

        if(strcmp(workNumber, "") != 0){
            if(strcmp(finalString, "Office Phone: ") != 0){
                strcat(finalString, ", ");
            }
            strcat(finalString, workNumber);
            free(workNumber);
        } else {
            if(strcmp(finalString, "Office Phone: ") == 0){
                strcpy(finalString, "");
            }
        }

        int count = strlen(finalString);
        int maxLenght = 40;

        if(count > maxLenght){
            strcat(finalString, "\n");
        } else {
            if(strcmp(finalString, "") != 0){
                for(int i = 0; i < maxLenght - count; i++){
                    strcat(finalString, " ");
                }
            }
        }

        if(strcmp(homeNumber, "") != 0){
            strcat(finalString, "Home Number: ");
            strcat(finalString, homeNumber);
            strcat(finalString, "\n");
            free(homeNumber);
        } else {
            if(strcmp(finalString, "") != 0){
                strcat(finalString, "\n");
            }
        }
        printf("%s",finalString);
        free(finalString);
    }
}

/*
 * If the long option is enabled, this function prints the information of a single login
 * for the current user
 */
void print_l(UserUTMP* user, bool wtmp){
    // Printing the current utmp information for the user
    if(user != NULL){
        time_t login_time = user->time;
        char* formatted_login_time = format_time(login_time, true);
        if (formatted_login_time == NULL) {
            printf("Error during the login time formatting process\n");
            strcpy(formatted_login_time, "");
        } else {
            // If the information are taken from the wtmp file, the label changes
            if(!wtmp){
                printf("On since %s", formatted_login_time);
            } else {
                printf("Last login %s", formatted_login_time);
            }
            free(formatted_login_time);
        }
        printf(" on %-5s", user->tty);
        // If the information are taken from the wtmp file, there is no idle time
        if(!wtmp){
            char* formatted_idle_time = format_time(login_time, false);

            if (formatted_idle_time != NULL) {
                const char* teletype = user->tty;

                if (check_write_status(teletype)){
                    if(strcmp(user->host, "") != 0){
                        printf(" from %s\n", user->host);
                    }
                    printf("%s\n", formatted_idle_time);
                } else {
                    printf("%s\n     (messages off)\n", formatted_idle_time);
                }
                free(formatted_idle_time);

            } else {
                printf("Error during the idle time formatting process\n");
            }
        } else {
            printf("\n");
        }
    } else {
        printf("Never logged in.\n");
    }
}

/*
 * If the long option is enabled, this function prints the last information for the current user
 */
void print_end_l(const struct passwd* pw){
    char plan_path[1024];
    snprintf(plan_path, sizeof(plan_path), "%s/.plan", pw->pw_dir);

    struct stat st;
    if (stat(plan_path, &st) == -1) {
        printf("No Plan.\n");
        return;
    }

    FILE *file = fopen(plan_path, "r");
    if (file == NULL) {
        perror("fopen");
        return;
    }

    printf("Plan:\n");
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("%s", buffer);
    }

    fclose(file);
}

/*
 * If the short option is enabled, this function prints only once the header of the myfinger program
 */
void print_start_s(){
    if(!header){
        printf("Login\t  Name\t\t    Tty\t     Idle  Login Time   Office\t   Office Phone\n");
        header = true;
    }
}

/*
* If the short option is enabled, this function prints the information of a single login
 * for the current user
 */
void print_s(const struct passwd* pw, UserUTMP* userUTMP, bool wtmp){
    // Getting the information from the gecos field
    char* login = pw->pw_name;
    char* gecos = strdup(pw->pw_gecos);
    if(gecos != NULL){
        char* name = strsep(&gecos, ",");
        if(name == NULL) {
            name = strdup("");
        }

        char* office = strsep(&gecos, ",");
        if(office == NULL) {
            office = strdup("");
        }

        char* workNumber = strsep(&gecos, ",");
        if(workNumber == NULL) {
            workNumber = strdup("");
        } else {
            workNumber = format_phone_number(workNumber);
        }

        // Printing the correctly fomatted information
        printf("%-10.10s", login);
        if (strcmp(name,"")!=0){
            printf("%-17.16s", name);
        } else {
            printf("                 ");
        }

        if(userUTMP != NULL){
            // If the user is logged in or has logged in in the past, print the information about it
            const char* teletype = userUTMP->tty;
            if (check_write_status(teletype)){
                printf(" %-9.10s", teletype);
            } else {
                printf("*%-9.9s", teletype);
            }
            if(!wtmp){
                char* idleTime = format_short_time(userUTMP->time, false);
                printf("%-6.5s", idleTime);
                free(idleTime);
            } else {
                printf("   *  ");
            }
            char* loginTime = format_short_time(userUTMP->time, true);
            printf("%-13.12s", loginTime);
            free(loginTime);
            if(strcmp(userUTMP->host, "") != 0){
                char* copy = strdup(userUTMP->host);
                strcat(copy, ")");
                printf("(%-10.9s", copy);
            } else {
                if(strcmp(office, "") != 0){
                    printf("%-11.10s", office);
                } else {
                    printf("           ");
                }
                if(strcmp(workNumber, "") != 0){
                    printf("%-13.12s", workNumber);
                }
            }
            printf("\n");
        } else {
            // Else, print the No logins label
            printf("  *\t        *   No logins\n");
        }
    }
}

/*
 * This function checks if write permission is denided to the device or not
 */
bool check_write_status(const char* line){
    char file_path[256];
    strcpy(file_path, "/dev/");
    strcat(file_path, line);

    struct stat file_stat;
    if (stat(file_path, &file_stat) == 0) {
        if (file_stat.st_mode & (S_IWOTH | S_IWGRP)) {
            return true;
        }
    }
    return false;
}

/*
 * This function correctly format the time for the long option
 */
char* format_time(const time_t time_seconds, bool isLogin) {
    time_t current_time = time(NULL);
    double diff_seconds = difftime(current_time, time_seconds);
    struct tm *time_info = localtime(&time_seconds);
    int size = 100;
    char* time_buffer = malloc(size * sizeof(char));
    if (time_buffer == NULL) {
        return NULL;
    }

    if (isLogin) {
        if (diff_seconds < 31536000) {
            // Less than a year login time string
            strftime(time_buffer, 100, "%a %b %d %H:%M (%Z)", time_info);
        } else {
            // More than a year login time string
            strftime(time_buffer, 100, "%d %b %Y", time_info);
        }
    } else {
        if (diff_seconds < 3600) { // Less than an hour
            // Prints only the minutes
            int minutes = (int)(diff_seconds / 60);
            snprintf(time_buffer, size, "    %d minutes idle", minutes);
        } else if (diff_seconds < 86400) { // Less than a day
            // Prints hours and minutes
            int hours = (int)(diff_seconds / 3600);
            int minutes = (int)(diff_seconds / 60) % 60;
            snprintf(time_buffer, size, "    %d hour %d minutes idle", hours, minutes);
        } else if (diff_seconds < 31536000) { // Less than a year
            // Prints the days
            int days = (int)(diff_seconds / 86400);
            snprintf(time_buffer, size, "    %d days idle", days);
        } else { // More than a year
            // Prints the years
            int years = (int)(diff_seconds / 31536000);
            snprintf(time_buffer, size, "    %d years idle", years);
        }
    }

    return time_buffer;
}

/*
 * This function correctly format the time for the short option
 */
char* format_short_time(const time_t time_seconds, bool isLogin) {
    time_t current_time = time(NULL);
    double diff_seconds = difftime(current_time, time_seconds);

    struct tm *time_info = localtime(&time_seconds);
    int size = 20;
    char* time_buffer = malloc(size * sizeof(char));
    if (time_buffer == NULL) {
        return NULL;
    }
    if (isLogin) {
        if (diff_seconds < 31536000){
            // Less than a year login time string
            strftime(time_buffer, size, "%b %d %H:%M", time_info);
        } else {
            // More than a year login time string
            strftime(time_buffer, size, "%b %Y", time_info);
        }
    } else {
        if (diff_seconds < 3600) { // Less than an hour
            // Prints only the minutes
            int minutes = (int)(diff_seconds / 60);
            snprintf(time_buffer, size, "%d", minutes);
        } else if (diff_seconds < 86400) { // Less than a day
            // Prints hours and minutes
            int hours = (int)(diff_seconds / 3600);
            int minutes = (int)(diff_seconds / 60) % 60;
            snprintf(time_buffer, size, "%d:%02d", hours, minutes);
        } else if (diff_seconds < 31536000) { // Less than a year
            // Prints the days
            int days = (int)(diff_seconds / 86400);
            snprintf(time_buffer, size, "%dd", days);
        } else { // More than a year
            // Prints the years
            int years = (int)(diff_seconds / 31536000);
            snprintf(time_buffer, size, "%dy", years);
        }
    }
    return time_buffer;
}

/*
 * This function correctly forma the phone numbers
 */
char* format_phone_number(const char* phoneNumber){
    int len = strlen(phoneNumber);
    char* number_buffer = malloc(15 * sizeof(char));
    if (number_buffer == NULL) {
        printf("Errore nell'allocazione della memoria.\n");
        return NULL;
    }

    if(len<=4){
        snprintf(number_buffer, 15, "%s", phoneNumber);
    } else if(len>4 && len <= 7){
        int i = len - 4;
        snprintf(number_buffer, 15, "%.*s-%.*s", i, phoneNumber, 4, phoneNumber+i);
    } else if(len>7 && len <= 10){
        int j = len - 7;
        snprintf(number_buffer, 15, "%.*s-%.*s-%.*s", j , phoneNumber, 3, phoneNumber + j, 4, phoneNumber + j + 3);
    }

    return number_buffer;
}
