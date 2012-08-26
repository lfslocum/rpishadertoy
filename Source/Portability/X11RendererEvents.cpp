#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <Include/VMS_Defines.h>
#include <Portability/PublicInterfaces/RendererEvents.hpp>
#include <Portability/PublicInterfaces/VmsKeys.h>
#include <X11/keysymdef.h>
#include <locale>

//there is not a wheel delta function in X11 so it'll have to be constant for each button press
#define WHEEL_INCREMENT 5
char RendererEvent::current_mask = 0x0;

button mouseType(unsigned int buttonID)
{
  switch(buttonID)
  {
    case Button1:
      return MOUSE_LEFT;
    case Button2:
      return MOUSE_MIDDLE;
    case Button3:
      return MOUSE_RIGHT;
    case Button4:
    case Button5:
      return MOUSE_WHEEL; 
    default:
      return MOUSE_OTHER;
  }
}

char getNewMask(KeySym sym)
{
  switch (sym)
  {
    case XK_Escape:
      return ESCAPE_MASK;
    case XK_Shift_L:
    case XK_Shift_R:
      return SHIFT_MASK;
    case XK_Control_L:
    case XK_Control_R:
      return CTRL_MASK;
    case XK_Alt_L:
    case XK_Alt_R:
      return ALT_MASK;
    case XK_Meta_L:
    case XK_Meta_R:
      return META_MASK;
  }
  return 0x0;
}

void applyShiftMask(KeySym& sym)
{
  switch(sym)
  {
    case XK_equal:
      sym = XK_plus;
      break;
    case XK_grave:
      sym = XK_asciitilde;
      break;
    default:
      break;
   }
}

char mapKeys(KeySym sym)
{
  //char key = (char)sym & 0x0ff;
  char key = (char)sym;
  switch(sym)
  {
    case XK_F1:
      return VMS_F1;
    case XK_F2:
      return VMS_F2;
    case XK_F3:
      return VMS_F3;
    case XK_F4:
      return VMS_F4;
    case XK_F5:
      return VMS_F5;
    case XK_F6:
      return VMS_F6;
    case XK_F7:
      return VMS_F7;
    case XK_F8:
      return VMS_F8;
    case XK_F9:
      return VMS_F9;
    case XK_F10:
      return VMS_F10;
    case XK_F11:
      return VMS_F11;
    case XK_F12:
      return VMS_F12;
    case XK_Home:
      return VMS_HOME;
    case XK_End:
      return VMS_END;
    case XK_Up:
      return VMS_UP;
    case XK_Down:
      return VMS_DOWN;
    case XK_Right:
      return VMS_RIGHT;
    case XK_Left:
      return VMS_LEFT;
    case XK_Page_Up:
      return VMS_PGUP;
    case XK_Page_Down:
      return VMS_PGDN;
    case XK_Return:
    case XK_KP_Enter:
      return VMS_ENTER;
    case XK_KP_Space:
      return VMS_SPACE;
    case XK_asciitilde:
      return VMS_TILDE;
    case XK_Escape:
      return VMS_ESC;
    case XK_Shift_L:
    case XK_Shift_R:
      return VMS_SHIFT;
    case XK_Control_L:
    case XK_Control_R:
      return VMS_CTRL;
    case XK_Alt_L:
    case XK_Alt_R:
      return VMS_ALT;
    case XK_plus:
    case XK_KP_Add:
      return VMS_PLUS;
    case XK_minus:
    case XK_KP_Subtract:
      return VMS_MINUS;
    default:
      return toupper(key);
  }
}



RendererEvent::RendererEvent(void* event_ptr, eventtype type)
{
  hfPrintf("Creating renderer event");
  this->data = vms_new EventDataWrapper();
  this->type = type;
  switch(type)
  {
    case MOUSE_UP:
    case MOUSE_DOWN:
      {
        XButtonEvent event = ((XEvent*)event_ptr)->xbutton;
        MousePressData* press_data = vms_new MousePress();
        this->data->press_data = press_data;
        press_data->down = type == MOUSE_DOWN; 
        press_data->which = mouseType(event.button);
        press_data->where.window_x = event.x;
        press_data->where.window_y = event.y;
        break;
      }
    case MOUSE_DBLCLK:
      {
        XButtonEvent event = ((XEvent*)event_ptr)->xbutton;
        MouseDoubleData* dbl_data = vms_new MouseDoubleData();
        this->data->double_data = dbl_data;
        dbl_data->which = mouseType(event.button);
        dbl_data->where.window_x = event.x;
        dbl_data->where.window_y = event.y;
        break;
      }
    case MOUSE_MOVE: 
      {
        XMotionEvent event = ((XEvent*)event_ptr)->xmotion;
        MouseMoveData* move = vms_new MouseMoveData();
        this->data->move_data = move;
        move->window_x = event.x;
        move->window_y = event.y;
        break;
      }
    case MOUSE_SCROLL:
      {
        XButtonEvent event = ((XEvent*)event_ptr)->xbutton;
        MouseWheelData* wheel_data = vms_new MouseWheelData();
        this->data->wheel_data = wheel_data;
        wheel_data->where.window_x = event.x;
        wheel_data->where.window_y = event.y;
        wheel_data->wheel_delta = event.button == Button4 ? WHEEL_INCREMENT : -WHEEL_INCREMENT;
        break;
      }
    case KEY_UP:
    case KEY_DOWN:
      {
        XKeyEvent event = ((XEvent*)event_ptr)->xkey;
        KeySym sym = XLookupKeysym(&event,0);
        char maskUpdate = getNewMask(sym);
        current_mask = type == KEY_UP ? current_mask & ~maskUpdate:current_mask | maskUpdate; 
        if(current_mask & SHIFT_MASK)
          applyShiftMask(sym);
        this->data->key_data = vms_new KeystrokeData();
        this->data->key_data->down = false;
        this->data->key_data->which = mapKeys(sym);
        break;
      }
    case SOFTWARE:
	break;
    default:
	break;
  }
  this->mask = current_mask;
}
