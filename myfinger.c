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
            getSpecifiedUser(names[i]);
        } else {
            //ricerco gli utenti anche per nome in gecos
            getSpecifiedAll(names[i]);
        }
    }
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
                if(!m_option){
                    getSpecifiedAll(user);
                } else {
                    getSpecifiedUser(user);
                }
            }
        }
    }
    endutent();
}


void getSpecifiedAll(char* user){
    struct passwd *pw;
    setpwent();
    bool user_found = false; // If the user exists
    while((pw = getpwent()) != NULL){
        char* gecos = strdup(pw->pw_gecos);
        char* name = strsep(&gecos, ",");
        char* singleName;
        bool passwd_found = false; // To check if the current passwd entry match the user
        // Confronto esatto tra il nome utente e il campo pw_name
        if(strcasecmp(user, pw->pw_name) == 0){
            passwd_found = true;
        }
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
// ################ TODO ############ se non ho trovato l'utente in utmp, provo in wtmp per il last login
    

//ALLO A FUNZIONARE FUNZIONA, MO VA ADATTATO, SIUM

    struct utmp ut;
    int wtmp_fd;
    time_t last_login_time = 0;
    char* target_username = user; // Cambia "target_user" con l'username desiderato

    if ((wtmp_fd = open(MIAO, O_RDONLY)) == -1) {
        perror("Errore nell'apertura del file wtmp");
        exit(EXIT_FAILURE);
    }

    while (read(wtmp_fd, &ut, sizeof(struct utmp)) == sizeof(struct utmp)) {
        if (strncmp(ut.ut_name, target_username, UT_NAMESIZE) == 0 && ut.ut_type == USER_PROCESS) {
            // Trovato un match per l'utente
            if (ut.ut_tv.tv_sec > last_login_time) {
                last_login_time = ut.ut_tv.tv_sec;
            }
        }
    }

    if (last_login_time != 0) {
        // Stampiamo il last login time
        printf("Last login time per l'utente %s: %s", target_username, formatTime(last_login_time, true));
    } else {
        printf("Nessun login trovato per l'utente %s\n", target_username);
    }

    close(wtmp_fd);








                if(s_option){
                    printShort(pw, NULL);
                } else{
                    printLong(pw, NULL);
                }
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



void getSpecifiedUser(const char* user){ // -m
    struct passwd* pw;
    if ((pw = getpwnam(user)) != NULL) {
        struct utmp* ut;
        setutent();
        int userFound = false;
        bool printed = false;
        while ((ut = getutent()) != NULL) {
            if (!printed){
                printStartL(pw);
                printed = true;
            }
            if (ut->ut_type == USER_PROCESS && strncmp(ut->ut_user, user, UT_NAMESIZE) == 0) {
                if(s_option){
                    printShort(pw, ut);
                } else{
                    printLong(pw, ut);
                }
                userFound = true;
            }
        } 
        endutent();

        // Esegui printLong(pw, NULL); solo se nessun utente è stato trovato
        if (!userFound) {
            if(s_option){
                printShort(pw, NULL);
            } else{
                if(!printed){
                    printStartL(pw);
                    printed = true;
                }
                printLong(pw, NULL);
            }
        }
    } else {
        printf("myfinger: %s: no such user.\n", user); //METTERE ANCHE SOPRA
    }
}


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
    char* name = strsep(&gecos, ",");
    if (name != NULL){
        printf("Login: %-32s Name: %s\n", login, name);
    } else {
        printf("Login: %-32s Name:\n", login);
    }
    printf("Directory: %-28s Shell: %-23s\n", pw->pw_dir, pw->pw_shell);


    char* office = strsep(&gecos, ",");
    strcpy(office, (office != NULL) ? office : "");

    char* workNumber = strsep(&gecos, ",");
    strcpy(workNumber, (workNumber != NULL) ? workNumber : "");

    char* homeNumber = strsep(&gecos, ",");
    strcpy(homeNumber, (homeNumber != NULL) ? homeNumber : "");


    char* finalString =  (char*)malloc(250 * sizeof(char));

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
    } else {
        if(strcmp(finalString, "") != 0){
            strcat(finalString, "\n");
        }
    }
    printf("%s",finalString);

//########## TODO ################# SISTEMARE #########################
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
        printf("%s", formatted_login_time);
        free(formatted_login_time);
    }

    printf(" on %s", ut->ut_line);
    char* formatted_idle_time = formatTime(login_time, false);

    if (formatted_idle_time != NULL) {

        const char* teletype = ut->ut_line;
        if (checkAsterisk(teletype)){
            printf(" from %s\n", ut->ut_host);
            printf("%s", formatted_idle_time);
        } else {
            printf("%s     (messages off)\n", formatted_idle_time);
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
        sprintf(time_buffer, "    %d hour %d minutes idle\n", hours, minutes);
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
