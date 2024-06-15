#include "myfinger.h"

struct UserUTMP;

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
    s_option = true;
    getActiveUsers();
}

void handle_names(char** names, int names_count){ //long
    char** copies = (char**)malloc(sizeof(char*)*names_count+2);
    if (copies == NULL) {
        fprintf(stderr, "Errore durante l'allocazione di memoria per l'aggiunta di un nome.\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < names_count; i++) {
        getSpecifiedUser(names[i], copies);
    }
    free(names);
    free(copies);
}



void getActiveUsers(){
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
                getSpecifiedUser(user, NULL);
            }
        }
    }
    endutent();

}


void getSpecifiedUser(const char* user, char** copies) {
    struct passwd *pw;
    setpwent();
    bool user_found = false; // If the user exists
    while((pw = getpwent()) != NULL) {
        char* gecos = strdup(pw->pw_gecos);
        char* name = strsep(&gecos, ",");
        char* singleName;
        bool passwd_found = false; // To check if the current passwd entry matches the user
        // Confronto esatto tra il nome utente e il campo pw_name
        if(strcmp(user, pw->pw_name) == 0) {
            passwd_found = true;
        }
        if(!m_option) {
            // Confronto esatto tra il nome utente e il nome nel campo gecos
            if(strcasecmp(user, name) == 0) {
                passwd_found = true;
            }
            // Verifica se ci sono ulteriori nomi all'interno del campo gecos
            while((singleName = strsep(&name, " "))) {
                // Confronto esatto tra il nome utente e i nomi all'interno del campo gecos
                if(strcasecmp(user, singleName) == 0) {
                    passwd_found = true;
                }
            }
        }

        //se in questa entry ho trovato il nome dell'utente
        if(passwd_found) {
            //se ho trovato almeno un utente con quel nome, allora non devo stampare che non esiste
            user_found = true;

            if(checkPresence(pw->pw_name, copies)) {
                continue;
            }

            if(!s_option || (l_option && s_option)) {
                printStartL(pw);
            } else {
                printStartS();
            }

            UserUTMP* userUTMP = malloc(sizeof(UserUTMP));
            if (userUTMP == NULL) {
                fprintf(stderr, "Failed to allocate memory for userUTMP\n");
                exit(EXIT_FAILURE);
            }
            memset(userUTMP, 0, sizeof(UserUTMP));

            struct utmp* ut;
            setutent();
            bool utmp_found = false;
            bool wtmp_print = true;
            while ((ut = getutent()) != NULL) {
                if (ut->ut_type == USER_PROCESS && strncmp(ut->ut_user, user, UT_NAMESIZE) == 0) {
                    userUTMP->time = ut->ut_tv.tv_sec;
                    strncpy(userUTMP->tty, ut->ut_line, sizeof(userUTMP->tty));
                    strncpy(userUTMP->host, ut->ut_host, sizeof(userUTMP->host));

                    if(!s_option || (l_option && s_option)) {
                        printLong(userUTMP, !wtmp_print);
                    } else {
                        printShort(pw, userUTMP, !wtmp_print);
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
                char* shell = malloc(sizeof(char) * 20);

                if ((wtmp_fd = open(WTMP_FILE, O_RDONLY)) == -1) {
                    perror("Errore nell'apertura del file wtmp");
                    exit(EXIT_FAILURE);
                }

                strcpy(shell, "");

                while (read(wtmp_fd, &wt, sizeof(struct utmp)) == sizeof(struct utmp)) {
                    if (strncmp(wt.ut_name, user, UT_NAMESIZE) == 0 && wt.ut_type == USER_PROCESS) {
                        if (wt.ut_tv.tv_sec > last_login_time) {
                            last_login_time = wt.ut_tv.tv_sec;
                            strncpy(shell, wt.ut_line, 20);
                            strncpy(userUTMP->tty, wt.ut_line, sizeof(userUTMP->tty));
                            strncpy(userUTMP->host, wt.ut_host, sizeof(userUTMP->host));
                            userUTMP->time = wt.ut_tv.tv_sec;
                        }
                    }
                }
                if (last_login_time != 0) {
                    if(!s_option || (l_option && s_option)) {
                        printLong(userUTMP, wtmp_print);
                    } else {
                        printShort(pw, userUTMP, wtmp_print);
                    }
                } else {
                    if(!s_option || (l_option && s_option)) {
                        printLong(NULL, false);
                    } else {
                        printShort(pw, NULL, false);
                    }
                }

                free(shell);
                close(wtmp_fd);
            }

            if(!s_option || (l_option && s_option)) {
                printEndL(pw);
            }

            free(userUTMP);
        }
    }
    endpwent();
    if(!user_found) {
        printf("myfinger: %s: no such user.\n", user);
    }
}




bool checkPresence(const char* name, char** copies) {
    if(copies != NULL){
        int count = 0;
        //printf("suca\n");
        while (copies[count] != NULL) {
            count++;
        }
        for (int i = 0; i < count; i++) {
            if (strcmp(name, copies[i]) == 0) {
                return true;
            }
        }
        copies[count] = strdup(name);
        if (copies[count] == NULL) {
            fprintf(stderr, "Errore durante la duplicazione del nome per l'aggiunta.\n");
            exit(EXIT_FAILURE);
        }
        copies[count + 1] = NULL;
    }
    return false;
}







//Potrei creare delle struct UserInfo e SessionInfo
//Passo UserInfo ad una funzione, poi da quella funzione cerco tutte le SessionInfo per quella funzione

//Nel caso long: Per ogni UserInfo stampo la parte fissa, poi cerco e stampo tutte le SessionInfo, poi alla
//fine stampo l'ultima parte dell'UserInfo, cioè il file .Plan

//Nel caso short: stampo la prima volta i nomi delle colonne. Poi stampo ogni volta la UserInfo (utente e nome) e
//una SessionInfo

// In generale, posso passare il passwd di ogni utente trovat alla funzione che stampa:
    // in caso di -l:
        //la parte iniziale comune, con utente, nome , ecc
        //le parti intermedie singole di ogni utmp
        //la parte finale comune, file .Plan
    // in caso di -s:
        //la parte fissa iniziale
        //La parte iniziale di ogni entry, cioè quella contenuta nella struct pw
        //per ogni print della parte iniziale di pw, la parte di utmp (ogni riga dello stesso utente è
            // un suo accesso)
        //






//LOGGED serve a capire se la data in utmp è il last login o il tempo di login
void printStartL(const struct passwd* pw){
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
            workNumber = formatPhoneNumber(workNumber);
        }

        char* homeNumber = strsep(&gecos, ",");
        if(homeNumber == NULL) {
            homeNumber = strdup("");
        } else {
            homeNumber = formatPhoneNumber(homeNumber);
        }


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

void printLong(UserUTMP* user, bool wtmp){
    if(user != NULL){
        time_t login_time = user->time;
        char* formatted_login_time = formatTime(login_time, true);
        if (formatted_login_time == NULL) {
            printf("Errore durante la formattazione del login time\n");
            strcpy(formatted_login_time, "");
        } else {
            if(!wtmp){
                printf("On since %s", formatted_login_time);
            } else {
                printf("Last login %s", formatted_login_time);
            }
            free(formatted_login_time);
        }
        printf(" on %-5s", user->tty);
        if(!wtmp){
            char* formatted_idle_time = formatTime(login_time, false);

            if (formatted_idle_time != NULL) {
                const char* teletype = user->tty;

                if (checkAsterisk(teletype)){
                    if(strcmp(user->host, "") != 0){
                        printf(" from %s\n", user->host);
                    }
                    printf("%s\n", formatted_idle_time);
                } else {
                    printf("%s\n     (messages off)\n", formatted_idle_time);
                }
                free(formatted_idle_time);

            } else {
                printf("Errore durante la formattazione dell'idle time\n");
            }
        } else {
            printf("\n");
        }
    } else {
        printf("Never logged in.\n");
    }
}

void printEndL(const struct passwd* pw){
    printf("No Plan.\n");
}

void printStartS(){
    if(!intestazione){
        printf("Login\t  Name\t\t    Tty\t     Idle  Login Time   Office\t   Office Phone\n");
        intestazione = true;
    }
}


void printShort(const struct passwd* pw, UserUTMP* userUTMP, bool wtmp){

    // FARE UNA STRUCT IN MODO DA NON DOVERLI RICREARE PER OGNI LOGIN DI OGNI PERSONA 
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
            workNumber = formatPhoneNumber(workNumber);
        }
    
        printf("%-10.10s", login);
        if (strcmp(name,"")!=0){
            printf("%-17.16s", name);
        } else {
            printf("                 ");
        }

        if(userUTMP != NULL){
            const char* teletype = userUTMP->tty;
            if (checkAsterisk(teletype)){
                printf(" %-9.10s", teletype);
            } else {
                printf("*%-9.9s", teletype);
            }
            if(!wtmp){
                char* idleTime = formatShortTime(userUTMP->time, false);
                printf("%-6.5s", idleTime);
                free(idleTime);
            } else {
                printf("   *  ");
            }
            char* loginTime = formatShortTime(userUTMP->time, true);
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
            printf("  *\t      *   No logins\n");
        }
    }
}

