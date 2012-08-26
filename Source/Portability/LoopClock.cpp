#include "LoopClock.hpp"


using namespace std;

LoopClock::LoopClock(void)
{
  for(int idx = 0; idx < numTimes; idx++)
    //assume ~24fps to start out
    loop_times[idx] = 41666666;
  loop_time_index = 0;
  frames = 0;
  clock_gettime(CLOCK_REALTIME, &frame_begin_time);
}


//records the start of the rendering time
void LoopClock::LoopStart(void)
{
  clock_gettime(CLOCK_REALTIME, &loop_begin_time);
}

//calculates total elapsed rendering time for the most recent frame
//returns true if a new framerate value is available
bool LoopClock::LoopEnd(void)
{
  ++frames;
  clock_gettime(CLOCK_REALTIME, &end_time);

  
  //update loop time
  loop_times[loop_time_index] = MSecDiff(end_time, loop_begin_time);
  loop_time_index = (loop_time_index + 1) % 5;

  
  //check FR timer
  float sd;
  if((sd = SecDiff(frame_begin_time, end_time)) >= 1.0)
    {
      this->last_loop_FPS = ((float) frames) / sd;
      frames = 0;
      clock_gettime(CLOCK_REALTIME, &frame_begin_time);
      return true;
    }
  else
    return false;
}

//returns the framerate
float LoopClock::GetFR(void)
{
  return this->last_loop_FPS;
}


float LoopClock::SecDiff(struct timespec start, struct timespec end)
{

  float secs = 0.0;
  if ((end.tv_nsec-start.tv_nsec)<0) {
    secs = end.tv_sec-start.tv_sec-1.0;
    secs += (1000000000+end.tv_nsec-start.tv_nsec) / 1000000000.0;
  } else {
    secs = 1.0 * (end.tv_sec-start.tv_sec);
    secs += (end.tv_nsec-start.tv_nsec)/1000000000.0;
  }
  return secs;
}

int LoopClock::MSecDiff(struct timespec start, struct timespec end)
{

  int msecs = 0.0;
  if ((end.tv_nsec-start.tv_nsec)<0) {
    msecs = (end.tv_sec-start.tv_sec)*1000;
    msecs += (1000000+end.tv_nsec-start.tv_nsec) / 1000000;
  } else {
    msecs = 1000 * (end.tv_sec-start.tv_sec);
    msecs += (end.tv_nsec-start.tv_nsec)/1000000;
  }
  return msecs;
}



int LoopClock::EstimateSleepTime(float max_framerate)
{
  int min_frame_time_ms = (int) (1000.0 / max_framerate);
  int avg_frame_time_ms = 0;
  for(int idx = 0; idx < numTimes; idx++)
    avg_frame_time_ms +=loop_times[idx];
  
  avg_frame_time_ms /= 5;

  int sleeper_time = min_frame_time_ms - avg_frame_time_ms;

  return (sleeper_time >= 0) ? sleeper_time : 0;
}
