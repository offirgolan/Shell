
/*
 *
 * CS-252 Fall 2013
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/prctl.h>
#include "y.tab.h"

int MAXBUFFERSIZE = 1024;

////////////  Start added code ///////////

extern "C" {char * read_line(); }

int mygetc(FILE * f) {
  static char *p;
  char ch;

  if (!isatty(0)) {
    // stdin is not a tty. Call real getc
    return getc(f);
  }

  // stdin is a tty. Call our read_line.

  if (p==NULL || *p == 0) {
    char * s = read_line();
    p = s;
  }

  ch = *p;
  p++;

  return ch;
}

#undef getc
#define getc(f) mygetc(f)

/////////// End added code ///////////

%}

%%

\n 	{
		return NEWLINE;
	}

[ \t] 	{
		/* Discard spaces and tabs */
     	}	

">" 	{
		return GREAT;
	}

">&" 	{
		return GREATAMP;
	}

">>" 	{
		return GREATGREAT;
	}

">>&" 	{
		return GREATGREATAMP;
	}

"<" 	{
		return LESS;
	}

"|" 	{
		return PIPE;
	}

"&" 	{
		return AMPERSAND;
	}

[^\n`]*`[^\n`]*`  {

                char *origCmd = strdup(yytext);
		char *startIndex = origCmd;
		char *endIndex;
		char *cmd = (char*) malloc(MAXBUFFERSIZE*sizeof(char*));
                char *subCmd = (char*) malloc(MAXBUFFERSIZE*sizeof(char*));
		char *buffer = (char*) malloc(MAXBUFFERSIZE*sizeof(char*));
                int ret;

                // Save original redirects
                int defaultin = dup(0);
                int defaultout = dup(1);

		// pipes
		int pipe1[2];
		int pipe2[2];
                pipe(pipe1);
		pipe(pipe2);

		startIndex = strchr(origCmd, '`');

		   endIndex = strchr(startIndex+1, '`');
		   int cmdLen = strlen(origCmd) - strlen(startIndex);

		   memcpy(cmd, origCmd, cmdLen);
		   memcpy(subCmd, startIndex+1, strlen(startIndex) - strlen(endIndex) - 1);
		
		   int flags = fcntl(pipe2[0], F_GETFL);
		   fcntl(pipe2[0],F_SETFL, flags | O_NONBLOCK);

		   dup2(pipe1[1], 1); //parent out to child
		   close(pipe1[1]);
		   printf("%s\nexit\n", subCmd);

                   ret = fork();
		   if(ret == 0)
                   {

			//redirect input from parent's output
			dup2(pipe1[0], 0);
			dup2(pipe2[1], 1);

			close(pipe1[0]);
			close(pipe2[1]);

			//_exit(0);
		   }
		   else
		   {
		     // wait for child to finish
		     while(waitpid(ret, NULL, 0) == -1);

		     close(pipe1[1]);

                     // Put result into buffer
		     for(int i = 0; i < MAXBUFFERSIZE; i++)
			   buffer[i] = 0;
		     
		     // read from pipe2 one char at a time
		     int count = 0;
		     char* buffer_tmp = buffer;
		     while(read(pipe2[0], buffer, 1) > 0)
		     {
		     	if(*buffer == '\n')
			   *buffer = ' ';
			count++;
			buffer++;
			// resize if necessary
			if(count == MAXBUFFERSIZE)
			{
	  		  MAXBUFFERSIZE *=2; 
	  		  buffer_tmp = (char*) realloc(buffer_tmp, MAXBUFFERSIZE*sizeof(char*));
	  		  assert(buffer_tmp != NULL);
			}
      		     }
		     close(pipe2[1]);
		     // set buffer to &buffer[0]
		     buffer = buffer_tmp;
		     // Make sure to null terminate 
		     buffer[count]=0;

		     // append the subshelled command to the initial command
		     cmd = (char*) realloc(cmd, (strlen(cmd) + count)*sizeof(char*));
		     strncat(cmd, buffer, strlen(buffer) );

			//dup2(defaultout, 1);	
	             //fprintf(stdout, "cmd: %s", cmd);
		     //fprintf(stdout, "count: %d\n", count);
		     //fprintf(stdout, "\nsizeof(cmd): %d\n", (int)strlen(cmd));
		     
		     // reset stdin and stdout to default
		     dup2(defaultin, 0);
		     dup2(defaultout, 1);


         	      // close
	       	      close(defaultin);
	              close(defaultout);

		      // free
		      free(subCmd); 
		      free(buffer);

		      prctl(PR_SET_PDEATHSIG, SIGHUP);

		      for(int i = (int)strlen(cmd) - 1; i >= 0; i--)
		        yyunput(cmd[i], yytext);

		     //perror("fork");
		     //_exit(0);
		   }
}

[^ \t\n<>&\|]*(\\.[^ \t\n<>&\|]*)*  {
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
                
                int len = strlen(yylval.string_val);
                char cmd [len + 1];

                int i = 0;
                int j = 0;
                for( ; i < len; i++)
		    {		        

		      if(yylval.string_val[i] != '\\')
                      {
		        cmd[j++] = yylval.string_val[i];

                      }
		      else
		      {
		        cmd[j++] = yylval.string_val[i+1];
			i++;
		      }
		      
		    }
		cmd[j] = '\0';
		//printf("%s\n", cmd);
		strcpy(yylval.string_val, cmd);

                
		return WORD;
	}

\"[^\n\"]*\"  {
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
                int len = strlen(yylval.string_val);
                char cmd [len + 1];

                int i = 0;
                int j = 0;
                for( ; i < len; i++)
		    {
		      if(yylval.string_val[i] != '\"')
                      {
		        cmd[j++] = yylval.string_val[i];

                      }
		    }
		cmd[j] = '\0';
		 //printf("GOT HERE[2]: %s\n", cmd);
		strcpy(yylval.string_val, cmd);

                
		return WORD;
	}

.	{
		/* Invalid character in input */
		return NOTOKEN;
	}

%%
	
