/*
  For testing reading and writing sound:  Run in a bash shell or
  whatever:

arecord -r 384000 -f S32_LE -t raw -c1 -d 5 -B20000  xxx
aplay -r 384000  -f S32_LE -c1 -t raw  xxx

*/

#define _GNU_SOURCE
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <limits.h>
#include <math.h>

#include <wayland-client.h>

#include "../include/panels.h"
#include "../lib/debug.h"


//#define RATE   44100  // samples per second feed to program arecord
//#define RATE  192000  // samples per second feed to program arecord
#define RATE  384000  // samples per second feed to program arecord

// STR(X) turns any CPP macro number into a string by using two macros.
#define STR(s) XSTR(s)
#define XSTR(s) #s


#define SND_FMT  PRIi32
#define ARECORD_FMT   "S32_LE"
typedef int32_t snd_t;



// example:
//   aplay -r 384000 -f S32_LE -t raw -c2 -B20000

// -f FORMAT -c numChannels -r Hz
// -B microseconds (buffer length)  1s/60 = 0.01666...seconds.
// 1 micro second is second/1000000
#if 1
static const char command[] =
        "aplay -r " STR(RATE) " -f " ARECORD_FMT " -c2 -t raw";
#else
static const char command[] = "hexdump -v";
#endif

static pid_t pid = 0;
#define LEN  (1024 * 4)
static snd_t buf[LEN];
#define SAMPLE_BYTES (sizeof(snd_t))
static FILE *fileOut;

void Spawn(void) {

    int fd[2];
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
            ASSERT(dup2(fd[0], 0) == 0);
            // Now the stdin is this pipe read fd.
            // After execl() this process reads stdin (fd=0).
            execl("/bin/sh", "sh", "-c", command, NULL);
            ASSERT(0, "execl(,,\"%s\") failed", command);
            exit(EXIT_FAILURE);
        default:
            // I'm the parent
    }
    close(fd[0]); // close read fd.

    fileOut = fdopen(fd[1], "a");
    ASSERT(fileOut);
}


static double dt, t;
static double wavePeriod = 1/400.0;
static double waveHalfPeriod;
static bool up = true;
static const snd_t high = 40000000;
static const snd_t low = -40000000;


static inline void Init(void) {

    dt = 1/((double) RATE);
    waveHalfPeriod = wavePeriod/2.0;
    ASSERT(dt < waveHalfPeriod);
    fileOut = stdout; // Spawn() can override fileOut.
};


// A square pulse.
static inline snd_t Wave(void) {

    snd_t ret;

    if(up)
        ret = high;
    else
        ret = low;

    t += dt;
    if(t > wavePeriod) {
        t -= wavePeriod;
        up = true;
    } else if(t > waveHalfPeriod) {
        up = false;
    }

    return ret;
}


static inline bool WriteSound() {

    for(int i=0; i<LEN; ++i)
        buf[i] = Wave();

    size_t ret = fwrite(buf, sizeof(snd_t), LEN, fileOut);

    return (ret != LEN)?true:false;
}


static
void catcher(int sig) {
    INFO("caught signal number %d, quitting", sig);
}

volatile sig_atomic_t running = 1;

static void sigvCatcher(int sig) {
    ASSERT(0, "caught signal number %d", sig);
    running = 0;
}



int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, sigvCatcher));

    ASSERT(SIG_ERR != signal(SIGTERM, catcher));
    ASSERT(SIG_ERR != signal(SIGQUIT, catcher));
    ASSERT(SIG_ERR != signal(SIGINT, catcher));

    Init();
    Spawn();

    while(running)
        if(WriteSound())
            break;

    if(pid) {
        printf("Cleaning up children processes\n");
        // Cleanup children processes.
        ASSERT(kill(pid, SIGTERM) == 0);
        ASSERT(waitpid(pid, 0, 0) == pid);
    }
    return 0;
}

