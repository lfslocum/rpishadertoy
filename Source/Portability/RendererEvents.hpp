/*
 * RendererEvents.hpp
 *
 *  Created on: Nov 23, 2011
 *      Author: jslocum
 *
 *  Cross-platform specification for events passed by the renderer; may be
 *  keyboard, mouse, or software related.
 */

#ifndef RENDEREREVENTS_HPP_
#define RENDEREREVENTS_HPP_

#include <Include/VMS_Defines.h>   // has uint32, float32, float64
#include <boost/shared_ptr.hpp>    // boost used for SceneObjectWrapperPtr
#include <Services/VmsCriticalSections/VmsSynchronizedQueue.hpp>
#include <Portability/Instrumentation/Instrumentation.h>
#include <Portability/PublicInterfaces/PortTstring.h>
#include <Services/VmsTextures/VmsTexture.h>
#include <Synthesizer/SceneObject.h>
#include <utility>



#define SHIFT_MASK 0x01
#define ALT_MASK 0x02
#define CTRL_MASK 0x04
#define ESCAPE_MASK 0x08
#define META_MASK 0x010


typedef enum {
  MOUSE_DOWN,   //when the mouse is pressed in the window
  MOUSE_UP,     //when the mouse is released in the window
  MOUSE_SCROLL, //the scroll wheel is moved
  MOUSE_MOVE,   //when the mouse is moved in the window
  MOUSE_DBLCLK, //double click
  KEY_UP,       //when a key is pressed or released
  KEY_DOWN,
  SOFTWARE     //if the renderer wants to send a non-input related message to
               //system control, or someone wants to MSG the renderer
} eventtype;


typedef enum {
  MOUSE_OTHER,
  MOUSE_LEFT,
  MOUSE_RIGHT,
  MOUSE_MIDDLE,
  MOUSE_WHEEL
} button;

typedef char key;


//x,y coordinates, relative to top-left of window
typedef struct MouseMove
{
  uint32 window_x;
  uint32 window_y;
} MouseMoveData;


typedef struct MousePress
{
  bool down; //true when pressed; false when released
  button which;
  MouseMoveData where;
} MousePressData;

typedef struct MouseDouble
{
  button which;
  MouseMoveData where;
} MouseDoubleData;

typedef struct MouseWheel
{
  int32 wheel_delta;
  MouseMoveData where;
} MouseWheelData;

typedef struct Keystroke
{
  bool down; //true means the key was pressed; false means it was released
  key which;
} KeystrokeData;


enum renderer_message {
  LOG_FRAMES,
  RENDERER_HANDLES_EVENTS,
  RENDERER_SCREENCAPTURE,
  REGISTER_TEXTURE,
  RENDERER_STOP,
  REGISTER_SCENEOBJECT,
  DEREGISTER_SCENEOBJECT,
  SCENEOBJECT_REQUEST,
  SCENEOBJECT_RETURN
};

struct SoftwareMessageData
{
  //I don't like this pointer implementation for contents. The user has to be
  //wary of making sure they don't need whatever is stored in contents, as it
  //will be deleted when the struct is.
  ~SoftwareMessageData()
  {
    switch(msg_type)
    {
	case LOG_FRAMES:
	  vms_delete (uint32*)contents;
	  break;
	case RENDERER_HANDLES_EVENTS:
		vms_delete (bool*)contents;
		break;
	case RENDERER_SCREENCAPTURE:
		vms_delete (tstring*)contents;
		break;
	case REGISTER_TEXTURE:
		vms_delete (RegisterTextureInfo*) contents;
		break;
	case RENDERER_STOP:
		vms_delete (char*) contents;
		break;
	case REGISTER_SCENEOBJECT:
		break;
	case DEREGISTER_SCENEOBJECT:
		vms_delete (int*) contents;
		break;
	case SCENEOBJECT_REQUEST:
		vms_delete (std::pair<int,int>*) contents;
		break;
	case SCENEOBJECT_RETURN:
		break;
    }
  }

  renderer_message msg_type;
  void* contents;
};

typedef struct EventDataWrapper {
  EventDataWrapper(): press_data(0), move_data(0), key_data(0), wheel_data(0),
                      double_data(0), message_data(0) {;}
  ~EventDataWrapper()
  {
    vms_delete press_data;
    vms_delete move_data;
    vms_delete key_data;
    vms_delete double_data;
    vms_delete wheel_data;
    vms_delete message_data;
  }
  MousePressData* press_data;
  MouseMoveData* move_data;
  KeystrokeData* key_data;
  MouseWheelData* wheel_data;
  MouseDoubleData* double_data;
  SoftwareMessageData* message_data;
} RendererDataWrapper;



class RendererEvent
{
public:
  //platform specific constructor that takes a windows/x11/osx event and
  //converts it to a RendererEvent
  RendererEvent(void* event_ptr, eventtype type);
  RendererEvent(SoftwareMessageData* msg)
  {
    hfPrintf("Creating software message event of type %u", msg->msg_type);
    this->data = new EventDataWrapper();
    this->type = SOFTWARE;
    this->mask = RendererEvent::current_mask;
    this->data->message_data = msg;
  };
  
  ~RendererEvent() {vms_delete data;}
  eventtype type; //mouse? keyboard? other?
  char mask; //modifiers, such as ctrl key held down, or caps lock on
  EventDataWrapper* data; //for mice, which button was held down? For
                              //keyboards, which key?
private:
  void updateMask(char new_mask);
  static char current_mask; //the current key mask, copied to mask in the
                            //constructor and modified when new modifier key
                            //events are received.
};




typedef boost::shared_ptr<RendererEvent> RendererEventPtr;

class RendererEventHandler
{
public:
  virtual void enqueueEvent(RendererEventPtr event) = 0;

protected:
  virtual RendererEventPtr popEvent() = 0;
  //queue of RendererEvent's
  SlowSynchronizedQueue<RendererEventPtr> event_queue;
  
private:
};

typedef boost::shared_ptr<RendererEventHandler> RendererEventHandlerPtr;

#endif /*RENDEREREVENTS_HPP_*/
