#include <time.h>




class LoopClock
{
  //keep track of average loop time in milliseconds.
  static const int numTimes = 5;
  long loop_times[numTimes];
  struct timespec frame_begin_time;
  struct timespec loop_begin_time;
  struct timespec end_time;

  //keeps track of which loop time is the oldest and should be replaced next
  unsigned short loop_time_index;

  //keeps track of number of frames processed since the last framerate update
  unsigned int frames;
  
  float last_loop_FPS;

  //returns the elapsed time from t2 to t1 in seconds
  float SecDiff(struct timespec t1, struct timespec t2);

  //returns the elapsed time from t2 to t1 in mseconds
  int MSecDiff(struct timespec t1, struct timespec t2);

  
public:
  LoopClock(void);

  //records the start of the looping time
  void LoopStart(void);

  //calculates total elapsed looping time for the most recent frame
  //returns true if a new framerate value is available
  bool LoopEnd(void);

  //returns the framerate
  float GetFR(void);

  int EstimateSleepTime(float max_framerate);
};
