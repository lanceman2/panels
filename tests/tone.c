/*

./tone_run | aplay -f S32_LE -c1 -r 44100


*/

// STR(X) turns any CPP macro number into a string by using two macros.
#define STR(s) XSTR(s)
#define XSTR(s) #s

//#define RATE   44100  // samples per second feed to program aplay
#define RATE  192000

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>

#include "../lib/debug.h"

#define LEN  (1024 * 4)

double freq = 400.0; // tone to output in Hz


int32_t out[LEN];
double t = 0;
double bigT;
double dt; // Time between samples output.
double omega;
double amp = INT_MAX - 10;
pid_t pid;


// Returns the pipe input file descriptor.
static inline int Spawn(void) {

    // -f FORMAT -c numChannels -r Hz
    // -B microseconds (buffer length)  1s/60 = 0.01666...seconds.
    const char command[] =
        "aplay -f S32_LE -c1 -r" STR(RATE);

    int fd[2] = { -1, -1 };
    ASSERT(pipe(fd) == 0);
    ASSERT(fd[0] >= 3);
    ASSERT(fd[1] >= 3);
    pid = fork();

    switch(pid) {
        case -1:
            ASSERT(0, "fork() failed");
            exit(EXIT_FAILURE);
        case 0:
            // I'm the child.
            close(fd[1]); // close write fd.
            errno = 0;
            ASSERT(dup2(fd[0], STDIN_FILENO) == STDIN_FILENO);
            execl("/bin/sh", "sh", "-c", command, NULL);
            ASSERT(0, "execl(,,\"%s\") failed", command);
            exit(EXIT_FAILURE);
        default:
            // I'm the parent
    }
    close(fd[0]); // close read pipe.
    return fd[1]; // Return write pipe.
}



static inline void fillBuffer(void) {

    for(size_t i=0; i < LEN; t += dt, ++i) {
        double x = amp * cos(omega * t);
        out[i] = (int32_t) x;
    }
    if(t > bigT)
        // Not letting t overflow.
        t -= bigT;
}


int main(void) {

    // The sample rate of the program reading the
    // data that this program outputs.
    size_t sample_rate = RATE; // Hz

    omega = 2.0 * M_PI * freq;
    dt = 1.0/((double) sample_rate);
    bigT = LEN * 10.0 / freq;

    int fd = Spawn();

    while(true) {

        int32_t *x = out;
        fillBuffer();
        size_t len = LEN * sizeof(*out);
        do {
            // I think this will block when it needs to due to
            // aplay reading input only so fast.
            ssize_t wr = write(fd, x, len);
            if(wr < 1) {
                ERROR("write failed");
                return 1;
            }
            len -= wr;
            x += len;
        } while(len);
    }

    // Cleanup child processes.
    if(pid) {
        ASSERT(kill(pid, SIGTERM) == 0);
        ASSERT(waitpid(pid, 0, 0) == pid);
    }

    return 0;
}
