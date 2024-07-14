#include <transport-cpp/device.h>
#include <transport-cpp/engine.h>

#include <poll.h>

namespace Context{
Device::~Device()
{
    logDebug("Device/Destructor", "Destroying device");

    deloadEngine();
}

Context::Device::ENGINE_PTR Context::Device::getCurrentLoadedEngine() const noexcept
{
    return mLoadedEngine;
}

void Context::Device::loadEngine(ENGINE_PTR engine)
{
    logDebug("Device", "Loading new engine");

    mLoadedEngine = engine;
}

void Context::Device::deloadEngine()
{
    logDebug("Device/deloadEngine", "Deloading Engine");

    auto currentEngine = mLoadedEngine;
    mLoadedEngine = nullptr;

    if(currentEngine != nullptr){
        logDebug("Device/deloadEngine", "Engine is valid, calling deregister device");
        currentEngine->deRegisterDevice(this);
    }

}

void Context::Device::loadPollStructure(POLL_PTR poll)
{
    logDebug("Device", "Loading new poll structure");

    mPollPtrValid   = true;
    mLoadedPoll     = poll;
}

void Context::Device::deloadPollStructure()
{
    logDebug("Device", "Deloading poll structure");
    mPollPtrValid = false;
}

void Device::readyRead()
{
    logDebug("Device", "Device is ready to perform a read, but functionality has not been implemented by child");
}

void Device::readyWrite()
{
    logDebug("Device", "Device is ready to perform a write, but functionality has not been implemented by child");
}

void Device::readyError()
{
    logError("Device", "Device has an error, but functionality has not been implemented by child");
}

Device::DEVICE_HANDLE Device::getDeviceHandle() const noexcept
{
    return mDeviceHandle;
}

Device::Device()
{

}

void Device::registerNewHandle(DEVICE_HANDLE handle)
{
    logDebug("Device", "Registering new handle");

    if(mLoadedEngine != nullptr){
        mLoadedEngine->registerNewHandle(mDeviceHandle, handle, this);
    }

    mDeviceHandle = handle;
}

void Device::logDebug(const std::string &calling_class, const std::string &message)
{
    printf("[DEBUG][%s]: %s\n", calling_class.c_str(), message.c_str());
}

void Device::logInfo(const std::string &calling_class, const std::string &message)
{
    printf("[INFO][%s]: %s\n", calling_class.c_str(), message.c_str());
}

void Device::logWarn(const std::string &calling_class, const std::string &message)
{
    printf("[WARN][%s]: %s\n", calling_class.c_str(), message.c_str());
}

void Device::logError(const std::string &calling_class, const std::string &message)
{
    printf("[ERROR][%s]: %s\n", calling_class.c_str(), message.c_str());
}

void Device::logFatal(const std::string &calling_class, const std::string &message)
{
    printf("[FATAL][%s]: %s\n", calling_class.c_str(), message.c_str());
}
}
