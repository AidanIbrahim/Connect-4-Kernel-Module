#include "connect4user.h"

#define BOARD_HEIGHT 6    //Size of the gameboard
#define BOARD_WIDTH 6    //Size of the gameboard

int main(int argc, char *argv[]){
    int running = 1;    //This will tell the prompter to keep prompting
    char userInput[100];    //Houses commands from the user, 100 us just arbitrary for now
    setlocale(LC_ALL, "en_US.UTF-8");   //For better graphics


    int writer = open("/dev/fourinarow", O_WRONLY);
    
    ssize_t written;
    write(writer, "BOARD\n", strlen(userInput));    //Initial print before loop
    printBoard();

    while (running) {
        printf("Connect4 >>: ");
        fgets(userInput, sizeof(userInput), stdin);

        if (strlen(userInput) > 99){
            printf("Connect4 <<: Command too long\n");
        } else {
            write(writer, userInput, strlen(userInput));    //Always print to terminal because I dont feel like typing a morbillion commands to see the board
            write(writer, "BOARD\n", strlen(userInput));
            printBoard();
        }



        if (!strcmp(userInput, "QUIT\n")) { //Exits the program
            running = 0;
        }
    }
    return 0;
} 

void printBoard(){
    int reader = open("/dev/fourinarow", O_RDONLY);
    setlocale(LC_CTYPE, "en_US.UTF-8");
    char ch;
    ssize_t bytesRead;

    while ((bytesRead = read(reader, &ch, 1)) > 0) {
        if (ch == '0'){
            wchar_t out = L'⚪';
            printf("\x1b[44m%lc\x1b[0m", out);
        } else if (ch == 'R'){
            wchar_t out = L'🔴';
            printf("\x1b[44m%lc\x1b[0m", out);
        } else if (ch == 'Y'){
            wchar_t out = L'🟡';
            printf("\x1b[44m%lc\x1b[0m", out);
        } else {
            putchar(ch);
        }
    }
    printf("\x1b[0m\n");
}