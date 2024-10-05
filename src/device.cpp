#include <cstring>
#include <transport-cpp/device.h>
#include <transport-cpp/engine.h>

#include <poll.h>
#include <unistd.h>

namespace Context {
Device::~Device() {
  logDebug("Device::Destructor", "Destroying device");

  deloadEngine();
}

Context::Device::ENGINE_PTR
Context::Device::getCurrentLoadedEngine() const noexcept {
  return mLoadedEngine;
}

void Context::Device::loadEngine(ENGINE_PTR engine) {
  logDebug("Device::loadEngine", "Loading new engine");

  mLoadedEngine = engine;
}

void Context::Device::deloadEngine() {
  logDebug("Device::deloadEngine", "Deloading Engine");

  const auto currentEngine = mLoadedEngine;
  mLoadedEngine = nullptr;

  if (currentEngine != nullptr) {
    logDebug("Device::deloadEngine",
             "Engine is valid, calling deregister device");
    currentEngine->deRegisterDevice(this);
  }
}

void Device::readyRead() {
  logDebug("Device::readyRead",
           "Device is ready to perform a read, but functionality has not been "
           "implemented by child");
}

void Device::readyWrite() {
  logDebug("Device::readyWrite",
           "Device is ready to perform a write, but functionality has not been "
           "implemented by child");
}

void Device::readyError() {
  logError("Device::readyError", "Device has an error, but functionality has "
                                 "not been implemented by child");
}

void Device::readyHangup() {
  logWarn("Device::readyHangup", "Device peer has hung up, but functionality "
                                 "has not been implemented by child");
}

void Device::readyInvalidRequest() {
  logWarn(
      "Device::readyInvalidRequest",
      "Invalid request, but functionality has not been implemented by child");
}

void Device::readyPeerDisconnect() {
  logWarn("Device::readyPeerDisconnect",
          "Peer has disconnected, but functionality has not been implemented "
          "by child");
}

std::string Device::errCodeToString(const ERROR_CODE &err_code) noexcept {
  std::string err_str;

  switch (err_code) {
  case ERROR_CODE::NO_ERROR:
    err_str = "NO_ERROR";
    break;
  case ERROR_CODE::INVALID_ARGUMENT:
    err_str = "INVALID_ARGUMENT";
    break;
  case ERROR_CODE::INVALID_LOGIC:
    err_str = "INVALID_LOGIC";
    break;
  case ERROR_CODE::DEVICE_NOT_READY:
    err_str = "DEVICE_NOT_READY";
    break;
  case ERROR_CODE::POLL_ERROR:
    err_str = "POLL_ERROR";
    break;
  case ERROR_CODE::TIMEOUT:
    err_str = "TIMEOUT";
    break;
  case ERROR_CODE::GENERAL_ERROR:
    err_str = "GENERAL_ERROR";
    break;
  }

  return err_str;
}

Device::DEVICE_HANDLE Device::getDeviceHandle() const noexcept {
  return mDeviceHandle;
}

Device::ERROR Device::getLastError() const noexcept { return mLastError; }

void Device::setLogger(const LOGGER &logger) noexcept { mLogger = logger; }

Device::Device() = default;

void Device::registerNewHandle(DEVICE_HANDLE handle) {
  logDebug("Device::registerNewHandle", "Registering new handle");

  if (mLoadedEngine != nullptr) {
    mLoadedEngine->registerNewHandle(mDeviceHandle, handle, this);
  }

  mDeviceHandle = handle;
}

void Device::setError(const DEVICE_ERROR &code,
                      const ERROR_STRING &description) {
  logDebug("Device::setError",
           "New error added with description: " + description);

  mLastError.code = code;
  mLastError.description = description;
}

void Device::requestRead() const noexcept {
  logDebug("Device::requestRead", "Request Read");
  if (mLoadedEngine) {
    mLoadedEngine->requestRead(mDeviceHandle);
  }
}

void Device::requestWrite() const noexcept {
  logDebug("Device::requestWrite", "Request Write");
  if (mLoadedEngine) {
    mLoadedEngine->requestWrite(mDeviceHandle);
  }
}

void Device::destroyHandle() {
  closeHandle();

  if (mLoadedEngine) {
    mLoadedEngine->deRegisterHandle(mDeviceHandle);
    mDeviceHandle = {};
  }
}

void Device::closeHandle() {
  if (mDeviceHandle) {
    close(mDeviceHandle.value());
  }
}

void Device::registerChildDevice(Device *device) {
  if (mLoadedEngine) {
    mLoadedEngine->registerDevice(device);
  }
}

void Device::logDebug(const std::string &calling_class,
                      const std::string &message) const {
  if (mLogger) {
    mLogger->logDebug(calling_class, message);
  }
}

void Device::logInfo(const std::string &calling_class,
                     const std::string &message) const {
  if (mLogger) {
    mLogger->logInfo(calling_class, message);
  }
}

void Device::logWarn(const std::string &calling_class,
                     const std::string &message) const {
  if (mLogger) {
    mLogger->logWarn(calling_class, message);
  }
}

void Device::logError(const std::string &calling_class,
                      const std::string &message) const {
  if (mLogger) {
    mLogger->logError(calling_class, message);
  }
}

void Device::logFatal(const std::string &calling_class,
                      const std::string &message) const {
  if (mLogger) {
    mLogger->logFatal(calling_class, message);
  }
}

void Device::logLastError(const std::string &calling_class) {
  const auto &[code, description] = mLastError;

  std::string err_code_str;

  if (std::holds_alternative<ERROR_CODE>(code)) {
    err_code_str =
        "[Internal Error: " + errCodeToString(std::get<ERROR_CODE>(code)) + "]";
  } else {
    const auto error_code = std::get<SYS_ERR_CODE>(code);

    err_code_str = "[System Error: " + std::to_string(error_code) +
                   " | errno desc: " + strerror(error_code) + "]";
  }

  logError(calling_class, err_code_str + ": " + description);
}
} // namespace Context
