#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <ftw.h>

#define BUFFER 4096
#define myPID getpid()

char *globalSrc = "";
char *globalDest = "";

/* functions */
void help_command(int tokensNumber, char *tokens[]) {
    if(tokensNumber > 1) {
        perror("Komenda help nie ma argumentów");
    }
    else {
        printf("*** Microshell SOP ***\n");
        printf("Autor: Patryk Bartkowiak\n");
        printf("Dostepne komendy: \n-exit \n-help \n-echo \n-cd \n-pwd \n-cat (czytanie i zapisywanie do plików, np. cat something >> file.txt) \n-pid \n-ppid \n-ls \n-cp \n-rename \n-mkdir \n-calc (prosty kalkulator) \n-wszystkie programy, ktore obsluguje podstawowy shell\n");
        printf("Bajery: \n-nazwa uzytkownika \n-kolory \n-historia \n-autouzupelnianie \n-kontrola sygnalow");
    }
    exit(EXIT_SUCCESS);
}
void echo_command(int tokensNumber, char *tokens[]) {
    int i;
    if( strcmp(tokens[tokensNumber - 2], ">>") != 0 )
        for(i = 1; i < tokensNumber; i++) {
	    if(tokens[i][strlen(tokens[i])-1] == '"')
		memmove(&tokens[i][strlen(tokens[i])-1], &tokens[i][strlen(tokens[i])], strlen(tokens[i]) - strlen(tokens[i]-1));
	    if(tokens[i][0] == '"')
		memmove(&tokens[i][0], &tokens[i][1], strlen(tokens[i]) - 0);
            printf("%s ", tokens[i]);
        }
    else {
        int i;
        FILE *fp;
        fp = fopen(tokens[tokensNumber - 1], "w");
        if (fp == NULL) {
            perror("");
            exit(EXIT_FAILURE);
        }
        for(i = 1; i < tokensNumber - 2; i++) {
            fprintf(fp, tokens[i], i+1);
            fputs(" ", fp);
        }
        fclose(fp);
    }
    exit(EXIT_SUCCESS);
}
void cd_command(int tokensNumber, char *tokens[]) {
    if(tokensNumber <= 2) {
        char *homedir = getenv("HOME");
        if( tokensNumber == 1 || strcmp(tokens[1], "~") == 0)
            chdir(homedir);
        else if(chdir(tokens[1]) == -1)
            perror("Nie mozna otworzyc katalogu");
    }
    else
        perror("Za duzo argumentów");
}
void pwd_command(int tokensNumber, char *tokens[]) {
    system("pwd");
    exit(EXIT_SUCCESS);
}
void rename_command(int tokensNumber, char *tokens[]) {
    if(tokensNumber == 3) {
        rename(tokens[1], tokens[2]);
        printf("Nazwa pliku %s została zmieniona na %s", tokens[1], tokens[2]);
        exit(EXIT_SUCCESS);
    }
    else {
        printf("Nieodpowiednia ilosc argumentow!");
        exit(EXIT_FAILURE);
    }
}
void cat_command(int tokensNumber, char *tokens[]) {
    if( strcmp(tokens[tokensNumber - 2], ">>") != 0 ) {
        int i = 1;
        char ch;
        while( strcmp(tokens[i], "") != 0 ) {
            FILE *f = fopen(tokens[i], "r");
            if (f == NULL) {
                perror("");
                exit(EXIT_FAILURE);
            }
            else {
                ch = fgetc(f);
                while(ch != EOF) {
                    printf("%c", ch);
                    ch = fgetc(f);
                }
                i++;
            }
            fclose(f);
        }
    }
    else {
        int i;
        FILE *fp;
        fp = fopen(tokens[tokensNumber - 1], "w");
        if (fp == NULL) {
            perror("");
            exit(EXIT_FAILURE);
        }
        for(i = 1; i < tokensNumber - 2; i++) {
            fprintf(fp, tokens[i], i+1);
            fputs(" ", fp);
        }
        fclose(fp);
    }
    exit(EXIT_SUCCESS);
}
void pid_command(int tokensNumber, char *tokens[]) {
    if(tokensNumber == 1) {
        printf("pid: %i", getpid());
        exit(EXIT_SUCCESS);
    }
    else {
        printf("Ta komenda nie powinna zawierac dodatkowych argumentow");
        exit(EXIT_FAILURE);
    }
}
void ppid_command(int tokensNumber, char *tokens[]) {
    if(tokensNumber == 1) {
        printf("ppid: %i", getppid());
        exit(EXIT_SUCCESS);
    }
    else {
        printf("Ta komenda nie powinna zawierac dodatkowych argumentow");
        exit(EXIT_FAILURE);
    }
}
void ls_command(int tokensNumber, char *tokens[]) {
    struct dirent *directories;
    DIR *dr;
    if(tokensNumber > 1)
        dr = opendir(tokens[1]);
    else
        dr = opendir(".");
    while((directories = readdir(dr)) != NULL) {
        printf("%s\n", directories->d_name);
    }
    closedir(dr);
    exit(EXIT_SUCCESS);
}
bool isDir(char *name) {
    DIR *directory = opendir(name);
    if(errno == ENOTDIR)
        return false;
    closedir(directory);
    return true;
}
void kopiowanie(const char *src, char *dest) {
    FILE *fptr1, *fptr2;
    char content;
    fptr1 = fopen(src, "r");
    fptr2 = fopen(dest, "w");
    if(fptr1 == NULL) {
        perror("Nie mozna otworzyc jednego z plikow");
        exit(EXIT_FAILURE);
    }
    else if(fptr2 == NULL) {
        perror("Nie mozna otworzyc jednego z plikow");
        exit(EXIT_FAILURE);
    }
    content = fgetc(fptr1);
    while(content != EOF) {
        fputc(content, fptr2);
        content = fgetc(fptr1);
    }
    fclose(fptr1);
    fclose(fptr2);
}
int copying(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf) {
    int len = strlen(globalSrc);
    char tempDest[BUFFER] = "";
	tempDest[0] = '\0';
    strcat(tempDest, globalDest);
    strcat(tempDest, fpath + len);
    if( ftwbuf->level == 0 ) {
        mkdir(tempDest, 0777);
    }
    else {
        if( tflag == FTW_D ) {
            mkdir(tempDest, 0777);
        }
        else {
            kopiowanie(fpath, tempDest);
        }
    }
    return 0;
}
int rmrf(char *path) {
    return nftw(path, copying, 64, FTW_PHYS);
}
void cp_command(int tokensNumber, char *tokens[]) {
    if(tokensNumber == 4) {
	globalSrc = tokens[2];
        globalDest = tokens[3];
        rmrf(tokens[2]);
	globalSrc = "";
        globalDest = "";
        exit(EXIT_SUCCESS);
    }
    else if(tokensNumber == 3) {
        if( isDir(tokens[2]) ) {
            FILE *fptr1, *fptr2;
            char content;
            char path[BUFFER];
            getcwd(path, sizeof(path));
            strcat(path, "/");
            strcat(path, tokens[2]);
            strcat(path, "/");
            strcat(path, tokens[1]);
            fptr1 = fopen(tokens[1], "r");
            fptr2 = fopen(path, "w");
            if(fptr1 == NULL) {
                perror("Nie mozna otworzyc jednego z plikow");
                exit(EXIT_FAILURE);
            }
            else if(fptr2 == NULL) {
                perror("Nie mozna otworzyc jednego z plikow");
                exit(EXIT_FAILURE);
            }
            content = fgetc(fptr1);
            while(content != EOF) {
                fputc(content, fptr2);
                content = fgetc(fptr1);
            }
            fclose(fptr1);
            fclose(fptr2);
            printf("Zawartosc skopiowana z %s do %s", tokens[1], path);
            exit(EXIT_SUCCESS);
        }
        else {
            FILE *fptr1, *fptr2;
            char content;
            fptr1 = fopen(tokens[1], "r");
            fptr2 = fopen(tokens[2], "w");
            if(fptr1 == NULL) {
                perror("Nie mozna otworzyc jednego z plikow");
                exit(EXIT_FAILURE);
            }
            else if(fptr2 == NULL) {
                perror("Nie mozna otworzyc jednego z plikow");
                exit(EXIT_FAILURE);
            }
            content = fgetc(fptr1);
            while(content != EOF) {
                fputc(content, fptr2);
                content = fgetc(fptr1);
            }
            fclose(fptr1);
            fclose(fptr2);
            printf("Zawartosc skopiowana z %s do %s", tokens[1], tokens[2]);
            exit(EXIT_SUCCESS);
        }
    }
    else {
        printf("Nieodpowiednia ilosc argumentow!");
        exit(EXIT_FAILURE);
    }
}
void mkdir_command(int tokensNumber, char *tokens[]) {
    if(tokensNumber > 1) {
        if( strcmp(tokens[1], "-p") != 0 ) {
            int i;
            for(i = 1; i < tokensNumber; i++) {
                if( mkdir(tokens[i], 0777) == -1) {
                    perror("");
                    exit(EXIT_FAILURE);
                }
            }
            if(tokensNumber > 2)
                printf("Foldery zostaly utworzone");
            else
                printf("Folder zostal utworzony");
        }
        else {
            char path[200];
            getcwd(path, sizeof(path));
            char *folder;
            folder = strtok(tokens[2], "/");
            while(folder != NULL) {
                if(folder[0] != '{') {
                    if( mkdir(folder, 0777) == -1) {
                        perror("");
                        exit(EXIT_FAILURE);
                    }
                    chdir(folder);
                    folder = strtok(NULL, "/");
                }
                else {
                    int i;
                    int len = strlen(folder);
                    /* usuniecie {} z nazwy folderu */
                    for(i = 1; i < len-1; i++)
                        folder[i-1] = folder[i];
                    folder[i-1] = '\0';
                    char *element;
                    element = strtok(folder, ",");
                    while(element != NULL) {
                        if( mkdir(element, 0777) == -1) {
                            perror("");
                            exit(EXIT_FAILURE);
                        }
                        element = strtok(NULL, ",");
                    }
                    folder = strtok(NULL, "/");
                }
            }
            chdir(path);
            printf("Foldery zostaly utworzone");
        }
        exit(EXIT_SUCCESS);
    }
    else {
        printf("Nie podano nazww folderow do utworzenia");
        exit(EXIT_FAILURE);
    }
}
void mv_command(int tokensNumber, char *tokens[]) {
    if(tokensNumber == 3) {
        FILE *fptr1, *fptr2;
        char content;
        fptr1 = fopen(tokens[1], "r");
        fptr2 = fopen(tokens[2], "w");
        if(fptr1 == NULL) {
            perror("Nie mozna otworzyc jednego z plikow");
            exit(EXIT_FAILURE);
        }
        else if(fptr2 == NULL) {
            perror("Nie mozna otworzyc jednego z plikow");
            exit(EXIT_FAILURE);
        }
        content = fgetc(fptr1);
        while(content != EOF) {
            fputc(content, fptr2);
            content = fgetc(fptr1);
        }
        fclose(fptr1);
        fclose(fptr2);
        unlink(tokens[1]);
        printf("Zawartosc przeniesiona z %s do %s", tokens[1], tokens[2]);
        exit(EXIT_SUCCESS);
    }
    else {
        printf("Nieodpowiednia ilosc argumentow!");
        exit(EXIT_FAILURE);
    }
}
void sig_handler(int sig) {
    pid_t currentPID = getpid();
    signal(sig, SIG_DFL);
    if(currentPID == myPID) {
        if(sig == SIGINT)
            signal(sig, SIG_IGN);
        if(sig == SIGTSTP)
            signal(sig, SIG_IGN);
    }

}
void calc_command(int tokensNumber, char *tokens[]) {
    if(tokensNumber < 4) {
        printf("Za malo argumentow!");
        exit(EXIT_FAILURE);
    }
    else if(tokensNumber > 4) {
        printf("Za duzo argumentow! program calc liczy proste obliczenia");
        exit(EXIT_FAILURE);
    }
    else {
        float num1 = atof(tokens[1]);
        float num2 = atof(tokens[3]);
        if( strcmp(tokens[2], "+") == 0 )
            printf("%g %s %g = %g", num1, tokens[2], num2,  num1 + num2);
        if( strcmp(tokens[2], "-") == 0 )
            printf("%g %s %g = %g", num1, tokens[2], num2,  num1 - num2);
        if( strcmp(tokens[2], "*") == 0 )
            printf("%g %s %g = %g", num1, tokens[2], num2,  num1 * num2);
        if( strcmp(tokens[2], "/") == 0 )
            printf("%g %s %g = %g", num1, tokens[2], num2,  num1 / num2);
    }
    exit(EXIT_SUCCESS);
}


