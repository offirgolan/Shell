
//
// Example of how to ignore ctrl-c
//

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>


extern "C" void disp( int sig )
{
	fprintf( stderr, "\n      Ouch!\n");
}

int
main()
{
	printf( "Type ctrl-c or \"exit\"\n");
	sigset( SIGINT, disp );
	for (;;) {
		
		char s[ 20 ];
		printf( "prompt>");
		fflush( stdout );
		gets( s );

		if ( !strcmp( s, "exit" ) ) {
			printf( "Bye!\n");
			exit( 1 );
		}
	}

	return 0;
}


