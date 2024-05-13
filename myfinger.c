#include <myfinger.h>

bool l_option = false;
bool m_option = false;
bool s_option = false;
bool p_option = false;
bool intestazione = false;


int main(int argc, char** argv) {
    int opt;
    char** names = NULL;
    int names_count = 0;
    while ((opt = getopt(argc, argv, "lmsp")) != -1) {

        switch (opt) {
            case 'l':
                // Handle -l option
                l_option = true;
                break;
            case 'm':
                // Handle -m option
                m_option = true;
                break;
            case 's':
                // Handle -s option
                s_option = true;
                break;
            case 'p':
                // Handle -p option
                p_option = true;
                break;
            case '?':
                fprintf(stderr, "usage: myfinger [-lmps] [login ...]\n");
                return 1;
            default:
                // Handle other errors
                abort();
        }

    }
    for(int i = 1; i < argc; ++i){
        if (strncmp(argv[i], "-", 1) == 0){
            continue;
        } else{
            bool try = false;
            for(int j = 0; j < names_count; ++j){
                if(strcmp(names[j], argv[i]) == 0){
                    try = true;
                }
            }
            if(try){
                continue;
            }
            names = (char**)realloc(names, (names_count + 1) * sizeof(char*));
            if (names == NULL) {
                fprintf(stderr, "Errore durante l'allocazione di memoria per i nomi.\n");
                return 1;
            }

            names[names_count++] = argv[i];
        }
    }

    /*  casi possibili:
        - nessun nome: cerco gli utenti attivi, quindi prima UTMP e poi PASSWD
            -s di default
            -m non ha effetto
            -l solo se specificato
            -p solo se -l specificato
        - con nome: cerco tra tutti gli utenti, quindi prima PASSWD e poi UTMP
            -l di default
            -m ha effetto
            -s solo se specificato
            -p solo se non è specificato -s
    */

    if (names_count == 0){
        handle_no_names();
    } else {
        handle_names(names, names_count);
    }
    return 0;
}

void handle_no_names(){ //short
    getActiveUsers();
}

void handle_names(char** names, int names_count){ //long
    for (int i = 0; i < names_count; i++) {
        if(m_option){
            //ricerco gli utenti solo per username
            getSpecifiedUser(names[i], true);
        } else {
            //ricerco gli utenti anche per nome in gecos
            getSpecifiedUser(names[i], false);
        }
    }
    free(names);
}


void getActiveUsers(){
    s_option = true;
    struct utmp* ut;
    setutent(); // Imposta il puntatore all'inizio del file /run/utmp
    char encounteredUsers[MAX_USERS][MAX_NAME_LENGTH]; // Array per memorizzare gli utenti già incontrati
    int numEncounteredUsers = 0; // Contatore degli utenti già incontrati
    bool userEncountered = false;

    while ((ut = getutent()) != NULL) {
        if (ut->ut_type == USER_PROCESS) {
            char* user = ut->ut_user;
            for (int i = 0; i < numEncounteredUsers; ++i) {
                if (strcmp(encounteredUsers[i], user) == 0) {
                    userEncountered = true;
                    break;
                }
            }
            if (!userEncountered) {
                strncpy(encounteredUsers[numEncounteredUsers], user, UT_NAMESIZE);
                ++numEncounteredUsers;
                userEncountered = false;
                if(m_option){
                    getSpecifiedUser(user, true);
                } else {
                    getSpecifiedUser(user, false);
                }
            }
        }
    }
    endutent();
}


void getSpecifiedUser(const char* user, bool option){
    struct passwd *pw;
    setpwent();
    bool user_found = false; // If the user exists
    while((pw = getpwent()) != NULL){
        char* gecos = strdup(pw->pw_gecos);
        char* name = strsep(&gecos, ",");
        char* singleName;
        bool passwd_found = false; // To check if the current passwd entry match the user
        // Confronto esatto tra il nome utente e il campo pw_name
        if(strcmp(user, pw->pw_name) == 0){
            passwd_found = true;
        }
        if(!option){
            // Confronto esatto tra il nome utente e il nome nel campo gecos
            if(strcasecmp(user, name) == 0){
                passwd_found = true;
            }
            // Verifica se ci sono ulteriori nomi all'interno del campo gecos
            while((singleName = strsep(&name, " "))){
                // Confronto esatto tra il nome utente e i nomi all'interno del campo gecos
                if(strcasecmp(user, singleName) == 0){
                    passwd_found = true;
                }
            }
        }

        //se in questa entry ho trovato il nome dell'utente
        if(passwd_found){
            //PRINT THE BEGINNING OF LONG OPTION FOR THAT USER
            if(!s_option || (l_option && s_option)){
                printStartL(pw);
            }
            user_found = true;
            struct utmp* ut;
            setutent();
            bool utmp_found = false;
            while ((ut = getutent()) != NULL) {
                if (ut->ut_type == USER_PROCESS && strncmp(ut->ut_user, user, UT_NAMESIZE) == 0) {
                    if(!s_option || (l_option && s_option)){
                        printLong(pw, ut);
                    } else{
                        printShort(pw, ut);
                    }
                    utmp_found = true;
                }
            }
            endutent();


            if (!utmp_found) {
                // Trying to search for the user in the wtmp log file
                struct utmp wt;
                int wtmp_fd;
                time_t last_login_time = 0;
                char* shell = (char*)malloc(sizeof(char) * 20);

                if ((wtmp_fd = open(MIAO, O_RDONLY)) == -1) {
                        perror("Errore nell'apertura del file wtmp");
                        exit(EXIT_FAILURE);
                }

                strcpy(shell,"");

                while (read(wtmp_fd, &wt, sizeof(struct utmp)) == sizeof(struct utmp)) {
                    if (strncmp(wt.ut_name, user, UT_NAMESIZE) == 0 && wt.ut_type == USER_PROCESS) {
                        if (wt.ut_tv.tv_sec > last_login_time) {
                            last_login_time = wt.ut_tv.tv_sec;
                            strncpy(shell, wt.ut_line, 20);
                        }
                    }
                }
                if (last_login_time != 0 ) {
                    char* time = formatTime(last_login_time, true);
                    printf("Last login %s on %s\n", time, shell);
                    free(time);
                } else {
                    if(s_option){
                        printShort(pw, NULL);
                    } else{
                        printLong(pw, NULL);
                    }
                }

                close(wtmp_fd);
            }

            //PRINT THE END OF LONG OPTION FOR THAT USER
            if(!s_option || (l_option && s_option)){
                printEndL(pw);
            }
        }
    }
    endpwent();
    if(!user_found){
        printf("myfinger: %s: no such user.\n", user);
    }
}











