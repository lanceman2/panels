// This is a test.  It uses lots of ASSERT() functions instead of regular
// error checking, but I think all the error modes are tested.  So, it
// could be a usable program by replacing the ASSERT() calls with regular
// error checks.  ASSERT() is just a great way to test code when your
// not sure how things (failure modes) work.

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
#include <fftw3.h>

#include <wayland-client.h>

#include "../include/panels.h"
#include "../lib/debug.h"

// TODO: We need to check, in this code, that the program "arecord" is
// really using the parameters we tell it to use.

#define RATE   44100  // samples per second feed to program arecord

// STR(X) turns any CPP macro number into a string by using two macros.
#define STR(s) XSTR(s)
#define XSTR(s) #s


static inline bool preDispatch(struct wl_display *d, int wl_fd) {

    if(wl_display_dispatch_pending(d) < 0)
        // TODO: Handle failure modes.
        return true; // fail

        // TODO: Add code (with poll(2)) to handle the errno == EAGAIN
        // case.  I think the wl_fd is a bi-directional (r/w) file
        // descriptor.

    // See
    // https://www.systutorials.com/docs/linux/man/3-wl_display_flush/
    //
    // apt install libwayland-doc  gives a large page 'man wl_display'
    // but not 'man wl_display_dispatch'
    //
    // wl_display_flush(d) Flushes write commands to compositor.
    errno = 0;
    // wl_display_flush(d) Flushes write commands to compositor.

    int ret = wl_display_flush(d);

    while(ret == -1 && errno == EAGAIN) {

        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(wl_fd, &wfds);
        // This may never happen, so lets see it if it does.
        DSPEW("waiting to flush wayland display");
        ret = select(wl_fd+1, 0, &wfds, 0, 0);
        switch(ret) {
            case -1:
                ERROR("select failed");
                exit(1);
            case 1:
                ASSERT(FD_ISSET(wl_fd, &wfds));
                break;
            default:
                ASSERT(0, "select() returned %d", ret);
        }

        errno = 0;
        ret = wl_display_flush(d);
    }
    if(ret == -1) {
        ERROR("wl_display_flush() failed");
        return true;
    }
    return false; // success.
}

static struct PnWidget *graph = 0;
static int pipe_fd = -1;
#define LEN  (1024)
static size_t samples = 0;
typedef int32_t snd_t;
static snd_t buf[LEN];

bool triggered = false;

size_t pointsPerDraw;

double t = 0.0;

#define NUM_POINTS 256
fftw_complex in[NUM_POINTS];
fftw_complex out[NUM_POINTS];
double measure[NUM_POINTS];
fftw_plan plan;

pid_t pid = 0;
#define SND_FMT  PRIi32
#define YMIN ((double) - 0.01)
#define YMAX ((double)   1.01)
#define SAMPLE_BYTES  (4)
#define ARECORD_FMT   "S32_LE"

const snd_t triggerHeight = 16000000;
#define REAL 0
#define IMAG 1

double fMax = ((double) RATE) / 2;
// delta frequency in FFT
double df;


// Returns the pipe input file descriptor.
static inline int Spawn(void) {

    // -f FORMAT -c numChannels -r Hz
    // -B microseconds (buffer length)  1s/60 = 0.01666...seconds.
    const char command[] =
        "arecord -f " ARECORD_FMT " -c1 -r" STR(RATE) " -B40000";

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
    return fd[0]; // Return read fd.
}


bool Plot(struct PnWidget *g, struct PnPlot *p, void *userData,
        double xMin, double xMax, double yMin, double yMax) {

    if(!triggered) return false;

    for(size_t i = 0; i < pointsPerDraw; ++i) {
        pnPlot_drawPoint(p, i * df, measure[i]);
        //printf("(%g,%g) ",  i * df, measure[i]);
    }
    //printf("\n\n");

    triggered = false;

    return false;
}

