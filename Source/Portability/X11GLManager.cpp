/*
 * X11GLManager.cpp
 *
 * Created Jun 21 2011
 *    Author: jslocum
 *
 *
 * The X11GLManager is responsible for creating a window and acquiring an openGL
 * context for the renderer to draw in. 
 * Much code taken from:
 * http://www.opengl.org/wiki/Tutorial:_OpenGL_4.0_Context_Creation_(GLX)
 */



#include "X11GLManager.hpp"
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "LoopClock.hpp"
#include <X11/X.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xinerama.h>
#include <Portability/PublicInterfaces/VmsKeys.h>
#include <Portability/PublicInterfaces/RendererEvents.hpp>


using namespace std;

static int visual_attribs[] =
  {
    GLX_X_RENDERABLE    , True,
    GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
    GLX_RENDER_TYPE     , GLX_RGBA_BIT,
    GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
    GLX_RED_SIZE        , 8,
    GLX_GREEN_SIZE      , 8,
    GLX_BLUE_SIZE       , 8,
    GLX_ALPHA_SIZE      , 8,
    GLX_DEPTH_SIZE      , 24,
    GLX_STENCIL_SIZE    , 8,
    GLX_DOUBLEBUFFER    , True,
    //GLX_SAMPLE_BUFFERS  , 1,
    //GLX_SAMPLES         , 4,
    None
  };


// Helper to check for extension string presence.  Adapted from:
//   http://www.opengl.org/resources/features/OGLextensions/
static bool isExtensionSupported(const char *extList, const char *extension)
 
{
 
  const char *start;
  const char *where, *terminator;
 
  /* Extension names should not have spaces. */
  where = strchr(extension, ' ');
  if ( where || *extension == '\0' )
    return false;
 
  /* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string. Don't be fooled by sub-strings,
     etc. */
  for ( start = extList; ; ) {
    where = strstr( start, extension );
 
    if ( !where )
      break;
 
    terminator = where + strlen( extension );
 
    if ( where == start || *(where - 1) == ' ' )
      if ( *terminator == ' ' || *terminator == '\0' )
        return true;
 
    start = terminator;
  }
 
  return false;
}
 
static bool ctxErrorOccurred = false;
static int ctxErrorHandler( Display *dpy, XErrorEvent *ev )
{
    ctxErrorOccurred = true;
    return 0;
}


OpenGLManager* OpenGLManager::GetGLManager(RendererEventHandlerPtr sysCtrl_handler, RendererEventHandlerPtr parent_handler)
{
  return new X11GLManager(sysCtrl_handler,parent_handler);
}


X11GLManager::X11GLManager(RendererEventHandlerPtr sysCtrl_handler, RendererEventHandlerPtr gl_renderer_handler)
{
  this->isFullscreen = false;
  this->parent = true;
  this->parent_handler = gl_renderer_handler;
  this->event_handler = this->parent ?  gl_renderer_handler:sysCtrl_handler;
  this->system_handler = sysCtrl_handler;
  lastMouseButton = vms_new ButtonPressInfo();
  tellRendererControl(this->parent);
}

X11GLManager::~X11GLManager(void)
{
  if(this->display)
    {
      glXMakeCurrent( this->display, 0, 0 );
      if(this->ctx)
        glXDestroyContext( this->display, this->ctx );

      if(this->win)
        XDestroyWindow( this->display, this->win );

      if(this->cmap)
        XFreeColormap( this->display, this->cmap );
      
      XCloseDisplay( this->display );
    }
  vms_delete lastMouseButton;
}


typedef void *(*glprocaddfunc)(const GLubyte *);

bool X11GLManager::initGLDebug(void)
{
  return setGLDebugFuncs((glprocaddfunc) glXGetProcAddress);
}


bool X11GLManager::initializeRenderingEnvironment(bool debug_context)
{
  this->windowSizeChanged = true;
  GetDisplay();
  GetGLVersion();
  ConfigVisual();
  CreateWindow();
  GetContext(debug_context);
  cout << "Setting current context" << endl;
  glXMakeCurrent(this->display, this->win, this->ctx);

  //Initiate glew
  glewExperimental = GL_TRUE;
  GLenum error = glewInit(); 
  if (error != GLEW_OK)
    return false;

  return true;
}

void X11GLManager::GetDisplay(void)
{
  this->display = XOpenDisplay(0);
  if ( !this->display )
  {
    cout << "Failed to open X display" << endl;
    exit(1);
  }
}


