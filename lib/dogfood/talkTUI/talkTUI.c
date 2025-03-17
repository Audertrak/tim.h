// YOUR ONLY CONCERN RIGHT NOW IS RENDERING SHAPES IN THE TERMINAL ON A WINDOWS
// DEVICE FOR THE LOVE OF GOD STOP SCOPE CREEPING

typedef void (*func)();

typedef struct {
  int x;
  int y;
  int z;
} Dim;

// window zero position; top left corner
const Dim ORIGIN = {0, 0, 0};

Dim Window_Size;

typedef enum { RGB, HSV, HSL, HEX } Colorspace;

typedef struct {
  int Red;
  int Green;
  int Blue;
  int Alpha;
} RGB_Color;

typedef struct {
  int Hue;
  int Saturation;
  int Value;
  int Alpha;
} HSV_Color;

typedef struct {
  int Hue;
  int Saturation;
  int Lightness;
  int Alpha;
} HSL_Color;

// color temporarily set to
typedef struct {
  int id;       // used to keep track of str order
  char Glyph;   // utf-8 character
  RGB_Color FG; // character color
  RGB_Color BG; // window color
} Cell;

typedef struct {
  int id;    // unique id of a rectangle container
  Dim Start; // starting point of a rectangle relative to the origin
  Dim Size;  // size of a rectangle relative to its origin
  Cell *Contents[];
} Rect;

/* Additional Properties
 *	- Speed
 *	- Accelaration
 */

/* 6 Degrees of Freedom
 *	- x,y,z
 *	- Pitch
 *	- Yaw
 *	- Roll
 */

/* Rectangle functions
 *	- Create
 *	- Draw
 *	- Resize
 *	- Destroy
 */

typedef struct {
  func compute;
  func draw;
  func clear;
} Event_Loop;

// reserve memory for a rectangle and its contents
void Create_Rect() {}

// draw the rectangle on the terminal
void Draw_Rect() {}

// resize the rectangle
void Resize_Rect() {}

// resize a single dimension
int Resize(int int_val) { return int_val; }

// get the size of the terminal	window
Dim Get_Window_Size() { return Window_Size; }

// move the shape in the direction of a straight line vector along one or more
// axis
void Translate(Rect rect, Dim start, Dim end) {}

// retain the orignal properties, but perform a skew along one or more
// dimensions
void Transform(Rect rect, Dim *start, Dim end) {}

int main() {
  Event_Loop event_loop;
  Rect test;

  return 0;
}
