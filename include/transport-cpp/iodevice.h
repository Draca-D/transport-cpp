#ifndef IODEVICE_H
#define IODEVICE_H

#include "device.h"

#include <functional>
#include <queue>
#include <chrono>
#include <memory>

namespace Context::Devices::IO {

class TRANSPORT_CPP_EXPORT IODevice :
        public Device
{
    using BYTE              = int8_t;
    using IODATA            = std::vector<BYTE>;
    using IODATA_CALLBACK   = std::function<void(const IODATA &)>;
    using ASYNC_QUEUE       = std::queue<IODATA>;
    using ASYNC_SHR_QUEUE   = std::queue<std::shared_ptr<IODATA>>;

private:
    IODATA_CALLBACK mCallback;
    ASYNC_QUEUE     mOutgoingQueue;
    ASYNC_SHR_QUEUE mOutgoingSharedQueue;

public:
    virtual ~IODevice() override;

    struct ReceivedData{
        RETURN_CODE             code;
        std::optional<IODATA>   data;
    };

    using SYNC_RX_DATA = ReceivedData;

    void setIODataCallback(IODATA_CALLBACK callback);

    RETURN_CODE asyncSend(const IODATA &data);
    RETURN_CODE asyncSend(const std::shared_ptr<IODATA> data);
    RETURN_CODE syncSend(const IODATA &data);
    RETURN_CODE syncSend(const std::shared_ptr<IODATA> data);

    SYNC_RX_DATA syncReceive(const std::chrono::milliseconds &timeout);
    SYNC_RX_DATA syncReceive();

protected:
    IODevice();

private:
    virtual void ioDataCallbackSet();

    void readyWrite() override;
    void readyRead() override;
    void readyError() override;

    ERROR readIOData(IODATA &data);

    bool isValidForOutgoinAsync();

    RETURN_CODE syncSend(const IODATA *data);
};

}

#endif // IODEVICE_H
