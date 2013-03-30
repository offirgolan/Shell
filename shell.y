
/*
 * CS-252 Spring 2013
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 * 
 *	cmd [arg]* [> filename]
 *
 *
 *      cmd [arg]* [ | cmd [arg]* ]* [ [> filename] [< filename] [ >& filename] [>> filename] [>>& filename] ]* [&]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%token	<string_val> WORD

%token 	NOTOKEN NEWLINE GREAT GREATAMP GREATGREAT GREATGREATAMP LESS AMPERSAND PIPE

%union	{
		char   *string_val;
	}

%{
//#define yylex yylex
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include <dirent.h>
#include <assert.h>
#include "command.h"
#include <pwd.h>

#define MAXFILENAME 1024

char **array = NULL;
int nEntries = 0;
int maxEntries = 20;

void yyerror(const char * s);
void expandWildcardsIfNecessary(char * arg);
void expandWildcard(char * prefix, char *suffix);
int compare(const void *str1, const void *str2);
int yylex();

%}

%%

goal:	
	commands
	;

commands: 
	command
	| commands command
	;

command: simple_command
        ;

simple_command:	
	pipe_list iomodifier_list background_opt NEWLINE {
	  //printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
        | NEWLINE { Command::_currentCommand.prompt(); }
	| error NEWLINE { yyerrok; }
         	 ;

pipe_list:
    pipe_list PIPE command_and_args
    | command_and_args
    ;

command_and_args:
	command_word arg_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD {
	  //printf("   Yacc: insert argument \"%s\"\n", $1);

	       expandWildcardsIfNecessary($1);
	}
	;

command_word:
	WORD {
	  //printf("   Yacc: insert command \"%s\"\n", $1);
	       
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

iomodifier_list:
       iomodifier_list iomodifier_opt
       | /* can be empty */
       ;

iomodifier_opt:
	GREAT WORD {
	  //printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._outCount++;
	}
	|
	GREATGREAT WORD {
	  //printf("   Yacc: append output \"%s\"\n", $2);
		Command::_currentCommand._append = 1;
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._outCount++;
	}
	|
	GREATAMP WORD {
	  //printf("   Yacc: insert error \"%s\"\n", $2);
		Command::_currentCommand._errFile = $2;
		Command::_currentCommand._errCount++;
	}
	|
	GREATGREATAMP WORD {
	  //printf("   Yacc: append error \"%s\"\n", $2);
		Command::_currentCommand._append = 1;
		Command::_currentCommand._errFile = $2;
		Command::_currentCommand._errCount++;
	}
	|
	LESS WORD {
	  //printf("   Yacc: insert input \"%s\"\n", $2);
		Command::_currentCommand._inputFile = $2;
		Command::_currentCommand._inCount++;
	}
	;

background_opt:
	AMPERSAND {
	  //printf("   Yacc: run in background YES\n");
		Command::_currentCommand._background = 1;
	}
	| /* can be empty */
	;

%%

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

void expandWildcardsIfNecessary(char * arg) 
{
  // Return if arg does not contain ‘*’ or ‘?’
  char *tempArg = strdup(arg);
  if (strchr(tempArg, '*') == NULL && strchr(tempArg, '?') == NULL && strchr(tempArg, '~') == NULL) 
    {
      Command::_currentSimpleCommand->insertArgument(tempArg);
      return;
    }

  expandWildcard("", tempArg);

  if(nEntries > 1)
  	qsort(array, nEntries, sizeof(char*), compare);
  // Add arguments
  for (int i = 0; i < nEntries; i++) 
  { 
    Command::_currentSimpleCommand->insertArgument(array[i]);
  }
  
  if(array != NULL)
    free(array);
  array = NULL;
  nEntries = 0;

  return;

}

