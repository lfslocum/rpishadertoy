#include <boost/shared_ptr.hpp>			// boost used for SceneObjectWrapperPtr
#include <Portability/PublicInterfaces/RendererEvents.hpp>
#include <Portability/PublicInterfaces/VmsKeys.h>


class OpenGLManager {
public:
  //returns a system-appropriate openGL manager object. 
  static OpenGLManager* GetGLManager(RendererEventHandlerPtr
                                     sysCtrl_event_handler,
                                     RendererEventHandlerPtr
                                     gl_renderer_event_handler);
  
  virtual ~OpenGLManager(void){};

  virtual bool initializeRenderingEnvironment(bool debug_context) = 0;
  virtual bool initGLDebug(void) = 0;
  
  bool setGLDebugFuncs(void *(*ext_proc)(const GLubyte *)) {
    glDebugMessageCallbackARB =
      (PFNGLDEBUGMESSAGECALLBACKARBPROC)
      ext_proc((const GLubyte*) "glDebugMessageCallbackARB");
    lfPrintf("glDebugMessageCallbackARB set to %p", glDebugMessageCallbackARB);

    glDebugMessageControlARB =
      (PFNGLDEBUGMESSAGECONTROLARBPROC)
      ext_proc((const GLubyte*) "glDebugMessageControlARB");
    lfPrintf("glDebugMessageControlARB set to %p", glDebugMessageControlARB);
  
    glDebugMessageInsertARB  =
      (PFNGLDEBUGMESSAGEINSERTARBPROC )
      ext_proc((const GLubyte*) "glDebugMessageInsertARB ");
    lfPrintf("glDebugMessageInsertARB  set to %p", glDebugMessageInsertARB);
  
    glGetDebugMessageLogARB  =
      (PFNGLGETDEBUGMESSAGELOGARBPROC )
      ext_proc((const GLubyte*) "glGetDebugMessageLogARB ");
    lfPrintf("glGetDebugMessageLogARB  set to %p", glGetDebugMessageLogARB);
  
    this->debug_loaded_successfully =
      (glDebugMessageCallbackARB != 0) && (glDebugMessageControlARB != 0);
    return this->debug_loaded_successfully;
  }

  
  //create a window and establish an OpenGL context for rendering in
  bool init(bool debug_context) {
    return initializeRenderingEnvironment(debug_context) &&
      ((debug_context && initGLDebug()) || !debug_context);
  }
  
  bool debug_loaded_successfully;
  
  virtual int GetWindowWidth(void) = 0;
  virtual int GetWindowHeight(void) = 0;

  //used to set/unset the renderer's context as the current GL context
  virtual void SetContextCurrent(void) = 0;
  virtual void UnsetContextCurrent(void) = 0;

  //swaps the front/back frame buffers, making the most recent frame visible
  virtual void SwapFrameBuffers(void) = 0;

  virtual bool WindowSizeChanged(void) = 0;

  virtual bool HandleWindowEvents(void) = 0;

  //returns a platform-specific device/context handle pair.
  //attribs must have space for two void*'s
  virtual void GetGLCLShareParameters(void** handle_pair) = 0;

  virtual void toggleFullScreen() = 0;

  virtual void toggleEventRecipient() = 0;

  bool HandleGlobalEvents(RendererEventPtr event) //checks every event to see if it is a global one, returns whether it used it or not
  {
    if(!event)
	return false;
    
    if(event->type == KEY_DOWN)
	{
      if((event->data->key_data->which == VMS_ENTER) && (event->mask & SHIFT_MASK) && (event->mask & CTRL_MASK)){
        lfPrintf("recieved message toggle signal: switching event recipient");
        this->toggleEventRecipient();
		return true;
	  }else if(event->data->key_data->which == VMS_F11){
	    this->toggleFullScreen();
		return true;
	  }
	}
	return false;
  }
};


