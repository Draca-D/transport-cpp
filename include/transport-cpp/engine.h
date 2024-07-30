#ifndef ENGINE
#define ENGINE

#include "transport-cpp.h"

#include <memory>
#include <vector>
#include <optional>
#include <map>
#include <poll.h>
#include <chrono>

namespace Context
{
class Device;

class TRANSPORT_CPP_EXPORT Engine
{
    friend class Device;

    using POLL_STRUCT   = pollfd;
    using POLL_LIST     = std::vector<POLL_STRUCT>;
    using DEVICE_HANDLE = std::optional<DEVICE_HANDLE_>;
    using POLL_MAP      = std::map<DEVICE_HANDLE_, Device*>;
    using ERROR_STRING  = std::string;
    using DEVICE_LIST   = std::vector<Device*>;
    using LOGGER        = std::shared_ptr<Transport::Logger>;

public:
    enum class ERROR_CODE{
        NO_ERROR,
        DEVICE_ALREADY_REGISTERED,
        DEVICE_DOES_NOT_EXIST,
        INVALID_ARGUMENT
    };

    struct ERROR{
        ERROR_CODE      code;
        ERROR_STRING    description;
    };

private:
    ERROR       mLastError = {ERROR_CODE::NO_ERROR, ""};
    POLL_LIST   mPollDevices;
    POLL_MAP    mDeviceMapping;
    DEVICE_LIST mDeviceList;
    LOGGER      mLogger = Transport::Logger::DefaultLogger;

public:
    ~Engine();
    Engine();

    [[nodiscard]] RETURN_CODE registerDevice(std::shared_ptr<Context::Device> device) noexcept;
    [[nodiscard]] RETURN_CODE registerDevice(Context::Device &device) noexcept;

    [[nodiscard]] RETURN_CODE deRegisterDevice(std::shared_ptr<Context::Device> device) noexcept;
    [[nodiscard]] RETURN_CODE deRegisterDevice(Context::Device &device) noexcept;

    void setLogger(const LOGGER &logger) noexcept;

    [[nodiscard]] ERROR getLastError() const noexcept;

    //Awaiters
    void awaitOnce(const std::optional<std::chrono::milliseconds> &optional_duration = {});
    void awaitFor(const std::chrono::milliseconds &duration);
    [[noreturn]] void awaitForever();

private:
    RETURN_CODE registerDevice(Context::Device *device);

    //slots
    RETURN_CODE deRegisterDevice(Context::Device *device);

    RETURN_CODE registerNewHandle(DEVICE_HANDLE old_handle,
                                  DEVICE_HANDLE new_handle,
                                  Device *relevant_device);

    RETURN_CODE deRegisterHandle(DEVICE_HANDLE handle);

    POLL_LIST::iterator findHandleInList(DEVICE_HANDLE handle);

    void setError(ERROR_CODE code, const ERROR_STRING &description);
    bool awaitOnceUpto(int ms);

    //loggers
    void logDebug(const std::string &calling_class, const std::string &message) const;
    void logInfo(const std::string &calling_class, const std::string &message) const;
    void logWarn(const std::string &calling_class, const std::string &message) const;
    void logError(const std::string &calling_class, const std::string &message) const;
    void logFatal(const std::string &calling_class, const std::string &message) const;

    void requestRead(const DEVICE_HANDLE &handle);
    void requestWrite(const DEVICE_HANDLE &handle);
};
}

#endif /* ENGINE */
