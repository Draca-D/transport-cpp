#include <transport-cpp/device.h>
#include <transport-cpp/engine.h>

#include <poll.h>
#include <unistd.h>

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

void Device::readyHangup()
{
    logWarn("Device", "Device peer has hung up, but functionality has not been implemented by child");
}

void Device::readyInvalidRequest()
{
    logWarn("Device", "Invalid request, but functionality has not been implemented by child");
}

void Device::readyPeerDisconnect()
{
    logWarn("Device", "Peer has disconnected, but functionality has not been implemented by child");
}

Device::DEVICE_HANDLE Device::getDeviceHandle() const noexcept
{
    return mDeviceHandle;
}

Device::ERROR Device::getLastError() const noexcept
{
    return mLastError;
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

void Device::setError(DEVICE_ERROR code, const ERROR_STRING &description)
{
    logDebug("Device", "New error added with description: " + description);

    mLastError.code         = code;
    mLastError.description  = description;
}

void Device::requestRead()
{
    logDebug("Device", "Request Read");
    if(mLoadedEngine){
        mLoadedEngine->requestRead(mDeviceHandle);
    }
}

void Device::requestWrite()
{
    logDebug("Device", "Request Write");
    if(mLoadedEngine){
        mLoadedEngine->requestWrite(mDeviceHandle);
    }
}

void Device::destroyHandle()
{
    closeHandle();

    if(mLoadedEngine){
        mLoadedEngine->deRegisterHandle(mDeviceHandle);
        mDeviceHandle = {};
    }
}

void Device::closeHandle()
{
    if(mDeviceHandle){
        close(mDeviceHandle.value());
    }
}

void Device::registerChildDevice(Device *device)
{
    if(mLoadedEngine){
        mLoadedEngine->registerDevice(device);
    }
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
