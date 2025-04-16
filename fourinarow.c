#include "fourinarow.h"

#define DEVICE_NAME "fourinarow" // Name for the device file
#define BUFFER_SIZE 1024 // Size of buffer to store user input
#define MAJOR_NUM 0 //Kernel assigned
#define BOARD_HEIGHT 6    //Size of the gameboard
#define BOARD_WIDTH 7    //Size of the gameboard
#define CONNECT_TO_WIN 4    //How many chips must connect to win the game


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aidan Ibrahim");

static int major_num;
static int currTurn = 0;    //Tracks the current turn. 0 for red, 1 for yellow

static int cTurnID = 1;     //The ID of the computer, Ex. 0 means the computer plays as yellow
static int gameOver = 0;    //If this value is 1, the game is over  
static char cColor = 'R';
static char pColor = 'Y';
static char gameBoard[BOARD_HEIGHT][BOARD_WIDTH] = {{'0'}};

static struct cdev connect_four;    //Cdev setup
static struct class *cl;

static char cmd[BUFFER_SIZE] = {0};  // Buffer to store message from userspace
static char kout[BUFFER_SIZE] = {0};  // Buffer to store message from userspace
static size_t kout_len = 0;
static size_t cmd_len = 0;

static struct file_operations fops = {  //Read write operations setup
    .read = device_read,
    .write = device_write,
};

static char *devnode(struct device *dev, umode_t *mode){    //Gives all users read write access
    if (mode){
        *mode = 0666;
    }

    return NULL;
}

int init_module(void){  //Initalizes everything for the module
	printk(KERN_INFO "Initalizing connect4...\n");
    resetBoard('Y');    //Player is yellow by default
	//Register the character device
    major_num = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops);
    if (major_num < 0){
        printk(KERN_ALERT "Failed to register a major number\n");
        return major_num;
    }

    cl = class_create(THIS_MODULE, DEVICE_NAME);
    cl->devnode = devnode;
    if (IS_ERR(cl)){
        unregister_chrdev(major_num, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class");
        return PTR_ERR(cl);
    }

    // Create device node
    if (device_create(cl, NULL, MKDEV(major_num, 0), NULL, DEVICE_NAME) == NULL) {
        class_destroy(cl);
        unregister_chrdev(major_num, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create device\n");
        return -1;
    }

    // Initialize and add the character device
    cdev_init(&connect_four, &fops);
    if (cdev_add(&connect_four, MKDEV(major_num, 0), 1) < 0) {
        device_destroy(cl, MKDEV(major_num, 0));
        class_destroy(cl);
        unregister_chrdev(major_num, DEVICE_NAME);
        printk(KERN_ALERT "Failed to add cdev\n");
        return -1;
        }
	return 0;
}

ssize_t device_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset){   //Sends the output buffer to user
    size_t kout_len = strlen(kout);
    
    if (len > kout_len - *offset){
        len = kout_len - *offset;
    }

    if (copy_to_user(buffer, kout + *offset, len)){
        printk(KERN_ALERT "Read error\n");
        return -EFAULT;
    } 

    *offset += len;
    return len;
}

ssize_t device_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset){    //recieves user commands
    cmd_len = 0;

    // Clear the command buffer before copying new data
    memset(cmd, 0, sizeof(cmd)); 


    if (copy_from_user(cmd, buffer, len)){  //This will get the user input
        printk(KERN_ALERT "Write Error\n");
        return -EFAULT;
    }
    cmd_len = len;
    cmd[cmd_len] = '\0';

    executeCmd();

    return len;
}

