
// button States
//
// Note: if it's not a toggle button it only has 4 usable states.  The
// toggle button adds another dimension, so it has 2 x 4 = 8 states.  At
// what point is a button a selector that has N * 4 states?  We know that
// you can say that button has 2 (remove the active state and just call it
// a transition, and remove the hover state), 3, (remove the active state
// and just call it a transition) or 5 (add an additional active state)
// states, but we had to start somewhere.
//
// The different between a toggle button and a check button:  In the
// implementation/code there is no difference except in the visuals or
// action animation.  In use the check button tends to change some future
// state that is (implied or explicit) to happen at a later event that
// will poll the state of the check button. Were as, a toggle button tends
// to make changes immediately when the toggle button is toggled.  Check
// button states tend to be batch processed in groups.  Toggle buttons
// event tend to cause immediate actions.
//
// There are also buttons that are in between toggle and check, like a
// toggle button with a LED (light emitting diode) light on it.  The LED
// light looks somewhat like a check, but it's just an indicator of
// whether the button is de-pressed or not.  The power button on an
// electronic device is a good example of a toggle, but if the LED light
// is changed just a little it can look like a check on top of a larger
// toggle.  There are two dynamical elements to this button, the LED
// light (or check) and the larger toggle button this it sits on.
//
#define NORMAL     (0)
#define HOVER      (1)
#define PRESSED    (2)
#define ACTIVE     (3) // animation, released
// TOGGLED is higher bits than the above states.
#define TOGGLED    (1 << 2)

// This must be true.  We cannot share bits between the first 4 state
// flags and TOGGLED.  In code we say:
//ASSERT( !( (NORMAL|HOVER|PRESSED|ACTIVE) & TOGGLED))

enum PnButtonState {

    PnButtonState_Normal   = NORMAL,
    PnButtonState_Hover    = HOVER,
    PnButtonState_Pressed  = PRESSED,
    PnButtonState_Active   = ACTIVE,
    PnButtonState_NumRegularStates,
    PnButtonState_ToggledNormal  = NORMAL  | TOGGLED,
    PnButtonState_ToggledHover   = HOVER   | TOGGLED,
    PnButtonState_ToggledPressed = PRESSED | TOGGLED,
    PnButtonState_ToggledActive  = ACTIVE  | TOGGLED,
    PnButtonState_NumToggleStates
};


#define NUM_FRAMES 10

struct PnButton {

    struct PnWidget widget; // inherit first

    void (*action)(struct PnButton *b, void *userData);

    enum PnButtonState state;
    uint32_t *colors;

    uint32_t frames; // an animation frame counter
};


struct PnToggleButton {

    struct PnButton button; // inherit first
};
