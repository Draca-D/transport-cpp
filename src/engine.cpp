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

Engine::Engine(){

}

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
    if(optional_duration){
        auto duration = optional_duration.value();

        awaitOnceUpto(static_cast<int>(duration.count()));
    }else{
        while(!awaitOnceUpto(std::numeric_limits<int>::max())){
            logWarn("Engine", "When awaiting once without a timeout. "
                              "Poll returned with no events pending even with the large timeout");
        }
    }
}

void Engine::awaitFor(const std::chrono::milliseconds &duration)
{
    auto start = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::steady_clock::now() - start;

    while(elapsed < duration){
        auto remaining = duration - elapsed;
        auto remaining_ms = std::chrono::duration_cast<std::chrono::milliseconds>(remaining).count();

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

  std::vector<DEVICE_HANDLE_> ready_read;
  std::vector<DEVICE_HANDLE_> ready_write;
  std::vector<DEVICE_HANDLE_> error;

  ready_read.reserve(static_cast<size_t>(res));
  ready_write.reserve(static_cast<size_t>(res));
  error.reserve(static_cast<size_t>(res));

  for(const auto &fd:mPollDevices){
    if(fd.revents == POLLIN){
      ready_read.push_back(fd.fd);
      count++;
    }else if(fd.revents == POLLOUT){
      ready_write.push_back(fd.fd);
      count++;
    }else if(fd.revents == POLLERR){
      error.push_back(fd.fd);
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

    device->deloadPollStructure();
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
        setError(ERROR_CODE::INVALID_ARGUMENT, "When registering a new handle, a new handle was not provided.");
        return RETURN::PASSABLE;
    }

    auto handle_pos = findHandleInList(old_handle);

    if(handle_pos == mPollDevices.end()){
        pollfd fd;
        fd.fd       = new_handle.value();
        fd.events   = POLLIN;

        mPollDevices.push_back(fd);
        mDeviceMapping.insert({fd.fd, relevant_device});

        auto it_to_poll = (mPollDevices.end() - 1);

        relevant_device->loadPollStructure(it_to_poll);

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

}
