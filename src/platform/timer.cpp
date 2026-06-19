#include "platform/timer.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <thread>
#endif

namespace timer {

TimerResolution::TimerResolution() {
#ifdef _WIN32
    timeBeginPeriod(1);
#endif
}

TimerResolution::~TimerResolution() {
#ifdef _WIN32
    timeEndPeriod(1);
#endif
}

void sleep_for(float seconds) {
#ifdef _WIN32
    if (seconds > 0.002f) {
        Sleep(static_cast<DWORD>(seconds * 1000.0f - 1.0f));
    }
#else
    std::this_thread::sleep_for(
        std::chrono::duration<float>(seconds));
#endif
}

} // namespace timer
