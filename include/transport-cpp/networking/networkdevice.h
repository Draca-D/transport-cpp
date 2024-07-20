#ifndef NETWORKDEVICE_H
#define NETWORKDEVICE_H

#include "../iodevice.h"

struct addrinfo;

namespace Context::Devices::IO::Networking {

using ADDR = std::string;
using PORT = unsigned short;

enum class IPVersion{
    ANY,
    IPv4,
    IPv6
};

struct HostAddr {
    ADDR ip;
    PORT port;
};

struct NetworkMessage{
    Context::Devices::IO::IODevice::IODATA data;
    HostAddr peer;
};

struct ConnectedHost {
    HostAddr    addr;
    IPVersion   ip_hint;
};

class AddrInfo
{
    using AddrInfoReturn = int;

private:
    addrinfo *mInfo     = nullptr;
    addrinfo *mCurrent  = nullptr;

public:
    AddrInfo() {}
    ~AddrInfo();

    AddrInfoReturn loadHints(const addrinfo &hints, const HostAddr &addr);

    addrinfo *next();
};

class TRANSPORT_CPP_EXPORT NetworkDevice :
        public IODevice
{
    using RX_CALLBACK   = std::function<void(const NetworkMessage &message)>;
    using SOCK_STYLE    = int;
    using DEVICE_HANDLE = std::optional<DEVICE_HANDLE_>;

private:
    RX_CALLBACK mCallback;

public:
    void setGenericNetworkCallback(RX_CALLBACK callback);

protected:
    NetworkDevice();

    ERROR receiveMessage(NetworkMessage &message);
    void notifyCallback(const NetworkMessage &message);

    RETURN_CODE createAndConnectSocket(const HostAddr &host, const IPVersion &ip_hint, const SOCK_STYLE &sock_style);
    RETURN_CODE createAndBindSocket(const HostAddr &host, const IPVersion &ip_hint, const SOCK_STYLE &sock_style);

    RETURN_CODE sockToReuse(DEVICE_HANDLE handle);
private:
    void readyRead() override;
};
}

#endif // NETWORKDEVICE_H
