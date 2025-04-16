#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>  // For copy_to_user, copy_from_user
#include <linux/device.h>    // For device class
#include <linux/random.h>   //Random for computer turns

int init_module(void);

ssize_t device_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset);

ssize_t device_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset);

void cleanup_module(void);

static char *devnode(struct device *dev, umode_t *mode);    //Sets the dev mode to 666 so everyone can access

int dropPiece(char column, char color);   //Drops a piece on the board

int testDropPiece(int column); //Returns what row a piece will land in given a column, but does not modify the board

int scoreMove(int row, int column, char color); //Move evalutation function

void executeCmd(void);  //Executes the users command

int CPUmove(char color);    //Handles Move logic for the computer

void resetBoard(char color);  //Resets the board to all 0s, and sets the correct values for players

int checkForWin(int row, int column);   //Checks for a win given the last piece placed