
struct PnCheck {

    struct PnWidget widget; // inherit first

    // If it's paired with a toggle button.
    struct PnToggleButton *toggleButton;

    // Used in struct PnToggleButton singly listed list.
    struct PnCheck *next;

    bool checked;
};


extern void RemoveToggleCheck(struct PnCheck *c);

