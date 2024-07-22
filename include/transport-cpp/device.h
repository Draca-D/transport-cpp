#ifndef DEVICE_H
#define DEVICE_H

#include <memory>

#include "transport-cpp.h"

#include <optional>
#include <stdio.h>
#include <vector>
#include <variant>

class pollfd;

namespace Context {
class Engine;

class TRANSPORT_CPP_EXPORT Device
{
    friend class Engine;

    using POLL_STRUCT   = pollfd;
    using DEVICE_HANDLE = std::optional<DEVICE_HANDLE_>;
    using ENGINE_PTR    = Engine*;
    using POLL_PTR      = std::vector<POLL_STRUCT>::iterator;
    using ERROR_STRING  = std::string;
    using LOGGER        = std::shared_ptr<Transport::Logger>;

public:
    enum class ERROR_CODE{
        NO_ERROR,
        INVALID_ARGUMENT,
        INVALID_LOGIC,
        DEVICE_NOT_READY,
        POLL_ERROR,
        TIMEOUT,
        GENERAL_ERROR
    };

    using DEVICE_ERROR  = std::variant<ERROR_CODE, SYS_ERR_CODE>;

    struct ERROR{
        DEVICE_ERROR    code;
        ERROR_STRING    description;
    };

    template<typename TP>
    struct Result
    {
        RETURN_CODE code;
        std::optional<TP> result;
    };

private:
    DEVICE_HANDLE   mDeviceHandle;
    ENGINE_PTR      mLoadedEngine   = nullptr;
    ERROR           mLastError      = {ERROR_CODE::NO_ERROR, ""};
    LOGGER          mLogger         = Transport::Logger::DefaultLogger;

private:
    void loadEngine(ENGINE_PTR engine);
    void deloadEngine();

public:
    virtual ~Device();

    [[nodiscard]] ENGINE_PTR getCurrentLoadedEngine() const noexcept;
    [[nodiscard]] DEVICE_HANDLE getDeviceHandle() const noexcept;
    [[nodiscard]] ERROR getLastError() const noexcept;

    void setLogger(const LOGGER &logger) noexcept;

protected:
    Device();

    virtual void registerNewHandle(DEVICE_HANDLE handle);
    void setError(DEVICE_ERROR code, const ERROR_STRING &description);

    void requestRead();
    void requestWrite();
    void destroyHandle();
    void closeHandle();

    void registerChildDevice(Device *device);

    void logDebug(const std::string &calling_class, const std::string &message) const;
    void logInfo(const std::string &calling_class, const std::string &message) const;
    void logWarn(const std::string &calling_class, const std::string &message) const;
    void logError(const std::string &calling_class, const std::string &message) const;
    void logFatal(const std::string &calling_class, const std::string &message) const;

    //Device Ready
    virtual void readyRead();
    virtual void readyWrite();
    virtual void readyError();
    virtual void readyHangup();
    virtual void readyInvalidRequest();
    virtual void readyPeerDisconnect();
};
}

#endif // DEVICE_H