void X11GLManager::toggleFullScreen()
{
  XEvent xev;

  xev.type = ClientMessage;
  xev.xclient.window = win;
  xev.xclient.message_type = XInternAtom(display, "_NET_WM_STATE", False);
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = 2;
  xev.xclient.data.l[1] = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

  XSendEvent(display, win, True, SubstructureNotifyMask, &xev);
}

bool X11GLManager::WindowSizeChanged(void)
{
  return this->windowSizeChanged;
}

int X11GLManager::GetWindowWidth(void)
{
  this->windowSizeChanged = false;
  return this->windowWidth;
}

int X11GLManager::GetWindowHeight(void)
{
  this->windowSizeChanged = false;
  return this->windowHeight;
}

//used to set/unset the renderer's context as the current GL context
void X11GLManager::SetContextCurrent(void)
{
  glXMakeCurrent(this->display, this->win, this->ctx);
}

void X11GLManager::UnsetContextCurrent(void)
{
  glXMakeCurrent( this->display, 0, 0 );
}

//swaps the front/back frame buffers, making the most recent frame visible
void X11GLManager::SwapFrameBuffers(void)
{
  glXSwapBuffers ( this->display, this->win );
}
  



void X11GLManager::GetGLVersion(void)
{
  // FBConfigs were added in GLX version 1.3.
  if ( !glXQueryVersion( this->display, &glx_major, &glx_minor ) || 
       ( ( glx_major == 1 ) && ( glx_minor < 3 ) ) || ( glx_major < 1 ) )
  {
    cout << "Invalid GLX version: " << glx_major << '.' << glx_minor << endl;
    exit(1);
  }
}
 
