#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "debug.h"

static inline bool set_cloexec(int fd) {

    int flags = fcntl(fd, F_GETFD);
    if(flags == -1) {
        ERROR("fcntl(%d,F_GETFD) failed", fd);
	return true;
    }
    if(fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1) {
        ERROR("fcntl(%d,%d | FD_CLOEXEC) failed", fd, flags);
	return true;
    }
    return false;
}


static inline void randname(char *buf) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long r = ts.tv_nsec;
    for (int i = 0; i < 6; ++i) {
        buf[i] = 'A'+(r&15)+(r&16)*2;
	r >>= 5;
    }
}

static int anonymous_shm_open(void) {

    // TODO: Add a bigger name differentiator, so we can run more
    // programs that use this library.

    char name[] = "/panels-XXXXXX";
    int retries = 77;

    do {
        randname(name + strlen(name) - 6);
	--retries;
	// shm_open guarantees that O_CLOEXEC is set
        errno = 0;
	int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL,
                S_IWUSR | S_IRUSR | S_IWOTH | S_IROTH);
	if(fd >= 0) {
	    if(shm_unlink(name)) {
                ERROR("shm_unlink(\"%s\") failed", name);
                close(fd);
                return -1;
            }
	    return fd;
	}
    } while(retries > 0 && errno == EEXIST);

    ERROR("Failed to get shm_open() file open");

    return -1;
}

static inline int getFileFromEnv(void) {

    const char *env = getenv("XDG_RUNTIME_DIR");
    const char template[] = "panels-XXXXXX";
    if(env == NULL) {
	NOTICE("XDG_RUNTIME_DIR is not set");
	return -1;
    }

    size_t name_size = strlen(template) + 1 + strlen(env) + 1;
    char *name = malloc(name_size);
    ASSERT(name, "malloc(%zu) failed", name_size);
    snprintf(name, name_size, "%s/%s", env, template);

    int fd = mkstemp(name);
    if(fd < 0) {
        WARN("mkstemp() failed");
	free(name);
	return -1;
    }

    // unlink asap; the file stays valid until all references close
    unlink(name);
    free(name);

    if(set_cloexec(fd)) {
        close(fd);
	return -1;
    }
    return fd;
}

// The caller must close the returned file descriptor if it is > -1.
//
int create_shm_file(size_t size) {

    int fd = getFileFromEnv();
    if(fd <= -1)
        fd = anonymous_shm_open();
    if(fd <= -1)
        return fd;

    if(ftruncate(fd, size) < 0) {
        ERROR("ftruncate(,%jd) failed", size);
	close(fd);
	return -1;
    }

    return fd;
}
