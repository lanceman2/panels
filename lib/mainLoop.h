// This main loop shit is not required to be used by libpanels API users.
// This is very opinionated, so it's very optional.  It's an option in the
// PnDisplay object.  libpanels API users can to make their own main loop
// code or they can use this code indirectly through pnDisplay_run() and
// other pnDisplay functions.

// Used to add file descriptors to the optional main loop stuff.
struct PnFD {

    int fd; // could be a reading or writing file descriptor.
    void *userData;
    bool (*callback)(int fd, void *userData);

    // We keep a list of struct PnFD:
    struct PnFD *next;

    bool isReader;
};


struct PnMainLoop {
    //
    // We want to use epoll_wait(2)?  We'd need to loop through all the
    // readers and writers for each select(2) call.  epoll_wait(2) returns
    // user data that makes it so we do not have to loop through all the
    // readers and writers.  We are expecting less than three file
    // descriptors, so it may be okay.  Using epoll_wait(2) requires a bit
    // more code, and there should be no performance gain if there is
    // three (4, 5, ?) or less file descriptors.
    //
    // There's also the question of whither to use the blocking or
    // non-blocking read and write calls.  For now we'll use blocking.  I
    // think that non-blocking is preferred for using epoll_wait(2).
    // Hence the edge_trigger parameter in the pnDisplay_addReader() call.
    // If edge_trigger is set the user must "drain" the file descriptor.
    //
    struct PnFD *readers; // list of readers
    struct PnFD *writers; // list of writers
    int epollFd;
};


static inline void FreeFd(struct PnFD *f) {

    DZMEM(f, sizeof(*f));
    free(f);
}

// TODO: We are not dealing with the error from this.
//
static inline bool RemoveFd(struct PnMainLoop *ml,
        struct PnFD **list, struct PnFD *f) {

    DASSERT(list);
    DASSERT(*list);
    DASSERT(f);
    DASSERT(f->fd >= 0);
    DASSERT(ml);
    DASSERT(ml->epollFd >= 0);

    bool ret = false; // default to success being return false.

    if(epoll_ctl(ml->epollFd, EPOLL_CTL_DEL, f->fd, 0) != 0) {
        ERROR("epoll_ctl(%d,EPOLL_CTL_DEL,,) failed", ml->epollFd);
        ret = true; // error
    }

    struct PnFD *prev = 0;
    struct PnFD *i = *list;
    for(; i; i = i->next) {
        if(i == f)
            break;
        prev = i;
    }
    // It must be found in the list.
    DASSERT(i == f);

    if(prev)
        prev->next = f->next;
    else
        *list = f->next;

    FreeFd(f);
    return ret;
}


static inline void pnMainLoop_destroy(struct PnMainLoop *ml) {

    DASSERT(ml);

    // The user is responsible for closing the file descriptor.
    while(ml->readers)
        RemoveFd(ml, &ml->readers, ml->readers);
    while(ml->writers)
        RemoveFd(ml, &ml->writers, ml->writers);

    if(ml->epollFd >= 0)
        close(ml->epollFd);

    DZMEM(ml, sizeof(*ml));
    free(ml);
}

static inline struct PnMainLoop *pnMainLoop_create() {

    struct PnMainLoop *ml = calloc(1, sizeof(*ml));
    ASSERT(ml, "calloc(1,%zu) failed", sizeof(*ml));

    ml->epollFd = epoll_create(1);
    if(ml->epollFd < 0) {
        ERROR("epoll_create() failed");
        pnMainLoop_destroy(ml);
        return 0;
    }

    return ml;
}

static inline struct PnFD *AddFD(struct PnFD **l, int fd,
        bool (*callback)(int fd, void *userData), void *userData) {

    struct PnFD *n = calloc(1, sizeof(*n));
    ASSERT(n, "calloc(1, %zu) failed", sizeof(*n));

    DASSERT(fd >= 0);

    n->fd = fd;
    n->callback = callback;
    n->userData = userData;

    if(!*l) {
        // It's first in the list.
        *l = n;
        return n;
    }

    // Add "n" to the end of the list:
    struct PnFD *last = *l;
    while(last->next) last = last->next;
    last->next = n;
    return n;
}


static inline bool pnMainLoop_addReader(struct PnMainLoop *ml,
        int fd, bool edge_trigger,
        bool (*read)(int fd, void *userData), void *userData) {

    DASSERT(ml);
    DASSERT(ml->epollFd >= 0);

    struct PnFD *n = AddFD(&ml->readers, fd, read, userData);
    n->isReader = true;

    // I think that the kernel copies from this data, so I just
    // built this structure on the stack here:
    struct epoll_event ee;
    ee.events = EPOLLIN | (edge_trigger?EPOLLET:0);
    ee.data.ptr = n;

    if(epoll_ctl(ml->epollFd, EPOLL_CTL_ADD, fd, &ee) != 0) {
        ERROR("epoll_ctl(%d,,,) failed", ml->epollFd);
        return true;
    }

    return false; // success
}

static inline bool
pnMainLoop_removeReader(struct PnMainLoop *ml, int fd) {
    DASSERT(ml);
    DASSERT(ml->epollFd >= 0);
    DASSERT(fd >= 0);
    DASSERT(ml->readers);

    struct PnFD *f = ml->readers;
    for(; f; f = f->next)
        if(f->fd == fd) {
            RemoveFd(ml, &ml->readers, f);
            break;
        }

    if(!f) {
        ERROR("Reader with fd=%d not found", fd);
        return true;
    }

    return false; // success.
}

static inline void
DoReader(struct PnMainLoop *ml, struct PnFD *f) {

    DASSERT(ml);
    DASSERT(ml->epollFd >= 0);
    DASSERT(f);
    DASSERT(f->fd >= 0);
    DASSERT(f->callback);
    DASSERT(ml->readers);

    if(f->callback(f->fd, f->userData))
        RemoveFd(ml, &ml->readers, f);
}

static inline void
DoWriter(struct PnMainLoop *ml, struct PnFD *f) {

    DASSERT(ml);
    DASSERT(ml->epollFd >= 0);
    DASSERT(f);
    DASSERT(f->fd >= 0);
    DASSERT(f->callback);
    DASSERT(ml->writers);

    if(f->callback(f->fd, f->userData))
        RemoveFd(ml, &ml->writers, f);
}


#define MAX_EVENTS  (7) // for epoll_wait().


static inline bool pnMainLoop_wait(struct PnMainLoop *ml) {

    DASSERT(ml);
    DASSERT(ml->epollFd >= 0);

    DASSERT(ml->readers || ml->writers);
    if(!ml->readers && !ml->writers) {
        ERROR("No readers or writers loaded");
        return true; // error
    }

    // TODO: Should this "ev" array be on the stack here, or
    // in the struct PnMainLoop?
    //
    struct epoll_event ev[MAX_EVENTS];
    int nfds = epoll_wait(ml->epollFd, ev, MAX_EVENTS, -1/*timeout*/);
    // We are not using a timeout, so it should not return 0.
    DASSERT(nfds != 0);
    if(nfds < 0) {
        ERROR("epoll_wait(%d,,,) failed", ml->epollFd);
        return false;
    }
    for(int i=0; i<nfds; ++i) {
        struct PnFD *f = ev[i].data.ptr;
        if(f->isReader)
            DoReader(ml, f);
        else
            DoWriter(ml, f);
    }

    return false; // success
}
