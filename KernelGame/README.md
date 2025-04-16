project3-AidanIbrahim

This project is a kernel module designed to implement connect4

MAKEFILE GUIDE: 
  Make: Compiles module code as well as userspace add on
  Make load: loads the module
  Make unload: removes the module
  Make reload: unloads then loads the module

How to use:
  commands should be in all caps, I'm just to lazy to hit shift while writing this
  echo commands into /dev/fourinarow 
  cat /dev/fourinarow to see module responses
  board: writes a board display to /dev/fourinarow
  DROPC A-H: drops the player's piece, will throw OOT if it's not the player turn. Still works if you feel like resize the board
  CTURN: Computer takes a turn
  RESET Y or R: Resets the board and assigns the player the chosen color. Yellow always goes first

Extras:
  connect4user.c: Only use this if your shell has UTF-8
  run with ./connect4
  simplifies input logic, you do not have to type echo "xyz" >> /dev/fourinarow, just type the command raw
  displays a colored board after every user input automatically
  

  
