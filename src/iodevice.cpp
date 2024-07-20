#include "transport-cpp/iodevice.h"

#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <fcntl.h>

namespace Context::Devices::IO {

IODevice::~IODevice()
{
    auto opt_hndl = getDeviceHandle();

    if(!opt_hndl){
        return;
    }

    auto handle = opt_hndl.value();

    close(handle);
}

void IODevice::setIODataCallback(IODATA_CALLBACK callback)
{
    logDebug("IODevice", "Callback updated");

    mCallback = callback;

    ioDataCallbackSet();
}

RETURN_CODE IODevice::asyncSend(const IODATA &data)
{
    if(!isValidForOutgoinAsync()){
        return RETURN::NOK;
    }

    mOutgoingQueue.push(data);

    requestWrite();

    return RETURN::OK;
}

RETURN_CODE IODevice::asyncSend(const std::shared_ptr<IODATA> data)
{
    if(!data){
        setError(ERROR_CODE::INVALID_ARGUMENT, "Shared ptr provided to asyncSend has not been initialised");
        return RETURN::NOK;
    }

    if(!isValidForOutgoinAsync()){
        return RETURN::NOK;
    }

    mOutgoingSharedQueue.push(data);

    requestWrite();

    return RETURN::OK;
}

RETURN_CODE IODevice::syncSend(const IODATA &data)
{
    return syncSend(&data);
}

RETURN_CODE IODevice::syncSend(const std::shared_ptr<IODATA> data)
{
    if(!data){
        setError(ERROR_CODE::INVALID_ARGUMENT, "Shared ptr provided to syncSend has not been initialised");
        return RETURN::NOK;
    }

    return syncSend(data.get());
}

IODevice::SYNC_RX_DATA IODevice::syncReceive(const std::chrono::milliseconds &timeout)
{
    SYNC_RX_DATA ret;

    ret.code = RETURN::OK;

    auto opt_hndl = getDeviceHandle();

    if(!opt_hndl){
        setError(ERROR_CODE::DEVICE_NOT_READY, "Device has not been configured yet. Unable to receive");
        ret.code = RETURN::NOK;

        return ret;
    }

    auto handle = opt_hndl.value();

    pollfd fd;

    fd.events = POLLIN;
    fd.fd = handle;

    auto start = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::steady_clock::now() - start;

    while(elapsed < timeout){
        auto remaining = timeout - elapsed;
        auto remaining_ms = std::chrono::duration_cast<std::chrono::milliseconds>(remaining).count();

        if(remaining_ms > std::numeric_limits<int>::max()){
            remaining_ms = std::numeric_limits<int>::max();
        }

        ret.data = IODATA();

        auto resp = poll(&fd, 1, static_cast<int>(remaining_ms));

        if(resp > 0){
            auto read_resp = readIOData(ret.data.value());

            if(std::holds_alternative<ERROR_CODE>(read_resp.code) &&
                    std::get<ERROR_CODE>(read_resp.code) == ERROR_CODE::NO_ERROR){
                return ret;
            }

            ret.code = RETURN::NOK;
            setError(read_resp.code, read_resp.description);
            return ret;

        }else if (resp == 0){
            elapsed = std::chrono::steady_clock::now() - start;
        }else{
            ret.code = RETURN::NOK;

            setError(ERROR_CODE::POLL_ERROR, "Poll returned an error");

            return ret;
        }
    }

    ret.code = RETURN::NOK;
    setError(ERROR_CODE::TIMEOUT, "sync read reached timeout");
    return ret;
}

IODevice::SYNC_RX_DATA IODevice::syncReceive()
{
    SYNC_RX_DATA ret;

    ret.code = RETURN::OK;

    auto opt_hndl = getDeviceHandle();

    if(!opt_hndl){
        setError(ERROR_CODE::DEVICE_NOT_READY, "Device has not been configured yet. Unable to receive");
        ret.code = RETURN::NOK;

        return ret;
    }

    auto handle = opt_hndl.value();

    pollfd fd;

    fd.events = POLLIN;
    fd.fd = handle;

    ret.data = IODATA();
    auto resp = poll(&fd, 1, -1);

    if(resp > 0){
        auto read_resp = readIOData(ret.data.value());

        if(!std::holds_alternative<ERROR_CODE>(read_resp.code) ||
                std::get<ERROR_CODE>(read_resp.code) != ERROR_CODE::NO_ERROR){

            ret.code = RETURN::NOK;
            setError(read_resp.code, read_resp.description);
        }
    }else{
        ret.code = RETURN::NOK;
        setError(ERROR_CODE::POLL_ERROR, "Unknown error in poll");
    }

    return ret;
}

IODevice::IODevice() :
    Device()
{

}

void IODevice::ioDataCallbackSet() {/*unused*/}

void IODevice::registerNewHandle(DEVICE_HANDLE handle)
{
    Device::registerNewHandle(handle);

    if(!handle){
        return;
    }

    auto hndl = handle.value();

    auto flags = fcntl(hndl, F_GETFL);

    if(flags == -1){
        logError("IODevice/registerNewHandle", "Unable to get handle flags: " + std::string(strerror(errno)));
        return;
    }

    if(fcntl(hndl, F_SETFL, flags | O_NONBLOCK) == -1){
        logError("IODevice/registerNewHandle", "could not set file flags: " + std::string(strerror(errno)));
        return;
    }
}

void IODevice::readyWrite()
{
    if(mOutgoingQueue.empty() && mOutgoingSharedQueue.empty()){
        requestRead();
        return;
    }

    if(!getDeviceHandle()){
        logError("IODevice/readyWrite", "Somehow got to readyWrite without a configured file descriptor");
        return;
    }

    bool popOutgoing = true;

    IODATA *next;

    if(!mOutgoingQueue.empty()){
        next = &mOutgoingQueue.front();
    }else{
        next = mOutgoingSharedQueue.front().get();
        popOutgoing = false;
    }

    auto ret = syncSend(next);

    if(popOutgoing){
        mOutgoingQueue.pop();
    }else{
        mOutgoingSharedQueue.pop();
    }

    if(ret == RETURN::NOK){
        logError("IODevice/readyWrite", "Unable to write to provided file descriptor. Error: " +
                 std::string(strerror(errno)));
    }

    requestWrite();
}

void IODevice::readyRead()
{
    logDebug("IODevice\readyReady", "incoming data");

    IODATA data;

    auto read_resp = readIOData(data);

    if(!std::holds_alternative<ERROR_CODE>(read_resp.code) ||
            std::get<ERROR_CODE>(read_resp.code) != ERROR_CODE::NO_ERROR){
        logError("IODevice/readyRead", "Error reading descriptor. " + read_resp.description);
        return;
    }

    notifyIOCallback(data);
}

void IODevice::readyError()
{
    logError("IODevice", "readyError, unknown error");
}

Device::ERROR IODevice::readIOData(IODATA &data)
{
    data.clear();

    ERROR err;
    err.code = ERROR_CODE::NO_ERROR;

    static thread_local BYTE buffer[2048];

    auto handle = getDeviceHandle().value();

    auto nbytes = read(handle, buffer, sizeof(buffer));

    if(nbytes == -1){
        err.code = errno;
        err.description = "read error";
    }

    while(nbytes > 0){
        data.reserve(data.capacity() + static_cast<size_t>(nbytes));

        for(int x = 0; x < nbytes; x++){
            data.push_back(buffer[x]);
        }

        nbytes = read(handle, buffer, sizeof(buffer));
    }

    return err;
}

void Context::Devices::IO::IODevice::notifyIOCallback(const IODATA &data)
{
    if(mCallback){
        mCallback(data);
    }
}

bool IODevice::isValidForOutgoinAsync()
{
    if(getCurrentLoadedEngine() == nullptr){
        setError(ERROR_CODE::INVALID_LOGIC, "Asynchronous sends can only be "
                                            "performed when a device is loaded into an engine."
                                            " Message will be dropped");

        return false;
    }

    if(!getDeviceHandle()){
        setError(ERROR_CODE::DEVICE_NOT_READY, "Device has not been configured yet. Unable to send. Dropping message");
        return false;
    }

    return true;
}

RETURN_CODE IODevice::syncSend(const IODATA *data)
{
    auto opt_hndl = getDeviceHandle();

    if(!opt_hndl){
        setError(ERROR_CODE::DEVICE_NOT_READY, "Device has not been configured yet. Unable to send. Dropping message");
        return RETURN::NOK;
    }

    auto handle = opt_hndl.value();

    pollfd fd;

    fd.events = POLLOUT;
    fd.fd = handle;

    auto nres = poll(&fd, 1, -1);

    if(nres == -1){
        setError(errno, "Device cannot be polled for pollout");
        return RETURN::NOK;
    }else if(nres == 0){
        setError(ERROR_CODE::POLL_ERROR, "Poll returned 0 available devices for a forever timeout on sync send");
        return RETURN::NOK;
    }

    if(fd.revents == POLLERR){
        readyError();

        setError(ERROR_CODE::POLL_ERROR, "Poll had an error");
        return RETURN::NOK;

    }else if(fd.revents == POLLHUP){
        readyHangup();

        setError(ERROR_CODE::POLL_ERROR, "Peer hung up");
        return RETURN::NOK;

    }else if(fd.revents == POLLRDHUP){
        readyPeerDisconnect();

        setError(ERROR_CODE::POLL_ERROR, "Peer disconnected");
        return RETURN::NOK;
    }

    auto ret = write(handle, data->data(), data->size());

    if(ret < 0){
        setError(errno, "Unable to write to provided file descriptor");
        return RETURN::NOK;
    }

    requestRead();

    return RETURN::OK;
}
}
