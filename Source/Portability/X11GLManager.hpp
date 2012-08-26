/*
 * X11GLManager.hpp
 *
 * Created Jun 21 2011
 *    Author: jslocum
 */

#ifndef X11GLMANAGER_CPP_
#define X11GLMANAGER_CPP_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glew.h>
//#include <GL/glxew.h>
#include <GL/gl.h>
#include <GL/glx.h>
//#include <GL/glxext.h>
#include <boost/shared_ptr.hpp>
#include <Portability/Instrumentation/Instrumentation.h>
#include <Portability/PublicInterfaces/OpenGLManager.hpp>
//#include <Gl/glu.h>




typedef GLXContext (*glXCreateContextAttribsARBProc) 
        (Display*, GLXFBConfig, GLXContext, Bool, const int*);

class X11GLManager : public OpenGLManager
{
public:
  X11GLManager(RendererEventHandlerPtr sysCtrl_handler, RendererEventHandlerPtr gl_renderer_handler);
  ~X11GLManager(void);

  //create a window and establish an OpenGL context for rendering in
  bool init(void);
  bool initializeRenderingEnvironment(bool debug_context);
  bool initGLDebug(void);
  
  int GetWindowHeight(void);
  int GetWindowWidth(void);

  void GetGLCLShareParameters(void** handle_pair);

  //used to set/unset the renderer's context as the current GL context
  void SetContextCurrent(void);
  void UnsetContextCurrent(void);

  //swaps the front/back frame buffers, making the most recent frame visible
  void SwapFrameBuffers(void);

  bool HandleWindowEvents(void);

  bool WindowSizeChanged(void);
private:
  //pointer to an X11 display object. Set by getDisplay()
  Display *display;

  //Check that version is high enough to support FBConfigs (1.3 or higher)
  //Set by GetGLVersion()
  int glx_major, glx_minor;
  
  //frame buffer configuration best matching the desirec specifications. Set by
  //ConfigVisual()
  GLXFBConfig bestFbc;

  //Visual info corresponding to bestFbc
  XVisualInfo *vi;

  //color map set by CreateWindow()
  Colormap cmap;
  
  //created by CreateWindow()
  Window win;

  //openGL context created by GetGLContext
  GLXContext ctx;
  
  int windowWidth, windowHeight;
  
  //Gets the display from the X server. Exits program on failure
  void GetDisplay(void);

  //Checks that the version of openGL is compatible with this program
  //Exits program if version is incompatible
  void GetGLVersion(void);

  //Finds the optimal frame buffer context, and grabs the corresponding visual
  //information for configuring the window
  //Exits program if no suitable framebuffers exist
  void ConfigVisual(void);

  //Creates a new window configured with the settings from ConfigVisual, maps
  //the window to a display, and sets the window name
  //Exits program on window creation failure
  void CreateWindow(void);

  //Gets an openGL contexts; attemps to get a version 4.0 context, but falls
  //back to an older version if not available
  //Exits if unable to create any context
  void GetContext(bool debug_context);

  //Handles expose events
  void HandleExpose(void);

  //Handles window-resizing events
  void Resize(int w, int h);

  //Initiallizes an event poller for the xwindows server
  int XEpollInit(struct epoll_event * event);

  void HandleXEvent(XEvent xe);

  bool windowSizeChanged;

  bool checkAndUpdateMouseButton(XButtonEvent);

  void toggleEventRecipient();

  void toggleFullScreen();

  void tellRendererControl(bool renderer_control);

  struct ButtonPressInfo
  {
    int x;
    int y;
    unsigned int which;
    Time time;
    bool lastEvent;
  }* lastMouseButton;


  bool parent;
  RendererEventHandlerPtr parent_handler;
  RendererEventHandlerPtr system_handler;
  RendererEventHandlerPtr event_handler;

  bool isFullscreen;

}; 




#endif /* X11GLMANAGER_CPP_ */