void expandWildcard(char * prefix, char *suffix) 
{

  if (suffix[0]== 0) {
    // suffix is empty. Put prefix in argument.
    //Command::_currentSimpleCommand->insertArgument(strdup(prefix));
    return;
  }

  bool dirFlag = false;
  // Obtain the next component in the suffix
  // Also advance suffix.
  if(suffix[0] == '/')
    {
      dirFlag = true;
    }
 
  char * suffix_backup = suffix;
  char * s = strchr(suffix, '/');
  char component[MAXFILENAME];
 for(int i = 0; i < MAXFILENAME; i++)
    component[i] = 0;
  if (s != NULL){ // Copy up to the first “/”
    if(prefix[0] == 0 && dirFlag)
      {
	suffix = s+1;
	s = strchr(suffix, '/');

	if(s == NULL)
	  {
	    strcpy(component, suffix);
	    suffix = suffix + strlen(suffix);
	    prefix = "/";
	  }
	else
	  {
	    strncpy(component,suffix, s-suffix);
	    suffix = s + 1;
	    prefix = "/";
	  }
      }
    else
      {
	strncpy(component,suffix, s-suffix);
	suffix = s + 1;
      }
      
  }
  else { // Last part of path. Copy whole thing.
    strcpy(component, suffix);
    suffix = suffix + strlen(suffix);
  }

  char tildePrefix[MAXFILENAME];
  for(int i = 0; i < MAXFILENAME; i++)
    tildePrefix[i] = 0;
  if(component[0] == '~')
    {
      struct passwd *pwd;
      if(strcmp(component, "~") == 0)
	pwd = getpwnam(getenv("USER"));
      else
	pwd = getpwnam(component+1);

      if(pwd == NULL)
	printf("Could not access user %s.\n", component+1);
      else
	{
	  if(suffix[0] == 0 && prefix[0] == 0)
	    sprintf(tildePrefix,"%s",pwd->pw_dir);
	  else if(suffix[0] == 0)
	    sprintf(tildePrefix,"%s/%s",pwd->pw_dir, component);
	  else if(prefix[0] == 0)
	    sprintf(tildePrefix,"%s/%s",pwd->pw_dir, suffix);
	  expandWildcardsIfNecessary(tildePrefix);
	  return;
	}
    }

  // Now we need to expand the component
  char newPrefix[MAXFILENAME];
  for(int i = 0; i < MAXFILENAME; i++)
    newPrefix[i] = 0;
  if (strchr(component, '*') == NULL && strchr(component, '?') == NULL) {
    // component does not have wildcards

    if(prefix[0] == 0 && !dirFlag)
      sprintf(newPrefix,"%s",component);
    else if(strcmp(prefix, "/") == 0)
      sprintf(newPrefix,"/%s",component);
    else
      sprintf(newPrefix,"%s/%s",prefix,component);    
    
    expandWildcard(newPrefix, suffix);
    return;
  }
  // Component has wildcards
  // Convert component to regular expression

   // 1. Convert wildcard to regular expression
  // Convert “*” -> “.*”
  //         “?” -> “.”
  //         “.” -> “\.”  and others you need
  // Also add ^ at the beginning and $ at the end to match
  // the beginning ant the end of the word.
  // Allocate enough space for regular expression
  char * reg = (char*)malloc(2*strlen(component)+10); 
  char * a = component;
  char * r = reg;
  *r = '^'; r++; // match beginning of line
  while (*a) 
    {
      if (*a == '*') { *r='.'; r++; *r='*'; r++; }
      else if (*a == '?') { *r='.'; r++;}
      else if (*a == '.') { *r='\\'; r++; *r='.'; r++;}
      else { *r=*a; r++;}
      a++;
    }
  *r='$'; r++; *r=0;// match end of line and add null char
  // 2. compile regular expression
  regex_t re;
  regmatch_t match;	
  int result = regcomp( &re, reg, REG_EXTENDED|REG_NOSUB);
  free(reg);
  if (result != 0) 
    {
      perror("Bad regular expression: compile");
      return;
    }
  
  // 3. List directory and add as arguments the entries 
  // that match the regular expression
  char * d;
  if (prefix[0] == 0) 
    d = "."; 
  else d = prefix;

  //printf("[2] Prefix: %s | Suffix: %s | Component: %s\n", prefix, suffix, component);

  DIR * dir = opendir(d);
  if (dir == NULL) 
    {
      //perror("opendir");
      return;
    }

  struct dirent * ent;
  if(array == NULL)
    array = (char**) malloc(maxEntries*sizeof(char*));

  while ( (ent = readdir(dir))!= NULL) {
    // Check if name matches
    if (regexec(&re, ent->d_name, 1, &match, 0 ) == 0)
      {

	if(array != NULL && nEntries > 1)
	  qsort(array, nEntries, sizeof(char*), compare);
	
	// Resize array if reached maxEntries
        if (nEntries == maxEntries) 
	{
	  maxEntries *=2; 
	  array = (char**) realloc(array, maxEntries*sizeof(char*));
	  assert(array!=NULL);
	}

	if (ent->d_name[0] == '.') 
	  {
	    if (component[0] == '.') 
	      {
		if(prefix[0] == 0)
	      sprintf(newPrefix,"%s",ent->d_name);
	    else if(prefix[0] == '/' && prefix[1] == 0)
	      sprintf(newPrefix,"/%s",ent->d_name);
	    else
	      sprintf(newPrefix,"%s/%s",prefix,ent->d_name);
		
	    expandWildcard(newPrefix,suffix);
	    if(s == NULL)
	      {
		array[nEntries]= strdup(newPrefix);
		nEntries++;
	      }
	      }
	  }
	else 
	  {
	    if(prefix[0] == 0)
	      sprintf(newPrefix,"%s",ent->d_name);
	    else if(prefix[0] == '/' && prefix[1] == 0)
	      sprintf(newPrefix,"/%s",ent->d_name);
	    else
	      sprintf(newPrefix,"%s/%s",prefix,ent->d_name);
	    
	    expandWildcard(newPrefix,suffix);
	    if(s == NULL)
	      {
		array[nEntries]= strdup(newPrefix);
		nEntries++;
	      }
	  }
	
      }
  }

  closedir(dir);
  return; 

}

int compare(const void *str1, const void *str2)
{

  return strcmp(*(char *const*)str1, *(char *const*)str2);
}

#if 0
main()
{
	yyparse();
}
#endif
