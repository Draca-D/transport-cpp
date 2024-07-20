#include <transport-cpp/engine.h>

#include <transport-cpp/device.h>

#include <algorithm>

namespace Context
{
Engine::~Engine()
{
    for(auto &device:mDeviceList){
        deRegisterDevice(device);
    }
}

Engine::Engine()= default;

RETURN_CODE Context::Engine::registerDevice(std::shared_ptr<Device> device) noexcept
{
    if(!device){
        setError(ERROR_CODE::INVALID_ARGUMENT, "When registering a new Device, an invalid sharedptr was provided");
        return RETURN::NOK;
    }

    return registerDevice(device.get());
}

RETURN_CODE Engine::registerDevice(Device &device) noexcept
{
    return registerDevice(&device);
}

RETURN_CODE Engine::deRegisterDevice(std::shared_ptr<Device> device) noexcept
{
    if(!device){
        setError(ERROR_CODE::INVALID_ARGUMENT, "When deregistering a Device, an invalid sharedptr was provided");
        return RETURN::NOK;
    }

    return deRegisterDevice(device.get());
}

RETURN_CODE Engine::deRegisterDevice(Device &device) noexcept
{
    return deRegisterDevice(&device);
}

Engine::ERROR Engine::getLastError() const noexcept
{
    return mLastError;
}

void Engine::awaitOnce(const std::optional<std::chrono::milliseconds> &optional_duration)
{
    int timeout = -1;

    if(optional_duration){
        auto duration = optional_duration.value();

        if(duration.count() > std::numeric_limits<decltype(timeout)>::max()){
            logWarn("Engine/awaitOnce", "Provided timeout exceeds the system max duration of " +
                    std::to_string(std::numeric_limits<decltype(timeout)>::max()) + " milliseconds. Clamping to max");

            timeout = std::numeric_limits<decltype(timeout)>::max();
        }else{
            timeout = static_cast<decltype (timeout)>(duration.count());
        }
    }

    awaitOnceUpto(timeout);
}

void Engine::awaitFor(const std::chrono::milliseconds &duration)
{
    auto start = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::steady_clock::now() - start;

    while(elapsed < duration){
        auto remaining = duration - elapsed;
        auto remaining_ms = std::chrono::duration_cast<std::chrono::milliseconds>(remaining).count();

        if(remaining_ms > std::numeric_limits<int>::max()){
            remaining_ms = std::numeric_limits<int>::max();
        }

        awaitOnceUpto(static_cast<int>(remaining_ms));

        elapsed = std::chrono::steady_clock::now() - start;
    }
}

[[noreturn]] void Engine::awaitForever (){
    while(true){
        awaitOnceUpto(std::chrono::milliseconds(100).count());
    }
}

bool Engine::awaitOnceUpto(int ms)
{
    auto res = poll(mPollDevices.data(), mPollDevices.size(), ms);
    int count = 0;

    if(res <= 0){
        return false;
    }



    static thread_local std::vector<DEVICE_HANDLE_> ready_read(256);
    static thread_local std::vector<DEVICE_HANDLE_> ready_write(256);
    static thread_local std::vector<DEVICE_HANDLE_> error(256);
    static thread_local std::vector<DEVICE_HANDLE_> hangup(256);
    static thread_local std::vector<DEVICE_HANDLE_> invalid(256);
    static thread_local std::vector<DEVICE_HANDLE_> peer_disconnect(256);

    ready_read.clear();
    ready_write.clear();
    error.clear();
    hangup.clear();
    invalid.clear();
    peer_disconnect.clear();

//    ready_read.reserve(static_cast<size_t>(res));
//    ready_write.reserve(static_cast<size_t>(res));
//    error.reserve(static_cast<size_t>(res));



    for(const auto &fd:mPollDevices){
        if(fd.revents == POLLIN){
            ready_read.push_back(fd.fd);
            count++;
        }else if(fd.revents == POLLOUT){
            ready_write.push_back(fd.fd);
            count++;
        }else if(fd.revents & POLLERR){
            error.push_back(fd.fd);
            count++;
        }else if(fd.revents & POLLHUP){
            hangup.push_back(fd.fd);
            count++;
        }else if(fd.revents & POLLNVAL){
            invalid.push_back(fd.fd);
            count++;
        }else if(fd.revents & POLLRDHUP){
            peer_disconnect.push_back(fd.fd);
            count++;
        }

        if(count >= res){
            break;
        }
    }

    for(const auto &poll_in:ready_read){
        if(mDeviceMapping.find(poll_in) != mDeviceMapping.end()){
            mDeviceMapping[poll_in]->readyRead();
        }
    }

    for(const auto &poll_out:ready_write){
        if(mDeviceMapping.find(poll_out) != mDeviceMapping.end()){
            mDeviceMapping[poll_out]->readyWrite();
        }
    }

    for(const auto &poll_err:error){
        if(mDeviceMapping.find(poll_err) != mDeviceMapping.end()){
            mDeviceMapping[poll_err]->readyError();
        }
    }

    for(const auto &hangup:hangup){
        if(mDeviceMapping.find(hangup) != mDeviceMapping.end()){
            mDeviceMapping[hangup]->readyHangup();
        }
    }

    for(const auto &inval:invalid){
        if(mDeviceMapping.find(inval) != mDeviceMapping.end()){
            mDeviceMapping[inval]->readyInvalidRequest();
        }
    }

    for(const auto &disc:peer_disconnect){
        if(mDeviceMapping.find(disc) != mDeviceMapping.end()){
            mDeviceMapping[disc]->readyPeerDisconnect();
        }
    }

    return true;
}

RETURN_CODE Engine::registerDevice(Device *device)
{
    logDebug("Engine", "Registering device");

    auto device_item = std::find(mDeviceList.begin(), mDeviceList.end(), device);

    if(device_item != mDeviceList.end()){
        setError(ERROR_CODE::DEVICE_ALREADY_REGISTERED, "");
        return RETURN::PASSABLE;
    }

    if(device->getCurrentLoadedEngine() != nullptr){
        device->deloadEngine();
    }

    device->loadEngine(this);

    mDeviceList.push_back(device);

    auto handle = device->getDeviceHandle();

    return registerNewHandle({}, handle, device);
}

RETURN_CODE Engine::deRegisterDevice(Device *device)
{
    logDebug("Engine", "Deregistering device");

    auto device_engine = device->getCurrentLoadedEngine();

    if(device_engine != nullptr){
        device->deloadEngine();
    }

    deRegisterHandle(device->getDeviceHandle());

    auto device_item = std::find(mDeviceList.begin(), mDeviceList.end(), device);

    if(device_item != mDeviceList.end()){
        mDeviceList.erase(device_item);
    }

    return RETURN::OK;
}

RETURN_CODE Engine::registerNewHandle(
        DEVICE_HANDLE old_handle,
        DEVICE_HANDLE new_handle,
        Device *relevant_device)
{
    logDebug("Engine", "Registering new handle");

    if(!new_handle){
        return RETURN::PASSABLE;
    }

    auto handle_pos = findHandleInList(old_handle);

    if(handle_pos == mPollDevices.end()){
        pollfd fd;
        fd.fd       = new_handle.value();
        fd.events   = POLLIN;

        mPollDevices.push_back(fd);
        mDeviceMapping.insert({fd.fd, relevant_device});

        return RETURN::OK;
    }

    if(old_handle){
        mDeviceMapping.erase(old_handle.value());
    }

    mDeviceMapping.insert({new_handle.value(), relevant_device});
    (*handle_pos).fd = new_handle.value();

    return RETURN::OK;
}

RETURN_CODE Engine::deRegisterHandle(DEVICE_HANDLE handle)
{
    logDebug("Engine", "Deregistering handle");

    if(!handle.has_value()){
        return RETURN::PASSABLE;
    }

    auto handle_pos = findHandleInList(handle);

    if(handle_pos == mPollDevices.end()){
        setError(ERROR_CODE::DEVICE_DOES_NOT_EXIST, "When deregistering the handle, "
                                                    "the device handle was not found in the registered list");
        return RETURN::NOK;
    }

    mPollDevices.erase(handle_pos);
    mDeviceMapping.erase(handle.value());
    return RETURN::OK;
}

Engine::POLL_LIST::iterator Engine::findHandleInList(
        DEVICE_HANDLE handle)
{
    if(!handle.has_value()){
        return mPollDevices.end();
    }

    std::vector<int> x;

    return std::find_if(mPollDevices.begin(), mPollDevices.end(),
                        [&handle](const decltype (mPollDevices)::value_type &fd){return fd.fd == handle.value();});
}

void Engine::setError(ERROR_CODE code, const ERROR_STRING &description)
{
    logDebug("Engine", "New error added with description: " + description);
    mLastError.code         = code;
    mLastError.description  = description;
}

void Engine::logDebug(const std::string &calling_class, const std::string &message)
{
    printf("[DEBUG][%s]: %s\n", calling_class.c_str(), message.c_str());
}

void Engine::logInfo(const std::string &calling_class, const std::string &message)
{
    printf("[INFO][%s]: %s\n", calling_class.c_str(), message.c_str());
}

void Engine::logWarn(const std::string &calling_class, const std::string &message)
{
    printf("[WARN][%s]: %s\n", calling_class.c_str(), message.c_str());
}

void Engine::logError(const std::string &calling_class, const std::string &message)
{
    printf("[ERROR][%s]: %s\n", calling_class.c_str(), message.c_str());
}

void Engine::logFatal(const std::string &calling_class, const std::string &message)
{
    printf("[FATAL][%s]: %s\n", calling_class.c_str(), message.c_str());
}

void Engine::requestRead(const DEVICE_HANDLE &handle)
{
    auto pollfd_it = findHandleInList(handle);

    if(pollfd_it == mPollDevices.end()){
        return;
    }

    pollfd_it->events = POLLIN;
}

void Engine::requestWrite(const DEVICE_HANDLE &handle)
{
    auto pollfd_it = findHandleInList(handle);

    if(pollfd_it == mPollDevices.end()){
        return;
    }

    pollfd_it->events = POLLOUT;
}

}