void executeCmd(void){  //This executes the currently stored command
    int i = 0;
    int j = 0;
    int rIndex = 0;
    size_t len = 0; //Used for seperating the arguements
    char *space = strchr(cmd, ' '); //Get the string minus the first space

    if (space) {
        len = space - cmd;  // Length up to the first space
        *space = '\0'; // Temporarily null-terminate command for comparison
    } else {
        len = strlen(cmd);  // No space found, copy entire string
    }

    if (!strncmp(cmd, "BOARD\n", cmd_len)){ //Request for the board
        printk(KERN_INFO "OK");

            // Add column labels (A B C D ...)
        kout[rIndex++] = ' ';  // Tab before column labels
        for (i = 0; i < BOARD_WIDTH; i++) {
            kout[rIndex++] = 'A' + i;  // Column letters A to H
        }
        kout[rIndex++] = '\n';  // Newline after column labels
        
    
        for (i = 0; i < BOARD_HEIGHT; i++){   //Write to the read buffer
            kout[rIndex++] = '0' + (BOARD_HEIGHT - i);
            kout[rIndex++] = ' ';

            for (j = 0; j < BOARD_WIDTH; j++){
                char cell = gameBoard[i][j];  // Get the current cell value
                kout[rIndex++] = cell;
            }
            kout[rIndex++] = '\n';
        }
        kout[rIndex] = '\0';
        kout_len = rIndex;
    } else if (!strncmp(cmd, "RESET", len) && len == strlen("RESET")){
        char color = *(space + 1);  // Get the column letter
        resetBoard(color);

    } else if (!strncmp(cmd, "DROPC", len) && len == strlen("DROPC")){    //Drop a piece
        if (!gameOver){
            char col = *(space + 1);  // Get the column letter
            if (col > 'A' && col < 'A' + (BOARD_WIDTH)){\
                if (currTurn != cTurnID){   //Check if it is not the computers turn
                    printk(KERN_INFO "Dropping Piece in column %c\n", col);
                    if (dropPiece(col, pColor)){
                        printk(KERN_INFO "Illegal Move");
                        strscpy(kout, "Illegal Move\n", sizeof(kout));
                        
                    } else {
                        currTurn = (currTurn + 1) % 2;
                    }
                } else {
                    printk(KERN_INFO "OOT");
                    strscpy(kout, "OOT\n", sizeof(kout));
                }
                
            } else {
                printk(KERN_INFO "Illegal Move");
                strscpy(kout, "Illegal Move\n", sizeof(kout));
            }

            
        } else {
            printk(KERN_INFO "NOGAME");
            strscpy(kout, "NOGAME\n", sizeof(kout));
        }
    } else if (!strncmp(cmd, "CTURN\n", cmd_len)){
        if (!gameOver){
            if (currTurn == cTurnID){   //Check if it is not the computers turn
                int column = CPUmove(cColor); //Find a better method
                char col = 'A' + column;
                if (dropPiece(col,cColor)){ //Return value one means an illegal move
                    column = (column + 1) % BOARD_WIDTH; //Iterate to adjacent indexes if the attempted move is illegal
                    col = 'A' + column;
                }
                printk(KERN_INFO "Dropping Piece in column %c\n", col);
                strscpy(kout, "OK\n", sizeof(kout));
                currTurn = (currTurn + 1) % 2;
            } else {
                printk(KERN_INFO "OOT\n");
                strscpy(kout, "OOT\n", sizeof(kout));
            }
        }
    } else {
        printk(KERN_INFO "Unknown command\n");
        strscpy(kout, "Unknown command\n", sizeof(kout));
    }

}

int CPUmove(char color){
    int best = 0; //Highest score move
    int column = 0;
    int currScore = 0;
    int i;
    for (i = 0; i < BOARD_WIDTH; i++){  //Iterate through all the columns
        currScore = max(scoreMove(testDropPiece(i), i, cColor),  scoreMove(testDropPiece(i), i, pColor));   //Score opponent moves as well, as a high score move should be blocked
        if (currScore > best){
            best = currScore;
            column = i;
        }
    }

    return column;
} 

