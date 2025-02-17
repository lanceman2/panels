
// button States
//
// Note: if it's not a toggle button it only has 4 usable states.  The
// toggle button adds another dimension (call it selection), so it has 2 x
// 4 = 8 states.  At what point is a button a selector that has N * 4
// states?
//
#define NORMAL     0
#define HOVER      1
#define PRESSED    2
#define ACTIVE     3 // animation, released
// TOGGLED is higher bits than the above states.
#define TOGGLED    1 << 2 // on with this as normal
#define NUM_STATES 

struct PnButton {

    struct PnWidget widget; // first inherit

    uint32_t state;
    uint32_t colors[NUM_STATES];
};
