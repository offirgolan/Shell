
/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <regex.h>
#include <string.h>
#include <signal.h>

#include "command.h"

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}

	char * buffer = "^.*${[^}][^}]*}.*$";
	regex_t re;
	regmatch_t match;	
	int result = regcomp( &re, buffer, 0);
	if (result != 0) 
	  {
	    perror("Bad regular expression: compile");
	    return;
	  }
	while(regexec(&re, argument, 1, &match, 0 ) == 0)
	  {
	    int varBegin, varEnd;
	    for(int i = 0; i < strlen(argument); i++)
	      {
		if(argument[i] == '$' && argument[i+1] == '{')
		  varBegin = i;
		if(argument[i] == '}')
		  {
		    varEnd = i;
		    break;
		  }
	      }
	    // Extract variable to be replaced
	    int varIndex = varEnd - varBegin - 1;
	    char enVar[varIndex];
	    enVar[varIndex-1] = '\0';
	    memcpy(enVar, &argument[varBegin+2],varIndex-1);

	    // Get the corresponding env. var
	    char *enviroment = getenv(enVar);
	    
	    // Copy the env. var to argument
            int len = strlen(argument) + strlen(enviroment) + 1;
	    char temp[len];
            for(int i = 0; i < len; i++)
              temp[i] = 0;
	    memcpy(temp, &argument[0], varBegin);
	    strcat(temp, enviroment);
	    strcat(temp, &argument[varEnd+1]);

	    argument = strdup(temp);

	  }
	
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_outCount = 0;
	_inCount = 0;
	_errCount = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_outCount = 0;
	_inCount = 0;
	_errCount = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
		printf("\n");
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void
Command::execute()
{

	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}
	
	// EXIT
	if(_numberOfSimpleCommands == 1 && !strcmp( _simpleCommands[0]->_arguments[0], "exit" ))
	  {
	    //printf("\n\t Fine! Just leave me here to die...\n\n");
	    exit(0);
	  }
	
	// CD
	if(!strcmp( _simpleCommands[0]->_arguments[0], "cd" ))
	  {
	    int ret;
	    if(_simpleCommands[0]->_numberOfArguments == 1)
	      ret = chdir(getenv("HOME"));
	    else
	      ret = chdir(_simpleCommands[0]->_arguments[1]);
	    if(ret < 0)
	      perror("cd");
	    clear();
	    prompt();
	    return;
	  }

	// CLEAR
	if(!strcmp( _simpleCommands[0]->_arguments[0], "clear" ))
	  {
	    std::system("clear");
	    clear();
	    prompt();
	    return;
	  }

	// SETENV
	if(!strcmp( _simpleCommands[0]->_arguments[0], "setenv" ))
	  {
	    int ret;
	    if(_simpleCommands[0]->_numberOfArguments != 3)
	      ret = setenv(NULL,NULL,0);
	    else
		ret = setenv(_simpleCommands[0]->_arguments[1],_simpleCommands[0]->_arguments[2],1);
	    if(ret < 0)
	      perror("setenv");
	    clear();
	    prompt();
	    return;
	  }

	// UNSETENV
	if(!strcmp( _simpleCommands[0]->_arguments[0], "unsetenv" ))
	  {
	    int ret;
	    if(_simpleCommands[0]->_numberOfArguments != 2)
	      ret = unsetenv(NULL);
	    else
	      ret = unsetenv(_simpleCommands[0]->_arguments[1]);
	    if(ret < 0)
	      perror("unsetenv");
	    clear();
	    prompt();
	    return;
	  }
	
	// AMBIG REDIR
	if(_errCount > 1 || _inCount > 1 || _outCount > 1)
	  {
	    printf("Ambiguous output redirect\n");
	    clear();
	    prompt();
	    return;
	  }

	// Print contents of Command data structure
	//print();

	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec
	int defaultin = dup(0);
	int defaultout = dup(1);
	int defaulterr = dup(2);
	
	int fdout, fdin, fderr;

	if(_inputFile)
	  {
	    fdin = open(_inputFile, O_RDONLY, 0664);
	    if(fdin < 0 )
	      {
		perror("inputFile open error");
		exit(1);
	      }
	  }
	else
	  {
	    // use default input
	    fdin = dup(defaultin);
	  }

	if(_errFile)
	  {
	    if(!_append)
	      fderr = open(_errFile, O_CREAT|O_WRONLY|O_TRUNC, 0664);
	    else
	      fderr = open(_errFile, O_CREAT|O_WRONLY|O_APPEND, 0664);
	    if(fderr < 0 )
	      {
		perror("errFile open error");
		exit(1);
	      }
	  }
	else
	  {
	    // use default error
	    fderr = dup(defaulterr);
	  }

	int ret;
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) 
	  { 
	    // redirect input
	    dup2(fdin,0);
	    dup2(fderr,2);
	    close(fdin);
	    
	    // setup output
	    if(i == _numberOfSimpleCommands - 1) // Last simple command
	      {
		if(_outFile)
		  {
		    if(!_append)
		      fdout = open(_outFile, O_CREAT|O_WRONLY|O_TRUNC, 0664);
		    else
		      fdout = open(_outFile, O_CREAT|O_WRONLY|O_APPEND, 0664);
		    if(fdout < 0 )
		      {
			perror("outFile open error");
			exit(1);
		      }
		  }
		else if(_errFile)
		  {
		    if(!_append)
		      fdout = open(_errFile, O_CREAT|O_WRONLY|O_TRUNC, 0664);
		    else
		      fdout = open(_errFile, O_CREAT|O_WRONLY|O_APPEND, 0664);
		    if(fdout < 0 )
		      {
			perror("outFile open error");
			exit(1);
		      }
		  }
		else
		  {
		    // use defauly output
		    fdout = dup(defaultout);
		  }
	      }
	    else 
	      {
		// Not last simple command
		//create pipe
		int fdpipe[2];
		pipe(fdpipe);
		fdout=fdpipe[1];
		fdin=fdpipe[0];
	      }
	    // Redirect output
	    dup2(fdout,1);
	    close(fdout);

	    ret = fork();
	    if (ret == 0) 
	      {

		if(!strcmp( _simpleCommands[0]->_arguments[0], "printenv" ))
		  {
		    char **p=environ;
		    while (*p!=NULL) {
		      printf("%s\n",*p);
		      p++;
		    }
		    exit(0);
		  }
		//child
		execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
		perror("execvp");
		_exit(1);
	      }
	    else if (ret < 0) 
	      {
		perror("fork");
		return;
	      }
	    // Parent shell continue
	  }

	// Restore input, output, and error
	dup2(defaultin, 0);
	dup2(defaultout, 1);
	dup2(defaulterr, 2);
	
	// close
	close(defaultin);
	close(defaultout);
	close(defaulterr);

	if (!_background) 
	  {
	    // wait for last process
	    waitpid(ret, NULL, 0);
	  }

	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{
  if ( isatty(0) )
    {
	printf("myshell > ");
	fflush(stdout);
    }
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

void ctrlCHandler(int sig_int=0) {
  printf("\n");
  Command::_currentCommand.prompt();
}

void zombie_handler(int signal)
{
  int status;
  pid_t	pid;
  
  while (1) {
    pid = wait3(&status, WNOHANG, (struct rusage *)NULL);
    if (pid == 0)
      return;
    else if (pid == -1)
      return;
    else ;
      //printf ("%d exited.\n", pid);
  }
}

main()
{

  /* ZOMBIE outputs in pipe commands! FIX THIS */
  signal(SIGINT, ctrlCHandler);
  signal(SIGCHLD,zombie_handler);
  Command::_currentCommand.prompt();
  yyparse();
}

