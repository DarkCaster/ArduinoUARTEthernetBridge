#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <cstdint>

class Timer
{
    private:
        std::chrono::microseconds interval;
        std::chrono::time_point<std::chrono::steady_clock> startTime;
        std::chrono::microseconds lastDiff;
    public:
        Timer();
        void SetInterval(int64_t intervalUsec);
        std::chrono::microseconds TimeLeft();
        bool Update();
        void Reset(bool freshStart);
};

#endif // TIMER_H