void  X11GLManager::ConfigVisual(void)
{
  cout << "Getting matching framebuffer configs\n" << endl;
  int fbcount;
  GLXFBConfig *fbc = glXChooseFBConfig( this->display, DefaultScreen( this->display ),
                                        visual_attribs, &fbcount );
  if ( !fbc )
    {
      cout <<  "Failed to retrieve a framebuffer config" << endl;
      exit(1);
    }
  cout <<  "Found " << fbcount << " matching FB configs." << endl;
 
  // Pick the FB config/visual with the most samples per pixel
  cout <<  "Getting XVisualInfos" << endl;
  int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;
 
  int i;
  for ( i = 0; i < fbcount; i++ )
    {
      XVisualInfo *vi = glXGetVisualFromFBConfig( this->display, fbc[i] );
      if ( vi )
        {
          int samp_buf, samples;
          glXGetFBConfigAttrib( this->display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
          glXGetFBConfigAttrib( this->display, fbc[i], GLX_SAMPLES       , &samples  );
 
          cout <<  "  Matching fbconfig " << i <<", visual ID " << vi->visualid
               << "SAMPLE_BUFFERS = " << samp_buf << ", SAMPLES = " << samples 
               << endl;
 
          if ( best_fbc < 0 || (samp_buf && samples > best_num_samp) )
            best_fbc = i, best_num_samp = samples;
          if ( worst_fbc < 0 || !samp_buf || samples < worst_num_samp )
            worst_fbc = i, worst_num_samp = samples;
        }
      XFree( vi );
    }
 
  this->bestFbc = fbc[ best_fbc ];
 
  // Be sure to free the FBConfig list allocated by glXChooseFBConfig()
  XFree( fbc );
 
  // Get a visual
  this->vi = glXGetVisualFromFBConfig( this->display, this->bestFbc );
  cout <<  "Chosen visual ID = " << this->vi->visualid << endl;
}

void X11GLManager::CreateWindow(void)
{
  
  cout <<  "Creating colormap" << endl;
  XSetWindowAttributes swa;

  swa.colormap = this->cmap = XCreateColormap( this->display,
                                         RootWindow( this->display, this->vi->screen ), 
                                         this->vi->visual, AllocNone );
  swa.background_pixmap = None ;
  swa.border_pixel      = 0;
  swa.event_mask        = KeyPressMask | KeyReleaseMask | ButtonPressMask | 
    ButtonReleaseMask | StructureNotifyMask | PointerMotionMask;
 
  cout <<  "Creating window" << endl;
  this->win = XCreateWindow( this->display, RootWindow( this->display, this->vi->screen ), 
                              0, 0, 100, 100, 0, this->vi->depth, InputOutput, 
                              this->vi->visual, 
                              CWBorderPixel|CWColormap|CWEventMask, &swa );
  if ( !this->win )
  {
    cout <<  "Failed to create window." << endl;
    exit(1);
  }
 
  // Done with the visual info data
  XFree( this->vi );
 
  XStoreName( this->display, this->win, "GL 4.2 Window" );
 
  cout <<  "Mapping window" << endl;
  XMapWindow( this->display, this->win );
 
}
 

void X11GLManager::GetContext(bool debug_context)
{
  // Get the default screen's GLX extension list
  const char *glxExts = glXQueryExtensionsString( this->display,
                                                  DefaultScreen( this->display ) );
 
  // NOTE: It is not necessary to create or make current to a context before
  // calling glXGetProcAddressARB
  glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
  glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)
           glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );
 
  // Install an X error handler so the application won't exit if GL 4.2
  // context allocation fails.
  //
  // Note this error handler is global.  All display connections in all threads
  // of a process use the same error handler, so be sure to guard against other
  // threads issuing X commands while this code is running.
  ctxErrorOccurred = false;
  int (*oldHandler)(Display*, XErrorEvent*) =
      XSetErrorHandler(&ctxErrorHandler);
 
  // Check for the GLX_ARB_create_context extension string and the function.
  // If either is not present, use GLX 1.3 context creation method.
  if ( !isExtensionSupported( glxExts, "GLX_ARB_create_context" ) ||
       !glXCreateContextAttribsARB )
  {
    cout <<  "glXCreateContextAttribsARB() not found"
            " ... using old-style GLX context" << endl;
    this->ctx = glXCreateNewContext( this->display, this->bestFbc, GLX_RGBA_TYPE, 0, True );
  }
 
  // If it does, try to get a GL 4.2 context!
  else
  {
    int context_attribs[] =
      {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
        GLX_CONTEXT_MINOR_VERSION_ARB, 2,
        GLX_CONTEXT_FLAGS_ARB  ,
        GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB |
        (debug_context ? GLX_CONTEXT_DEBUG_BIT_ARB : 0),
        0
      };
 
    cout <<  "Creating context" << endl;
    this->ctx = glXCreateContextAttribsARB( this->display, this->bestFbc, 0,
                                      True, context_attribs );
 
    // Sync to ensure any errors generated are processed.
    XSync( this->display, False );
    if ( !ctxErrorOccurred && this->ctx )
      cout <<  "Created GL 4.2 context" << endl;
    else
    {
      // Couldn't create GL 4.2 context.  Fall back to old-style 2.x context.
      // When a context version below 4.2 is requested, implementations will
      // return the newest context version compatible with OpenGL versions less
      // than version 4.2.
      // GLX_CONTEXT_MAJOR_VERSION_ARB = 2
      context_attribs[1] = 2;
      // GLX_CONTEXT_MINOR_VERSION_ARB = 0
      context_attribs[3] = 0;
 
      ctxErrorOccurred = false;
 
      cout <<  "Failed to create GL 4.1 context"
              " ... using old-style GLX context" << endl;
      this->ctx = glXCreateContextAttribsARB( this->display, this->bestFbc, 0, 
                                        True, context_attribs );
    }
  }
 
  // Sync to ensure any errors generated are processed.
  XSync( this->display, False );
 
  // Restore the original error handler
  XSetErrorHandler( oldHandler );
 
  if ( ctxErrorOccurred || !this->ctx )
  {
    cout <<  "Failed to create an OpenGL context" << endl;
    exit(1);
  }
 
  // Verifying that context is a direct context
  if ( ! glXIsDirect ( this->display, this->ctx ) )
  {
    cout <<  "Indirect GLX rendering context obtained" << endl;
  }
  else
  {
    cout <<  "Direct GLX rendering context obtained" << endl;
  }

}


//passes window resizing events to the renderer
void X11GLManager::Resize(int w, int h)
{
  this->windowSizeChanged = true;
  this->windowWidth = w;
  this->windowHeight = h;
}


int X11GLManager::XEpollInit(struct epoll_event * event)
{
  //file descriptor used by epoll to read events.
  int x_connection = XConnectionNumber(this->display);

  //TODO: Request keyboard, mouse and resize events from XWindows

  //Create a new event poller to watch one file descriptor (the Xwindows server)
  int epfd = epoll_create(1);
  
  //Config the new epoller to look for incoming signals from Xwindows
  event->events = EPOLLIN | EPOLLERR | EPOLLPRI;
  event->data.fd = x_connection;
  epoll_ctl(epfd, EPOLL_CTL_ADD, x_connection, event);
  //return the file descriptor corresponding to our new epoller
  return epfd;
}
 