int scoreMove(int row, int column, char color){
    int i = 1;
    int score = 0;
    int scalingFactor = 3;
    int cScaling = scalingFactor;
    int cCount = 1;
    char oppColor;
    if (color == 'Y'){
        oppColor = 'R';
    } else {
        oppColor = 'Y';
    }
    if (row == -1){ //Negative score for illegal moves
        return -10000;
    }
    
    //Right Horizontal
    while ((column + i) < BOARD_WIDTH && i < CONNECT_TO_WIN){
        if (gameBoard[row][column + i] == color){
            score += cScaling;
            cScaling += scalingFactor; //Weights consecutive or close together pieces higher
            cCount++;
        } else if (gameBoard[row][column + i] == oppColor){
            score--;
            break;
        }
        i++;
    }

    i = 1;
    //Left Horizontal
    while ((column - i) >= 0 && i < CONNECT_TO_WIN){
        if (gameBoard[row][column - i] == color){
            score += cScaling;
            cScaling += scalingFactor; //Weights consecutive or close together pieces higher
            cCount++;
        } else if (gameBoard[row][column - i] == oppColor){
            score--;
            break;
        }
        i++;
    }
    
    if (cCount >= CONNECT_TO_WIN){   //Imminent win, play at all costs
        score += 1000;
    }

    i = 1;
    cCount = 1;
    cScaling = scalingFactor;
    //Vertical
    while ((row + i) < BOARD_HEIGHT && i < CONNECT_TO_WIN){
        if (gameBoard[row + i][column] == color){
            score += cScaling;
            cScaling += scalingFactor; //Weights consecutive or close together pieces higher
            cCount++;
        } else if (gameBoard[row + i][column] == oppColor){
            break;
        }
        i++;
    }

    if (cCount >= CONNECT_TO_WIN){   //Imminent win, play at all costs
        score += 1000;
    }

    // Diagonal Down-Right
    i = 1;
    cCount = 1;
    cScaling = scalingFactor;
    while ((row + i) < BOARD_HEIGHT && (column + i) < BOARD_WIDTH && i < CONNECT_TO_WIN){
        if (gameBoard[row + i][column + i] == color){
            score += cScaling;
            cScaling += scalingFactor; // Weigh consecutive pieces higher
            cCount++;
        } else if (gameBoard[row + i][column + i] == oppColor){
            break;
        }
        i++;
    }

    
    i = 1;
    cScaling = scalingFactor;
    // Diagonal Up-Left (↖) - also for completeness
    while ((row - i) >= 0 && (column - i) >= 0 && i < CONNECT_TO_WIN){
        if (gameBoard[row - i][column - i] == color){
            score += cScaling;
            cScaling += scalingFactor;
            cCount++;
        } else if (gameBoard[row - i][column - i] == oppColor){
            break;
        }
        i++;
    }

    if (cCount >= CONNECT_TO_WIN){   //Imminent win, play at all costs
        score += 1000;
    }

    // Diagonal Up-Right
    i = 1;
    cCount = 1;
    cScaling = scalingFactor;
    while ((row - i) >= 0 && (column + i) < BOARD_WIDTH && i < CONNECT_TO_WIN){
        if (gameBoard[row - i][column + i] == color){
            score += cScaling;
            cScaling += scalingFactor;
            cCount++;
        } else if (gameBoard[row - i][column + i] == oppColor){
            break;
        }
        i++;
    }

    i = 1;
    cScaling = scalingFactor;
    // Diagonal Down-Left (↖) - adding the second diagonal direction for completeness
    while ((row + i) < BOARD_HEIGHT && (column - i) >= 0 && i < CONNECT_TO_WIN){
        if (gameBoard[row + i][column - i] == color){
            score += cScaling;
            cScaling += scalingFactor;
            cCount++;
        } else if (gameBoard[row + i][column - i] == oppColor){
            break;
        }
        i++;
    }

    if (cCount >= CONNECT_TO_WIN){   //Imminent win, play at all costs
        score += 1000;
    }

    score += (BOARD_WIDTH - abs((BOARD_WIDTH / 2) - column));
    return score;

}

void resetBoard(char color){    //Clears the board and sets player colors
    int i, j;

    if (color == 'Y'){
        cTurnID = 1;
        pColor = 'Y';
        cColor = 'R';
    } else if (color == 'R') {
        cTurnID = 0;
        pColor = 'R';
        cColor = 'Y';
    } else {    //Error case in which function returns without changing the board state
        printk(KERN_INFO "Bad color\n");
        strscpy(kout, "Bad color\n", sizeof(kout));
        return;
    }

    currTurn = 0;   //Set as yellow turn first
    gameOver = 0;   //Game is not over

    for (i = 0; i < BOARD_HEIGHT; i++){
        for (j = 0; j < BOARD_WIDTH; j++){
            gameBoard[i][j] = '0';
        }
    }
    printk(KERN_INFO "Board Reset\n");
    strscpy(kout, "OK\n", sizeof(kout));
}

int dropPiece(char column, char color){   //Drops a piece on the board. 1 is red, -1 is yellow. 0 is an open space
    int i = BOARD_HEIGHT;
    int j = column - 'A';
    for (i = BOARD_HEIGHT; i >= 0; i--){
        if (gameBoard[i][j] == '0'){
            gameBoard[i][j] = color;

            checkForWin(i, j);
            strscpy(kout, "OK\n", sizeof(kout));
            return 0;
        }
    }
    
    printk(KERN_INFO "Illegal Move\n");
    strscpy(kout, "Illegal Move\n", sizeof(kout));
    return 1;   //Just in case but if working properly code should never get here
}

int testDropPiece(int column){   //Returns what row the piece will land in if it is dropped in a column
    int i = BOARD_HEIGHT;
    for (i = BOARD_HEIGHT; i >= 0; i--){
        if (gameBoard[i][column] == '0'){
            return i;
        }
    }
    return -1;   //Just in case but if working properly code should never get here
}

