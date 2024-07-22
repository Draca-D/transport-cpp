#ifndef NETWORKDEVICE_H
#define NETWORKDEVICE_H

#include <utility>

#include "../iodevice.h"

struct addrinfo;

namespace Context::Devices::IO::Networking {
    using ADDR = std::string;
    using PORT = unsigned short;

    enum class IPVersion {
        ANY,
        IPv4,
        IPv6
    };

    struct HostAddr {
        ADDR ip;
        PORT port;
    };

    struct NetworkMessage {
        IODevice::IODATA data;
        HostAddr peer;
    };

    struct ConnectedHost {
        HostAddr addr;
        IPVersion ip_hint;
    };

    class AddrInfo {
        using AddrInfoReturn = int;

    private:
        addrinfo *mInfo = nullptr;
        addrinfo *mCurrent = nullptr;

    public:
        AddrInfo() = default;

        ~AddrInfo();

        AddrInfoReturn loadHints(const addrinfo &hints, const HostAddr &addr);

        addrinfo *next();
    };

    class TRANSPORT_CPP_EXPORT NetworkDevice :
            public IODevice {
        struct OutgoingMessage {
            OutgoingMessage(HostAddr in_addr, std::unique_ptr<IODATA> data_ptr,
                            const IPVersion &version): addr(std::move(in_addr)), ip_hint(version) {
                data = std::move(data_ptr);
            }

            OutgoingMessage(HostAddr in_addr, const std::shared_ptr<IODATA> &data_ptr,
                            const IPVersion &version): addr(std::move(in_addr)), ip_hint(version), data(data_ptr) {
            }

            OutgoingMessage(HostAddr in_addr, const IODATA &data_ptr,
                            const IPVersion &version): addr(std::move(in_addr)), ip_hint(version), data(data_ptr) {
            }

            HostAddr addr;
            IODATA_CHOICE data;
            IPVersion ip_hint;
        };

        using RX_CALLBACK = std::function<void(const NetworkMessage &message)>;
        using SOCK_STYLE = int;
        using DEVICE_HANDLE = std::optional<DEVICE_HANDLE_>;
        using OUTGOING_MESSAGE = OutgoingMessage;
        using SEND_QUEUE = std::queue<OUTGOING_MESSAGE>;

    private:
        RX_CALLBACK mCallback;
        SEND_QUEUE mOutgoingQueue;

    public:
        void setGenericNetworkCallback(const RX_CALLBACK &callback);

        [[nodiscard]] virtual RETURN_CODE sendTo(const HostAddr &dest, const IODATA &message,
                                                 const IPVersion &ip_hint = IPVersion::ANY);

        [[nodiscard]] virtual RETURN_CODE sendTo(const HostAddr &dest, const std::shared_ptr<IODATA> &message,
                                                 const IPVersion &ip_hint = IPVersion::ANY);

        [[nodiscard]] virtual RETURN_CODE sendTo(const HostAddr &dest, std::unique_ptr<IODATA> message,
                                                 const IPVersion &ip_hint = IPVersion::ANY);

        [[nodiscard]] virtual RETURN_CODE syncSendTo(const HostAddr &dest, const IODATA_CHOICE &message,
                                                     const IPVersion &ip_hint = IPVersion::ANY);

    protected:
        NetworkDevice();

        ERROR receiveMessage(NetworkMessage &message);

        void notifyCallback(const NetworkMessage &message);

        RETURN_CODE createAndConnectSocket(const HostAddr &host, const IPVersion &ip_hint,
                                           const SOCK_STYLE &sock_style);

        RETURN_CODE createAndBindSocket(const HostAddr &host, const IPVersion &ip_hint, const SOCK_STYLE &sock_style);

        RETURN_CODE sockToReuse(DEVICE_HANDLE handle);

    private:
        void readyRead() override;

        void readyWrite() override;

        RETURN_CODE performSendTo(const HostAddr &dest, const IODATA_CHOICE &data, const IPVersion &ip_hint);
    };
}

#endif // NETWORKDEVICE_H
