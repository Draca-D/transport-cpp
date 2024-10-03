//
// Created by dracad on 14/07/24.
//

#include <stdexcept>
#include <transport-cpp/timer.h>

#include <sys/timerfd.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cmath>

timespec nsToTimespec(std::chrono::nanoseconds ns)
{
    static constexpr double secToNsec = 1000000000.0;

    timespec ts;

    auto nsDbl = static_cast<double>(ns.count());
    auto fractionalSec = nsDbl/secToNsec;

    double seconds;

    ts.tv_nsec = static_cast<decltype(ts.tv_nsec)>(std::modf(fractionalSec, &seconds) * secToNsec);
    ts.tv_sec = static_cast<decltype(ts.tv_sec)>(seconds);

    return ts;
}

namespace Context {

Timer::Timer() : Device(),
    mIsRunning(false), mSetDuration(std::chrono::milliseconds::max())
{
    auto tmrfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

    if(tmrfd < 0){
        throw std::runtime_error(std::string("Unable to create timer err: ")
                                 + strerror(errno));
    }

    registerNewHandle(tmrfd);
}

Timer::~Timer()
{
    logDebug("Timer", "Destructing timer");

    auto tmrfd = getDeviceHandle().value();
    close(tmrfd);
}

RETURN_CODE Context::Timer::stop() noexcept
{
    if(!mIsRunning){
        return RETURN::PASSABLE; //its fine to run the rest of the code even
        // if this is already stopped, just feels cleaner this way
    }

    mIsRunning = false;
    auto tmrfd = getDeviceHandle().value();

    itimerspec tmr;

    //Theres an accuracy of about 1000ns on the timers.

    //When, in the future, will the first trigger happen (must be non zero)
    tmr.it_value.tv_nsec    = 0;
    tmr.it_value.tv_sec     = 0;

    //When, after the first trigger, will the fd continue to trigger. if both values are 0, after the initial trigger
    // the fd will no longer trigger when polled, it will be a dead fd
    tmr.it_interval.tv_sec  = 0;
    tmr.it_interval.tv_nsec = 0; // limit appears to be 3000ns, that could be a limitation,
    // of my setup here, but poll wont return any faster than 3000ns, lower values
    // will mean a non '1' value in the 'buf' below, indicating that multiple
    // intervals have occured since poll returned.

    timerfd_settime(tmrfd, 0, &tmr, nullptr);
    return RETURN::OK;
}

RETURN_CODE Timer::resume() noexcept
{
    if(mSetDuration == decltype (mSetDuration)::max()){
        setError(ERROR_CODE::INVALID_LOGIC, "Cannot resume timer, it hasnt been started");
        return RETURN::NOK;
    }

    return start(mSetDuration);
}

RETURN_CODE Timer::start(const std::chrono::milliseconds &duration) noexcept
{
    if(isRunning()){
        stop();
    }

    mSetDuration    = duration;
    auto tmrfd      = getDeviceHandle().value();

    itimerspec tmr;

    //Theres an accuracy of about 1000ns on the timers.

    //When, in the future, will the first trigger happen (must be non zero)
    //    tmr.it_value.tv_nsec = 10000000000;
    //    tmr.it_value.tv_sec = 0;

    //When, after the first trigger, will the fd continue to trigger. if both values are 0, after the initial trigger
    // the fd will no longer trigger when polled, it will be a dead fd
    //    tmr.it_interval.tv_sec = 0;
    //    tmr.it_interval.tv_nsec = 10000000000; // limit appears to be 3000ns, that could be a limitation,
    // of my setup here, but poll wont return any faster than 3000ns, lower values
    // will mean a non '1' value in the 'buf' below, indicating that multiple
    // intervals have occured since poll returned.

    auto ts = nsToTimespec(duration);

    tmr.it_value    = ts;
    tmr.it_interval = ts;

    //tv_nsec cannot exceed 1s

    auto response   = timerfd_settime(tmrfd, 0, &tmr, nullptr);

    if(response < 0){
        setError(errno, "Unable to start timer");
        return RETURN::NOK;
    }

    mIsRunning = true;

    return RETURN::OK;
}

void Timer::setCallback(std::function<void (void)> callback)
{
    mCallback = callback;
}

bool Timer::isRunning() const noexcept
{
    return mIsRunning;
}

void Timer::readyRead()
{
    static thread_local char buffer[8];

    read(getDeviceHandle().value(), buffer, sizeof(buffer)); //timer needs to be read otherwise
    //time will not be reset and will always be 'valid' causing poll to return

    if(mCallback){
        mCallback();
    }
}

void Timer::readyError()
{
    logError("Timer", "Error occured with the file descriptor, error is unknown");
}

} // Context
