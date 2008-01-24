#include <prefix/dtcoreprefix-src.h>
#include <dtCore/timer.h>

#ifdef DELTA_WIN32
   #include <Windows.h>
   void dtCore::AppSleep(unsigned int milliseconds){Sleep(milliseconds);}
#else
   #include <unistd.h>
   void dtCore::AppSleep(unsigned int milliseconds){usleep((milliseconds) * 1000);}
#endif
namespace dtCore
{

Timer::Timer()
{

}

Timer::~Timer()
{

}


const Timer* Timer::Instance()
{
   static Timer sTimer;
   return &sTimer;
}

Timer_t Timer::Tick() const
{
   return mTimer.tick();
}

double Timer::DeltaSec( Timer_t t1, Timer_t t2 ) const
{
   return mTimer.delta_s(t1, t2);
}

double Timer::DeltaMil( Timer_t t1, Timer_t t2 ) const
{
   return mTimer.delta_m(t1, t2);
}

double Timer::DeltaMicro( Timer_t t1, Timer_t t2 ) const
{
   return mTimer.delta_u(t1, t2);
}

double Timer::DeltaNano( Timer_t t1, Timer_t t2 ) const
{
   return mTimer.delta_n(t1, t2);
}

double Timer::GetSecondsPerTick() const
{
   return mTimer.getSecondsPerTick();
}

}//namespace dtCore

