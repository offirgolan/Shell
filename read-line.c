/*
 * CS354: Operating Systems. 
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <assert.h>

#define MAX_BUFFER_LINE 2048

// Buffer where line is stored
int line_length;
int curr_pos;
char line_buffer[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.
int history_index = 0;
char **history = NULL;
int history_length = 0;
int HISTORY_SIZE = 10;

void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n"
    " ctrl-D       Delete key at current position\n"
    " ctrl-A       Move to beginning of line\n"
    " ctrl-E       Move to end of line\n";

  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {

  // Set terminal in raw mode
  struct termios orig_attr;
  tcgetattr(0, &orig_attr);
  tty_raw_mode();

  line_length = 0;
  curr_pos = 0;

  // Create history array
  if(history == NULL)
    history = (char**) malloc(HISTORY_SIZE*sizeof(char*));

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch >= 32 && ch != 127) {
      // It is a printable character. 

      // Do echo
      write(1,&ch,1);

      // If max number of character reached return.
      if (line_length==MAX_BUFFER_LINE-2) break; 
      
      if(curr_pos != line_length)
	{
	  char temp;
	  int i;
	  
	  line_length++;
	  
	  for( i = line_length; i > curr_pos; i--)
	    {
	      line_buffer[i] = line_buffer[i-1];
	    }
	  line_buffer[curr_pos]=ch;
	  
	  for(i = curr_pos+1; i < line_length; i++)
	    {
	      ch = line_buffer[i];
	      write(1, &ch, 1);
	    }
	  
	  ch = 8;
	  for(i = curr_pos +1 ; i < line_length; i++)
	    write(1, &ch, 1); 
	}
      else
	{
	  // add char to buffer.
	  line_buffer[line_length]=ch;
	  line_length++;
	}
      
      curr_pos++;
    }
    else if (ch==10) {
      // <Enter> was typed. Return line
      
      // Print newline
      write(1,&ch,1);
      
      // Add line to history
      if(history_length == HISTORY_SIZE)
	{
	  HISTORY_SIZE *=2; 
	  history = (char**) realloc(history, HISTORY_SIZE*sizeof(char*));
	  assert(history != NULL);
	}
      line_buffer[line_length]=0;
      if(line_buffer[0] != 0)
	{
	  history[history_length] = strdup(line_buffer);
	  history_length++;
	}

      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    }
    else if ((ch == 8 || ch == 127) && line_length != 0) {
      // <backspace> was typed. Remove previous character read.

      // Go back one character
      ch = 8;
      write(1,&ch,1);

      // Write a space to erase the last character read
      ch = ' ';
      write(1,&ch,1);

      // Go back one character
      ch = 8;
      write(1,&ch,1);

      // Remove one character from buffer
      line_length--;
      curr_pos--;
    }
    else if(ch == 4  && line_length != 0)
      {
	// CTRL_D:  Remove char at position
	int i;
	for (i = curr_pos; i < line_length - 1; i++) 
	  {
	    line_buffer[i] = line_buffer[i+1];
	    write(1,line_buffer[i],1);
	  }

	line_buffer[line_length] = ' ';
	line_length--;

	//ch = 8;
	for(i = curr_pos; i < line_length; i++)
	  {
	    ch = line_buffer[i];
	    write(1, &ch, 1);
	  }
	ch = ' ';
	write(1, &ch, 1);
	ch = 8;
	write(1, &ch, 1);
	
	for(i = line_length; i > curr_pos; i--)
	  write(1, &ch, 1);
	
      }
    else if(ch == 1)
      {
	// CTRL-A : Move to beginning of line
	int i;
	for(i = curr_pos; i > 0; i--)
	  {
	    ch = 8;
	    write(1,&ch,1);
	    curr_pos--;
	  }
      }

    else if(ch == 5)
      {
	// CTRL-E : Move to end of line
	int i;
	for(i = curr_pos; i < line_length; i++)
	  {
	    char ch = line_buffer[i];
	    write(1, &ch, 1);
	    curr_pos++;
	  }
      }
    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);

      //LEFT ARROW
      if(ch1 == 91 && ch2 == 68)
	{
	  if(curr_pos > 0)
	    {
	      ch = 8;
	      write(1,&ch,1);
	      curr_pos--;
	    }
	}

      //RIGHT ARROW
      if(ch1 == 91 && ch2 == 67)
	{
	  if(curr_pos < line_length)
	    {
	      char ch = line_buffer[curr_pos];
	      write(1, &ch, 1);
	      curr_pos++;
	    }
	}

      // UP-ARROW
      if (ch1==91 && ch2==65 && history_length > 0) {
	// Up arrow. Print next line in history.

	// Erase old line
	// Print backspaces
	int i = 0;
	for (i =line_length - curr_pos; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}

	// Print spaces on top
	for (i =0; i < line_length; i++) {
	  ch = ' ';
	  write(1,&ch,1);
	}

	// Print backspaces
	for (i =0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}

	// Copy line from history
	strcpy(line_buffer, history[history_index]);
	line_length = strlen(line_buffer);
	history_index=(history_index+1)%history_length;
	
	
	// echo line
	write(1, line_buffer, line_length);
	curr_pos = line_length;

      }

      // DOWN-ARROW
      if (ch1==91 && ch2==66) {
	// Up arrow. Print next line in history.
	
	// Erase old line
	// Print backspaces
	int i = 0;
	for (i = line_length - curr_pos; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}

	// Print spaces on top
	for (i =0; i < line_length; i++) {
	  ch = ' ';
	  write(1,&ch,1);
	}

	// Print backspaces
	for (i =0; i < line_length; i++) {
	  ch = 8;
	  write(1,&ch,1);
	}	

	if(history_index > 0)
	  {
	    // Copy line from history
	    strcpy(line_buffer, history[history_index]);
	    line_length = strlen(line_buffer);
	    history_index=(history_index-1)%history_length;
	    
	    // echo line
	    write(1, line_buffer, line_length);
	    curr_pos = line_length;
	  }
	else
	  {
	    // Copy line from history
	    strcpy(line_buffer, "");
	    line_length = strlen(line_buffer);

	    // echo line
	    write(1, line_buffer, line_length);
	    curr_pos = line_length;
	  }
      }
      
    }

  }

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;
  
  tcsetattr(0, TCSANOW, &orig_attr);
  return line_buffer;
}