void X11GLManager::HandleXEvent(XEvent xe)
{
  bool enqueue = false;
  RendererEventPtr event_ptr;
  switch(xe.type)
    {
    case KeyPress:
      event_ptr = RendererEventPtr(vms_new RendererEvent((void*)&xe,KEY_DOWN));
      if(event_ptr->data->key_data->which == VMS_ENTER && event_ptr->mask & SHIFT_MASK && event_ptr->mask & CTRL_MASK)
	      toggleEventRecipient();
      enqueue = true;
      lastMouseButton->lastEvent = false;
      break;
    case KeyRelease:
      event_ptr = RendererEventPtr(vms_new RendererEvent((void*)&xe,KEY_UP));
      enqueue = true;
      lastMouseButton->lastEvent = false;
      break;
    case ButtonPress:
      if(xe.xbutton.button == Button4 || xe.xbutton.button == Button5)
	      event_ptr = RendererEventPtr(vms_new RendererEvent((void*)&xe,MOUSE_SCROLL));
      else
	      event_ptr = RendererEventPtr(vms_new RendererEvent((void*)&xe,MOUSE_DOWN));
      enqueue = true;
      break;
    case ButtonRelease:
      if(checkAndUpdateMouseButton(xe.xbutton))
	      event_ptr = RendererEventPtr(vms_new RendererEvent((void*)&xe,MOUSE_DBLCLK)); //x11 doesn't know about double clicks so it'll have to be handled internally
      else
	      event_ptr = RendererEventPtr(new RendererEvent((void*)&xe,MOUSE_UP));
      enqueue = true;
      break;
    case MotionNotify:
      event_ptr = RendererEventPtr(vms_new RendererEvent((void*) &xe,MOUSE_MOVE));
      enqueue = true;
      break;
    case ConfigureNotify:
	    enqueue = false;
      XConfigureEvent* xce = (XConfigureEvent *) &xe;
      cout <<"resizing to "<< xce->width<<" pixels wide and "<< xce->height <<" pixels high" << endl;
      Resize(xce->width, xce->height);
      break;
    }
  //this returns whether it used it or not, for the moment it's ignored 
  this->HandleGlobalEvents(event_ptr);
  if(enqueue)
  {
    hfPrintf("Enqueued event of type %u",event_ptr->type);
    this->event_handler->enqueueEvent(event_ptr);
  }
}

bool X11GLManager::HandleWindowEvents(void)
{
  XFlush(this->display);
  int poll = XEventsQueued(this->display, QueuedAlready);
  bool new_events = (poll > 0);
  for(;poll > 0;poll--)
  {
    XEvent xe;
    XNextEvent(this->display, &xe);
    HandleXEvent(xe);
  }
  return new_events;
}


void X11GLManager::GetGLCLShareParameters(void** handle_pair)
{
  handle_pair[0] = this->display;
  handle_pair[1] = this->ctx;
}

#define DIST_THRESH 5;
#define TIME_THRESH 500; //half second double click is standard 

bool X11GLManager::checkAndUpdateMouseButton(XButtonEvent xe)
{
  if(xe.button == Button4 || xe.button == Button5) //if it is a scroll event it's not a double click
  {
    lastMouseButton->lastEvent = false;
    return false;
  }
  bool distance_x = abs(lastMouseButton->x - xe.x) < DIST_THRESH;
  bool distance_y = abs(lastMouseButton->y - xe.y) < DIST_THRESH;
  bool time = xe.time - lastMouseButton->time < TIME_THRESH;
  bool ret = lastMouseButton->lastEvent && distance_x && distance_y && time && (lastMouseButton->which == xe.button);
  lastMouseButton->which = xe.button;
  lastMouseButton->x = xe.x;
  lastMouseButton->y = xe.y;
  lastMouseButton->time = xe.time;
  lastMouseButton->lastEvent = true;
  return ret;
}

void X11GLManager::toggleEventRecipient()
{
  parent = !parent;
  this->event_handler = parent ? this->parent_handler:this->system_handler;
  tellRendererControl(this->parent);
}

void X11GLManager::tellRendererControl(bool renderer_control)
{
  SoftwareMessageData* data = vms_new SoftwareMessageData();
  data->msg_type = RENDERER_HANDLES_EVENTS;
  bool* content = vms_new bool;
  *content = renderer_control;
  data->contents = (void*)content;
  RendererEvent* toggleEvent = new RendererEvent(data);
  this->parent_handler->enqueueEvent(RendererEventPtr(toggleEvent));
  }
