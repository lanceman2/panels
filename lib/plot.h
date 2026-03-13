
enum PnPlotType {

    // TODO: since there are just two values use a bool and not a enum
    //
    // Static - drawn and does not change except for zooming and widget
    // resize.
    PnPlotType_static,

    // Dynamic - every frame may change
    PnPlotType_dynamic // like an oscilloscope
};

enum PnDrawMethod {

    PnDrawMethod_cairo,
    PnDrawMethod_beam,
    PnDrawMethod_raw
};

struct PnBeamPoint {

    struct PnBeamPoint *next;
    uint32_t color;
};


// For time faded beam oscilloscope using just single pixel points per
// user plot draw point.
//
// This is the use less memory method (not as big as the widget pixel
// buffer, if there are fewer stored points per draw frame):
//
// This uses a linked list of "points" that when in use are drawn at every
// frame; for example every 0.01667 seconds (at 60 Hz) we draw and fade
// all active points from first to last.  For performance this assumes
// that the list is small compared to the number of pixels in the graph
// widget.
//
// Drawing more than the BeamPoint list in a frame will simply overwrite
// the oldest values of the BeamPoint circular buffer.  It's up to the API
// user to make the BeamPoint circular buffer long enough to display what
// they want to see.
//
// 1. The Bad: It could just end up just drawing at just one X,Y pixel
// position for many points in the list.  It would be stupid about that,
// but it would still work.  Or, the list could be too short and not
// enough points are drawn; but that's more of a API user choose, they
// get what they ask for.  The list memory should not exceed the memory
// needed to cover all the pixels in the graph widget; otherwise they
// should use method 2 shown next (if it's implemented).
//
// 2. An alternative is to make (more memory) an array of points that maps
// one to one to all the available pixels in the graph widget. The linked
// list is embedded in the array.  When points in the array are
// overwritten (by user plotting) the color, and prev (and next?) of that
// point and it's neighboring points are patched.  This could keep us from
// redundant pixel coloring, but at a cost of using lots of unused memory
// given that we are coloring only a small portion of all the pixels that
// the graph widget has.  We'd copy the embedded list to the graph widget
// pixel buffer for every draw frame, as this does now.  The old
// quickscope project does this.
//
// We should be able to keep a user (and underlying) interfaces the same
// for both of the two methods (1, 2 above).  So, we could seamlessly
// switch between the two methods, assuming we implement both methods.
//
struct PnBeam {

    // The list of allocated points in this beam.
    struct PnBeamPoint *first, *last;

    uint32_t brightColor, dimColor;

    // A looping age (frame) counter.
    uint32_t age;

    // An allocated array.
    struct PnBeamPoint *points;

    // The number of points allocated in points array.
    // This limits the number of points that can 
    size_t numPoints;
};


struct PnPlot {

    // This inherits a panels widget action callback thingy.
    struct PnCallback callback; // inherit first.

    struct PnGraph *graph;

    union {

        // TODO: Do we want to structure this union so that we can draw a
        // fading scope beam with Cairo?  So many options ...
        //
        // Or is it that we just think of this union as a list of different
        // independent drawing methods.

        struct {
            // For a static or scope plot these are connected to the
            // appropriate object at the start of a plot draw which is at
            // the time of the panels widget action callback.
            //
            // This is just so we do not deference one more pointer, like
            // p->point and not p->g->point, in a tight Cairo drawing
            // loop.
            cairo_t *line, *point;
        } cairo;


        struct PnBeam beam;

        struct {
        } raw;
    };

    struct PnZoom *zoom;

    // TODO: make this a bool.
    enum PnPlotType type; // static or scope

    enum PnDrawMethod drawMethod; // Cairo, beam, or raw.

    // Just for scope plots.  Is zero for static plots.
    uint32_t shiftX, shiftY;

    uint32_t lineColor, pointColor;

    double lineWidth, pointSize;
    // The last point drawn is needed to draw lines when
    // there is a line being drawn in the future.
    double x, y; // last point
};

