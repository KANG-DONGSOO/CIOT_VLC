#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

volatile sig_atomic_t print_flag = false;

void handle_alarm( int sig ) {
    print_flag = true;
}

int main() {
    signal( SIGALRM, handle_alarm ); // Install handler first,
    ualarm(1000,0); // before scheduling it to be called.

    for (;;) {
        if ( print_flag ) {
            printf( "Hello\n" );
            print_flag = false;
            ualarm(1000,0);
        }
    }
}