static inline void InitGraph(struct PnWidget *win) {

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
    // This plot, p, is owned by the graph, w.
    pnPlot_setLineColor(p, 0xFFFF0000);
    pnPlot_setPointColor(p, 0xFF00FFFF);
    pnPlot_setLineWidth(p, 2.2);
    pnPlot_setPointSize(p, 2.1);

    ASSERT(NUM_POINTS %2 == 0);

    pointsPerDraw = NUM_POINTS/2;
    df = fMax/((double) pointsPerDraw);
    ASSERT(pointsPerDraw > 100);
    double xMin = - 5.0 * df; // near 0.0 but a little negitive
    double xMax = xMin + fMax + 5.0 * df;
    plan = fftw_plan_dft_1d(NUM_POINTS, in, out,
                        FFTW_FORWARD, FFTW_ESTIMATE);
    ASSERT(plan);

    //                     xMin  xMax   yMin YMax
    pnGraph_setView(graph, xMin, xMax, YMIN, YMAX);

    for(size_t i=0; i<NUM_POINTS; ++i)
        in[i][IMAG] = 0.0;
}


static inline void ReadSound(void) {

    ssize_t rd;
    size_t lenRd = 0;

    // likely errno is 11 WOULDBLOCK on failure.
    // TODO: We could deal with errno. 
    //
    while((rd = read(pipe_fd, buf, LEN)) > 0) {
        ASSERT(rd % SAMPLE_BYTES == 0,
                "read non-multiple of " STR(SAMPLE_BYTES) " bytes");
        lenRd += rd;
    }
    samples = lenRd/SAMPLE_BYTES;

    //INFO("read %zu samples:", samples);
#if 0

    for(size_t i=0; i < samples; ++i)
        printf("%" SND_FMT " ", buf[i]);
    printf("\n");
#endif

    // If select() popped we should have data.
    ASSERT(samples > 0);

    if(samples >= NUM_POINTS) {
        pnWidget_queueDraw(graph, 0);
        if(!triggered) {
            ASSERT(plan);
            // We compute the FFT for plotting now.  Note, we don't
            // recompute the FFT if we have one waiting to be plotted
            // already.
            triggered = true;
            // Convert uint32_t to double.
            double max = 0.0;
            for(size_t i=0; i<NUM_POINTS; ++i) {
                double x = fabs((double) buf[i]);
                if(x > max)
                    max = x;
            }
            max *= ((double) NUM_POINTS);

            for(size_t i=0; i<NUM_POINTS; ++i)
                in[i][REAL] = ((double) buf[i])/max;
            fftw_execute(plan);
            for(size_t i=0; i<NUM_POINTS; ++i)
                measure[i] = sqrt(out[i][REAL] * out[i][REAL] +
                        out[i][IMAG] * out[i][IMAG]);
        }
    }
}


static inline void Run(struct PnWidget *win) {
    
    struct wl_display *d = pnDisplay_getWaylandDisplay();
    ASSERT(d);
    int wl_fd = wl_display_get_fd(d);
    ASSERT(wl_fd == 3);

    pipe_fd = Spawn();
    ASSERT(pipe_fd > wl_fd);

    // Run the main loop until the GUI user causes it to stop.
    while(true) {

        if(preDispatch(d, wl_fd)) return;

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(wl_fd, &rfds);
        FD_SET(pipe_fd, &rfds);
        int ret = select(pipe_fd+1, &rfds, 0, 0, 0);
        switch(ret) {
            case -1:
                ERROR("select failed");
                exit(1);
            case 1:
            case 2:
                ASSERT(FD_ISSET(wl_fd, &rfds) ||
                        FD_ISSET(pipe_fd, &rfds));
                if(FD_ISSET(wl_fd, &rfds)) {
                    if(wl_display_dispatch(d) == -1 ||
                            !pnDisplay_haveWindow())
                        // Got running window based GUI.
                        return;
                }
                if(FD_ISSET(pipe_fd, &rfds)) {
                    ReadSound();
                }
                break;
            default:
                ASSERT(0, "select() returned %d", ret);
        }
    }
}

static
void catcher(int sig) {
    ASSERT(0, "caught signal number %d", sig);
}


int main(void) {

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));

    printf("fftw_execute=%p\n", fftw_execute);

    struct PnWidget *win = pnWindow_create(0, 10, 10,
            0/*x*/, 0/*y*/, PnLayout_LR/*layout*/, 0,
            PnExpand_HV);
    ASSERT(win);
    pnWindow_setPreferredSize(win, 1100, 900);

    InitGraph(win);

    pnWindow_show(win);
    Run(win);

    // Cleanup child processes.
    if(pid) {
        ASSERT(kill(pid, SIGTERM) == 0);
        ASSERT(waitpid(pid, 0, 0) == pid);
    }
    fftw_destroy_plan(plan);
    return 0;
}
