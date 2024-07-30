//
// Created by dracad on 14/07/24.
//

#ifndef TIMER_H
#define TIMER_H

#include <transport-cpp/device.h>
#include <chrono>
#include <functional>

namespace Context {

class TRANSPORT_CPP_EXPORT Timer :
        public Device
{
private:
    bool                        mIsRunning;
    std::chrono::milliseconds   mSetDuration;
    std::function<void(void)>   mCallback;

public:
    Timer();
    ~Timer() override;

    RETURN_CODE stop() noexcept;
    RETURN_CODE resume() noexcept;
    [[nodiscard]] RETURN_CODE start(const std::chrono::milliseconds &duration) noexcept;

    void setCallback(std::function<void(void)> callback);

    [[nodiscard]] bool isRunning() const noexcept;

private:
    void readyRead() override;
    void readyError() override;
};

} // Context

#endif //TIMER_H
