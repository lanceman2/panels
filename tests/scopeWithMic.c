// This is a test.  It uses lots of ASSERT() functions instead of regular
// error checking, but I think all the error modes are tested in some way.
// So, it could be a usable program by replacing the ASSERT() calls with
// regular error checks.  ASSERT() is just a great way to test code when
// your not sure how things like failure modes work.

/*

  I have found that the arecord program makes sound files (sound data)
  that has the "wrong" sample rate.  In this example the play back sounds
  like it is playing very fast.  And now, today, (Tue Nov 11 10:53:22 AM
  EST 2025) it works, WTF!

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
#define RATE  192000  // samples per second feed to program arecord
//#define RATE  384000  // samples per second feed to program arecord

// STR(X) turns any CPP macro number into a string by using two macros.
#define STR(s) XSTR(s)
#define XSTR(s) #s


#define SND_FMT  PRIi32
#define YMIN ((double) INT_MIN)
#define YMAX ((double) INT_MAX)
#define SAMPLE_BYTES  (4)
#define ARECORD_FMT   "S32_LE"

// example:
//   arecord -r 384000 -f S32_LE -t raw -c1 -d 5 -B20000

// -f FORMAT -c numChannels -r Hz
// -B microseconds (buffer length)  1s/60 = 0.01666...seconds.
// 1 micro second is second/1000000 
static const char command[] =
        "arecord -r " STR(RATE) " -f " ARECORD_FMT " -c1 -t raw -B80000";

static pid_t pid = 0;
typedef int32_t snd_t;
static const snd_t triggerHeight = 5000000;

static struct PnWidget *graph = 0;
static int pipe_fd = -1;
#define LEN  (1024 * 4)
static size_t samples = 0;
// The read(2) buffer.
static snd_t buf[LEN];

static bool triggered = false;

static const size_t pointsPerDraw = 2000;

static double dt; // dt is time between samples in seconds


static inline void Spawn(void) {

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
            close(fd[0]); // close read fd.
            errno = 0;
            ASSERT(dup2(fd[1], 1) == 1);
            // Now the stdin is this pipe write fd.
            // After execl() this process writes stdout (fd=1) to the
            // write end of the pipe.
            execl("/bin/sh", "sh", "-c", command, NULL);
            ASSERT(0, "execl(,,\"%s\") failed", command);
            exit(EXIT_FAILURE);
        default:
            // I'm the parent
    }
    close(fd[1]); // close write fd.
    int flags = fcntl(fd[0], F_GETFL, 0);
    ASSERT(flags >= 0);
    // We need a non-blocking read to it does not hang forever
    // in a read(2) call.
    ASSERT(fcntl(fd[0], F_SETFL, flags|O_NONBLOCK) != -1);

    pipe_fd = fd[0]; // pipe read fd.
}


static inline void Init(void) {

    ASSERT(graph);

    dt = 1.0/((double) RATE); // time in seconds between samples
    double range = pointsPerDraw * dt;

    double tMin = - range * 0.01; // near 0.0 but a little negitive
    double tMax = pointsPerDraw * dt - tMin;

    // We'll plot signal VS. time in seconds

    //                     xMin  xMax   yMin YMax
    pnGraph_setView(graph, tMin, tMax, YMIN, YMAX);
}


static inline bool ReadSound(int fd, void *userData) {

    DASSERT(fd >= 0);
    DASSERT(fd == pipe_fd);

    ssize_t rd;
    size_t lenRd = 0;

    // likely errno is 11 WOULDBLOCK on failure.
    // TODO: We could deal with errno. 
    //
    while((rd = read(pipe_fd, buf + lenRd,
                    SAMPLE_BYTES * (LEN - lenRd))) > 0) {
        ASSERT(rd % SAMPLE_BYTES == 0,
                "read non-multiple of " STR(SAMPLE_BYTES) " bytes");
        lenRd += rd;
        if(LEN <= lenRd) break;
    }
    samples = lenRd/SAMPLE_BYTES;

#if 0
    INFO("read %zu samples:", samples);

    for(size_t i=0; i < samples; ++i)
        printf("%" SND_FMT " ", buf[i]);
    printf("\n");
#endif

    // If select() popped we should have data.
    ASSERT(samples > 0);

    if(samples >= pointsPerDraw)
        pnWidget_queueDraw(graph, 0);

    return false;
}


static
void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

double t = 0.0;


bool Plot(struct PnWidget *g, struct PnPlot *p, void *userData,
        double xMin, double xMax, double yMin, double yMax) {

    // Note: This trigger idea/stuff could be done in the sound read
    // function, ReadSound(), which could make it so that the what is
    // already plotted stays plotted when there is no trigger event.

    // If there is no trigger than there will be nothing plotted.
    size_t i = 1;
    for(;!triggered && i < samples; ++i) {

        if(buf[i-1] > 0 || buf[i] < triggerHeight
                || buf[i-1] >= buf[i]) continue;

        triggered = true;
        break;
    }
    --i;

    if(!triggered || i >= samples)
        // Plot nothing in this case.
        return false;

    // t0 is the time that a linear interpolation shows the sound would
    // pass through zero in both time and signal.  buf[i] is below or
    // equal to zero and buf[i+1] is above zero.
    ASSERT(buf[i+1] > buf[i]);
    ASSERT(buf[i] <= 0);
    double t0;
    t0 = buf[i+1];
    t0 -= buf[i];
    ASSERT(t0 >= ((double) buf[i+1]));
    t0 = dt - buf[i+1] * dt/(t0);

    size_t num = 0;

    // Note: we are just plotting pointsPerDraw (or less) and than
    // ignoring the rest of the sound data buffer.  We could do what ever
    // we like.

    for(;i < samples && num < pointsPerDraw; ++i, ++num) {
        double x = num;
        x *= dt;
        x -= t0;
        pnPlot_drawPoint(p, x, (double) buf[i]);
    }

    triggered = false;

    return false;
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    struct PnWidget *win = pnWindow_create(0, 10, 10,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWindow_setPreferredSize(win, 1100, 900);

    // The auto 2D plotter grid (graph)
    graph = pnGraph_create(
            win/*parent*/,
            90/*width*/, 70/*height*/, 0/*align*/,
            PnExpand_HV/*expand*/);
    ASSERT(graph);
    //                  Color Bytes:  A R G B
    pnWidget_setBackgroundColor(graph, 0xA0101010, 0);

    struct PnPlot *p = pnScopePlot_create(graph, Plot, 0);
    ASSERT(p);
    // This plot, "p", is owned by "graph".
    pnPlot_setLineColor(p, 0xFFFF0000);
    pnPlot_setPointColor(p, 0xFF00FFFF);
    pnPlot_setLineWidth(p, 2.2);
    pnPlot_setPointSize(p, 2.1);

    Init();
    Spawn();
    pnWindow_show(win);

    ASSERT(pnDisplay_addReader(pipe_fd, 0/*edge_trigger*/,
            ReadSound, 0) == false);

    if(pnDisplay_run()) {
        ERROR("pnDisplay_run() failed");
        ASSERT(0);
    }

    if(pid) {
        printf("Cleaning up children processes\n");
        // Cleanup children processes.
        ASSERT(kill(pid, SIGTERM) == 0);
        ASSERT(waitpid(pid, 0, 0) == pid);
    }
    return 0;
}