int main() {
    char *command;
    char *tokens[BUFFER];
    int tokensNumber;
    char *username=getenv("USER");
    signal(SIGINT, sig_handler);
    signal(SIGTSTP, sig_handler);

    while(1) {
        memset(tokens, 0, sizeof(tokens));
        tokensNumber = 0;
        char path[200];
        getcwd(path, sizeof(path));
        printf("[");
        printf("\033[0;33m");
        printf("%s: ", username);
        printf("\033[0;32m");
        printf("%s", path);
        printf("\033[0m");
        printf("]\n");
        command = readline("$ ");
        if(strlen(command) > 0)
            add_history(command);

        char *token;

        token = strtok(command, " ");
        int i = 0;
        while(token != NULL) {
            tokens[i] = token;
            token = strtok(NULL, " ");
            i++;
            tokensNumber++;
        }

        if( strcmp(tokens[0], "exit") == 0 )
            exit(EXIT_SUCCESS);
        else if( strcmp(tokens[0], "cd") == 0 )
            cd_command(tokensNumber, tokens);
        else {
            int pid = fork();
            if(pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            else if(pid == 0) {
                if(strcmp(tokens[0], "help") == 0)
                    help_command(tokensNumber, tokens);
                else if (strcmp(tokens[0], "echo") == 0)
                    echo_command(tokensNumber, tokens);
                else if (strcmp(tokens[0], "pwd") == 0)
                    pwd_command(tokensNumber, tokens);
                else if (strcmp(tokens[0], "cat") == 0)
                    cat_command(tokensNumber, tokens);
                else if (strcmp(tokens[0], "pid") == 0)
                    pid_command(tokensNumber, tokens);
                else if (strcmp(tokens[0], "ppid") == 0)
                    ppid_command(tokensNumber, tokens);
                else if (strcmp(tokens[0], "ls") == 0)
                    ls_command(tokensNumber, tokens);
                else if( strcmp(tokens[0], "cp") == 0 )
                    cp_command(tokensNumber, tokens);
                else if( strcmp(tokens[0], "mkdir") == 0 )
                    mkdir_command(tokensNumber, tokens);
                else if( strcmp(tokens[0], "rename") == 0 )
                    rename_command(tokensNumber, tokens);
                else if( strcmp(tokens[0], "mv") == 0 )
                    mv_command(tokensNumber, tokens);
                else if( strcmp(tokens[0], "calc") == 0 )
                    calc_command(tokensNumber, tokens);
                else
                    if(execvp(tokens[0], tokens) == -1) {
                        perror("Blad polecenia");
                        exit(EXIT_FAILURE);
                    }
            }
            else {
                waitpid(pid, NULL, 0);
            }
        }
        free(command);
        printf("\n\n");
    }
}
