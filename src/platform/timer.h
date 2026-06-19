#pragma once

namespace timer {

class TimerResolution {
public:
    TimerResolution();
    ~TimerResolution();
    TimerResolution(const TimerResolution&) = delete;
    TimerResolution& operator=(const TimerResolution&) = delete;
};

void sleep_for(float seconds);

} // namespace timer
