// Looks like it's convenient to let PnMenuBar and PnMenu know
// about each other.  It's just a way to do it of many.  Not
// giving a fuck about OOP (object oriented programming).

struct PnMenuBar;

struct PnMenu {

    // We are trying to keep the struct PnButton, which we build this
    // PnMenu widget on, opaque; so we just have a pointer to it.
    // We'll append the widget button type with menu, W_MENU.
    //
    // This is a different way to inherit a button widget.  We know
    // the widget part of it as it has a struct PnWidget in it.
    //
    // Funny how we can inherit without knowing the full button data
    // structure.  We'll use this pointer as the thing that the user
    // gets.  offsetof(3) is our friend to find other data in this.
    //
    // Does C++ have this kind of opaque inheritance built into the
    // language?  I grant you, it's not as efficient (plus one pointer and
    // dereferencing) but we don't need to expose the full class (struct)
    // data structure in a header file.
    //
    // Why?  Why not.  It's not very efficient and we learn from it.
    // Most code does this shit all the time.
    //
    // We'll use this pointer as the User interface pointer of this
    // structure, instead of a pointer to this structure.  We just append
    // the type (| W_MENU) of this button (like we said above).
    //
    // Perhaps it's called composition, but the API user will not know or
    // see the difference, so I think it's fair to call it inheritance.
    struct PnWidget *button; // inherit (opaquely).

    struct PnWidget *popup;
};

struct PnMenuBar {

    struct PnWidget widget; // inherit first (transparently).

    // Current active menu:
    struct PnMenu *menu;
};
