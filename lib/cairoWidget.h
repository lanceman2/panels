
// button States
//
// Note: if it's not a toggle button it only has 4 usable states.
// The toggle button adds another dimension 
//
// 
#define NORMAL     0
#define FOCUS      1
#define PRESSED    2
#define ACTIVE     3 // animation
// TOGGLED is higher bits than the above states.
#define TOGGLED    1 << 2 // on with this as normal
#define NUM_STATES 

struct PnButton {

    struct PnWidget widget; // first inherit

    uint32_t state;
    uint32_t colors[];
};