/*
CREARE FUNZIONI CHE, IN BASE AD UN BOOLEANO, RITORNANO LA STRINGA FATTA PER IL -s O PER IL -l
*/















void printShort(const struct passwd* pw, const struct utmp* ut){
    if(!intestazione){
        printf("Login\t  Name\t\t   Tty\t    Idle  Login Time   Office\t  Office Phone\n");
        intestazione = true;
    }
    printf("%-10s", pw->pw_name);
    char* gecos = strdup(pw->pw_gecos); // Duplica la stringa per evitare modifiche dirette
    char* name = strsep(&gecos, ",");
    if (name != NULL){
        printf("%-.16s", name);
    } else {
        printf("\t\t");
    }
    if(ut != NULL){
        const char* teletype = ut->ut_line;
        if (checkAsterisk(teletype)){
            printf(" %-10s\n", teletype);
        } else {
            printf("*%-9s\n", teletype);
        }
    } else {
        printf("\n");
    }
}


void printStartL(const struct passwd* pw){
    char* login = pw->pw_name;
    char* gecos = strdup(pw->pw_gecos); // Duplica la stringa per evitare modifiche dirette
    if(gecos != NULL){
        char* name = strsep(&gecos, ",");
        char* office = strsep(&gecos, ",");
        if (office == NULL) {
            office = strdup("");
        }
        
        char* workNumber = strsep(&gecos, ",");
        if (workNumber == NULL) {
            workNumber = strdup("");
        } else {
            workNumber = formatPhoneNumber(workNumber);
        }

        char* homeNumber = strsep(&gecos, ",");
        if (homeNumber == NULL) {
            homeNumber = strdup("");
        } else {
            homeNumber = formatPhoneNumber(homeNumber);
        }


        if (name != NULL){
            printf("Login: %-32s Name: %s\n", login, name);
        } else {
            printf("Login: %-32s Name:\n", login);
        }
        printf("Directory: %-28s Shell: %-23s\n", pw->pw_dir, pw->pw_shell);

        //printNumberOffice(gecos, true); //true for L option, false for S option

        
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

void printLong(const struct passwd* pw, const struct utmp* ut){
    if(ut != NULL){
        printSpecificUTMP(ut);
    } else {
        printf("Never logged in.\n");
    }
}

void printEndL(const struct passwd* pw){
    if(p_option){
        printf("non sto stampando il Plan\n");
    } else {

    }
}



bool checkAsterisk(const char* line){
    char file_path[256];
    // Copy /dev/ into the buffer
    strcpy(file_path, "/dev/");

    // Concatena ut->ut_line al buffer
    strcat(file_path, line);
    //printf("\n%s\n", file_path);

    struct stat file_stat;
    if (stat(file_path, &file_stat) == 0) {
        // Check if group or others have write permission
        if (file_stat.st_mode & (S_IWOTH | S_IWGRP)) {
            return true;
        }
    }
    return false;
}

void printSpecificUTMP(const struct utmp* ut) {
    time_t login_time = ut->ut_tv.tv_sec;
    char* formatted_login_time = formatTime(login_time, true);
    if (formatted_login_time == NULL) {
        printf("Errore durante la formattazione del login time\n");
    } else {
        printf("On since %s", formatted_login_time);
        free(formatted_login_time);
    }

    printf(" on %-5s", ut->ut_line);
    char* formatted_idle_time = formatTime(login_time, false);

    if (formatted_idle_time != NULL) {

        const char* teletype = ut->ut_line;
        if (checkAsterisk(teletype)){
            if(strcmp(ut->ut_host, "") != 0){
                printf(" from %s\n", ut->ut_host);
            }
            printf("%s\n", formatted_idle_time);
        } else {
            printf("%s\n     (messages off)\n", formatted_idle_time);
        }

        free(formatted_idle_time);
    } else {
        printf("Errore durante la formattazione dell'idle time\n");
    }
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
        if (diff_seconds < 15552000) {
            //15552000
            strftime(time_buffer, 100, "%a %b %d %H:%M (%Z)", time_info);
            // Formatta la data e l'ora con l'anno e memorizzale nel buffer
        } else {
            // Se sono trascorsi meno di 6 mesi, formatta la data e l'ora con ore e minuti
            strftime(time_buffer, 100, "%a %b %Y (%Z)", time_info);
        }
    } else {
        int hours = diff_seconds / 3600;
        int minutes = ((int)diff_seconds % 3600) / 60;
        sprintf(time_buffer, "    %d hour %d minutes idle", hours, minutes);
    }

    return time_buffer;
}

char* formatPhoneNumber(const char* phoneNumber){
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
