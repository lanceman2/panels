// A test that loads libpanels.so and then unloads it, and checks that
// libpanels.so unloads along with it's dependencies by printing out the
// libraries listed in the file /proc/PID/maps
//
// We automate this a little and grep for libraries in the /proc/PID/maps,
// to determine if a library is mapped (or not) when it should (not) be.
// A human looking at the printed output is a better check, i.e. it's not
// likely we are automatically checking all failure modes; but shit this
// is better than not doing this test at all.
//
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <dlfcn.h>
#include <unistd.h>

#include "../lib/debug.h"


static void catcher(int sig) {

    ASSERT(0, "caught signal number %d", sig);
}

static int count = 0;
static pid_t pid;

static inline void CheckLib(const char *libname, bool present) {

    const size_t Len = 1024;
    char buf[Len];

    // TODO: We may be assuming that this can run.  There may be other
    // failure modes we are not checking.
    //
    snprintf(buf, Len, "cat /proc/%d/maps | grep -q %s", pid, libname);
    int ret = system(buf);

    if(present && ret == 0)
        // success; grep found libname
        return;
    if(!present && ret == 256)
        /*TODO: I don't understand this return code, 256, but it seems
         * to be what is returned from grep when it's not found. */
        // success; grep did not find libname
        return;

    ASSERT(0, "Library \"%s\" IS %spresent in the program ret=%d",
            libname, present?"NOT ":"", ret);
}


// Prints /proc/PID/maps so we can see what libraries are loaded
// at the time this is called.
//
// And the Prints the open files from /proc/PID
//
static inline void PrintStuff(const char *str) {

    const size_t Len = 1024;
    char buf[Len];
    printf("---[%d]-----------------------------------------\n", ++count);
    printf("----------------%s-----------------\n", str);
    printf("--------------------------------------------\n");
    snprintf(buf, Len, "cat /proc/%d/maps > /proc/%d/fd/1", pid, pid);
    printf("------------ files mapped \"%s\"\n", buf);
    // TODO: Looks like the call to system opens the anon_inode:inotify
    // file and uses it in later calls too.  I guess system(3) is kind of
    // a shit libc call anyway.  I do not feel like confirming this...
    system(buf);
    snprintf(buf, Len, "ls -flt --color=yes /proc/%d/fd > /proc/%d/fd/1",
            pid, pid);
    printf("------------ files open \"%s\"\n", buf);
    system(buf);
    printf("---[%d]----------------------------------\n\n", count);
}


static void LoadAndRun(void) {

    PrintStuff("Before Loading");
    void *dlhandle = dlopen(DSO_FILE, RTLD_NOW);
    ASSERT(dlhandle);
    int (*runLabels)(void) = dlsym(dlhandle, "runLabels");
    DASSERT(runLabels);
    CheckLib("libcairo", true);
    CheckLib("libfontconfig", true);
    PrintStuff("After Loading and before running");
    ASSERT(runLabels() == 0);
    ASSERT(0 == dlclose(dlhandle));
    PrintStuff("After dlcose()");
    CheckLib("libcairo", false);
    CheckLib("libfontconfig", false);
}


int main(void) {

    // Note: this program does not link with many libraries at compile
    // time.  It links and then unlinks at run-time.  We are testing
    // that.

    ASSERT(SIG_ERR != signal(SIGSEGV, catcher));
    pid = getpid();

    for(int i=0; i<2; ++i)
        LoadAndRun();

    return 0;
}
