#ifndef DEVICE_H
#define DEVICE_H

#include "transport-cpp.h"

#include <optional>
#include <stdio.h>
#include <vector>

class pollfd;

namespace Context {
class Engine;

class TRANSPORT_CPP_EXPORT Device
{
    friend class Engine;

    using POLL_STRUCT   = pollfd;
    using DEVICE_HANDLE = std::optional<DEVICE_HANDLE_>;
    using ENGINE_PTR    = std::add_pointer<Engine>::type;
    using POLL_PTR      = std::vector<POLL_STRUCT>::iterator;

private:
    DEVICE_HANDLE   mDeviceHandle;
    ENGINE_PTR      mLoadedEngine   = nullptr;
    POLL_PTR        mLoadedPoll;
    bool            mPollPtrValid   = false;

private:
    void loadEngine(ENGINE_PTR engine);
    void deloadEngine();

    void loadPollStructure(POLL_PTR poll);
    void deloadPollStructure();

    //Device Ready

    virtual void readyRead();
    virtual void readyWrite();
    virtual void readyError();

public:
    virtual ~Device();

    ENGINE_PTR getCurrentLoadedEngine() const noexcept;
    DEVICE_HANDLE getDeviceHandle() const noexcept;

protected:
    Device();

    void registerNewHandle(DEVICE_HANDLE handle);

    void logDebug(const std::string &calling_class, const std::string &message);
    void logInfo(const std::string &calling_class, const std::string &message);
    void logWarn(const std::string &calling_class, const std::string &message);
    void logError(const std::string &calling_class, const std::string &message);
    void logFatal(const std::string &calling_class, const std::string &message);
};
}

#endif // DEVICE_H