int checkForWin(int row, int column){
    int i, count, r, c; //Variables we will use to check win conditions
    char color;

    if (currTurn == 0){ //Color played this turn
        color = 'Y';
    } else {
        color = 'R';
    }

    count = 0;
    //Horizontal Case
    for (i = -CONNECT_TO_WIN; i < CONNECT_TO_WIN; i++){
        c = column + i; //Iterate through the columns 4 behind to 4 ahead
        if (c >= 0 && c < BOARD_WIDTH){  //Avoid out of bounds error
            if (gameBoard[row][c] == color){
                count++;    //Piece in a row
            } else {
                count = 0;  //Connected pieces are broken, reset the counter
            }

            if (count == CONNECT_TO_WIN){   //Win found
                if (currTurn == cTurnID){   //Computer win
                    printk(KERN_INFO "Lose\n");
                    strscpy(kout, "Lose\n", sizeof(kout));
                } else {
                    printk(KERN_INFO "Win\n");
                    strscpy(kout, "WIN\n", sizeof(kout));
                }
                gameOver = 1;
                return 1;
            }
        }
    }

    count = 0;
    //Vertical Case
    for (i = -CONNECT_TO_WIN; i < CONNECT_TO_WIN; i++){
        r = row + i; //Iterate through the columns 4 behind to 4 ahead
        if (r >= 0 && r < BOARD_HEIGHT){  //Avoid out of bounds error
            if (gameBoard[r][column] == color){
                count++;    //Piece in a row
            } else {
                count = 0;  //Connected pieces are broken, reset the counter
            }

            if (count == CONNECT_TO_WIN){   //Win found
                if (currTurn == cTurnID){   //Computer win
                    printk(KERN_INFO "Lose\n");
                    strscpy(kout, "Lose\n", sizeof(kout));
                } else {
                    printk(KERN_INFO "Win\n");
                    strscpy(kout, "Win\n", sizeof(kout));
                }
                gameOver = 1;
                return 1;
            }
        }
    }
    

    count = 0;
    //Diagonal Case
    for (i = -CONNECT_TO_WIN; i < CONNECT_TO_WIN; i++){
        r = row + i; //Iterate through the columns 4 behind to 4 ahead
        c = column + i;
        if (r >= 0 && r < BOARD_HEIGHT && c >= 0 && c < BOARD_WIDTH){  //Avoid out of bounds error
            if (gameBoard[r][c] == color){
                count++;    //Piece in a row
            } else {
                count = 0;  //Connected pieces are broken, reset the counter
            }

            if (count == CONNECT_TO_WIN){   //Win found
                if (currTurn == cTurnID){   //Computer win
                    printk(KERN_INFO "Lose\n");
                    strscpy(kout, "Lose\n", sizeof(kout));
                } else {
                    printk(KERN_INFO "Win\n");
                    strscpy(kout, "Win\n", sizeof(kout));
                }
                gameOver = 1;
                return 1;
            }
        }
    }
    
    count = 0;
    //Second Diagonal Case
    for (i = -CONNECT_TO_WIN; i < CONNECT_TO_WIN; i++){
        r = row + i; //Iterate through the columns 4 behind to 4 ahead
        c = column - i;
        if (r >= 0 && r < BOARD_HEIGHT && c >= 0 && c < BOARD_WIDTH){  //Avoid out of bounds error
            if (gameBoard[r][c] == color){
                count++;    //Piece in a row
            } else {
                count = 0;  //Connected pieces are broken, reset the counter
            }

            if (count == CONNECT_TO_WIN){   //Win found
                if (currTurn == cTurnID){   //Computer win
                    printk(KERN_INFO "Lose\n");
                    strscpy(kout, "Lose\n", sizeof(kout));
                } else {
                    printk(KERN_INFO "Win\n");
                    strscpy(kout, "Win\n", sizeof(kout));
                }
                gameOver = 1;
                return 1;
            }
        }
    }

    //Board filled, tie case
    for (i = 0; i < BOARD_WIDTH; i++){
        if (gameBoard[0][i] == '0'){   //See if there is an open space on the top row
            gameOver = 0;
            return 0;
        }
    }

    printk(KERN_INFO "Tie\n");   //No open spaces, so it's a tie
    strscpy(kout, "Tie\n", sizeof(kout));
    return 1;
    

}

void cleanup_module(void)   //removes the module and cleans up member parts
{
	printk(KERN_INFO "Cleaning up connect4...\n");
    cdev_del(&connect_four); // Remove cdev
    device_destroy(cl, MKDEV(major_num, 0)); // Destroy device
    class_destroy(cl); // Destroy device class
    unregister_chrdev(major_num, DEVICE_NAME); // Unregister character deviceex

}