
static inline void Run(struct PnWindow *win) {
#ifdef RUN
    while(pnDisplay_dispatch());
#else
     while(!pnWindow_isDrawn(win))
        ASSERT(pnDisplay_dispatch());
#endif
}
