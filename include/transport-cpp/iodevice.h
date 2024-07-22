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
public:
    using BYTE              = int8_t;
    using IODATA            = std::vector<BYTE>;
    using IODATA_CHOICE     = std::variant<std::shared_ptr<IODATA>, IODATA, std::unique_ptr<IODATA>>;

private:
    using DEVICE_HANDLE     = std::optional<DEVICE_HANDLE_>;
    using IODATA_CALLBACK   = std::function<void(const IODATA &)>;
    using ASYNC_QUEUE       = std::queue<IODATA_CHOICE>;

private:
    IODATA_CALLBACK mCallback;
    ASYNC_QUEUE     mOutgoingQueue;

public:
    virtual ~IODevice() override;

    struct ReceivedData{
        RETURN_CODE             code{};
        std::optional<IODATA>   data;
    };

    using SYNC_RX_DATA = ReceivedData;

    void setIODataCallback(const IODATA_CALLBACK &callback);

    [[nodiscard]] virtual RETURN_CODE asyncSend(const IODATA &data);
    [[nodiscard]] virtual RETURN_CODE asyncSend(const std::shared_ptr<IODATA> &data);
    [[nodiscard]] virtual RETURN_CODE asyncSend(std::unique_ptr<IODATA> data); //message will be moved, original value will be invalidated
    [[nodiscard]] virtual RETURN_CODE syncSend(const IODATA_CHOICE &data);

    virtual SYNC_RX_DATA syncReceive(const std::chrono::milliseconds &timeout);
    virtual SYNC_RX_DATA syncReceive();

protected:
    IODevice();

    ERROR readIOData(IODATA &data) const noexcept;

    void notifyIOCallback(const IODATA &data) const;

    void registerNewHandle(DEVICE_HANDLE handle) override;

    void readyWrite() override;
    void readyRead() override;
    void readyError() override;

    bool isValidForOutgoinAsync();

    [[nodiscard]] static bool ioDataChoiceValid(const IODATA_CHOICE &data) noexcept;

private:
    virtual void ioDataCallbackSet();

    RETURN_CODE performSyncSend(const IODATA_CHOICE &data);
};

}

#endif // IODEVICE_H