bool checkAsterisk(const char* line){
    char file_path[256];
    strcpy(file_path, "/dev/"); // Copy /dev/ into the buffer
    strcat(file_path, line); // Concatena ut->ut_line al buffer

    struct stat file_stat;
    if (stat(file_path, &file_stat) == 0) {
        // Check if group or others have write permission
        if (file_stat.st_mode & (S_IWOTH | S_IWGRP)) {
            return true;
        }
    }
    return false;
}



char* formatTime(const time_t time_seconds, bool isLogin) {
    time_t current_time = time(NULL);
    double diff_seconds = difftime(current_time, time_seconds);

    struct tm *time_info = localtime(&time_seconds);
    char* time_buffer = malloc(100 * sizeof(char));
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
        if(hours > 24){
            hours = hours / 24;
            sprintf(time_buffer, "    %d days idle", hours);
        } else {
            sprintf(time_buffer, "    %d hour %d minutes idle", hours, minutes);
        }
    }

    return time_buffer;
}


char* formatShortTime(const time_t time_seconds, bool isLogin) {
    time_t current_time = time(NULL);
    double diff_seconds = difftime(current_time, time_seconds);

    struct tm *time_info = localtime(&time_seconds);
    int size = 20;
    char* time_buffer = malloc(size * sizeof(char));
    if (time_buffer == NULL) {
        return NULL;
    }
    if (isLogin) {
        strftime(time_buffer, size, "%b %d %H:%M", time_info);
    } else {
        if (diff_seconds < 3600) { // Minore di un'ora
            // Stampo solo i minuti
            int minutes = (int)(diff_seconds / 60);
            snprintf(time_buffer, size, "%d", minutes);
        } else if (diff_seconds < 86400) { // Minore di un giorno
            // Stampo ore e minuti
            int hours = (int)(diff_seconds / 3600);
            int minutes = (int)(diff_seconds / 60) % 60;
            snprintf(time_buffer, 20, "%d:%02d", hours, minutes);
        } else if (diff_seconds < 31536000) { // Minore di un anno
            // Stampo i giorni
            int days = (int)(diff_seconds / 86400);
            snprintf(time_buffer, size, "%dd", days);
        } else { // Maggiore di un anno
            // Stampo gli anni
            int years = (int)(diff_seconds / 31536000);
            snprintf(time_buffer, size, "%dy", years);
        }
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
