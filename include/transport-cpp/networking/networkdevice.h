#ifndef NETWORKDEVICE_H
#define NETWORKDEVICE_H

#include "../iodevice.h"

struct addrinfo;

namespace Context::Devices::IO::Networking {

using ADDR = std::string;
using PORT = unsigned short;

struct HostAddr {
    ADDR ip;
    PORT port;
};

struct NetworkMessage{
    Context::Devices::IO::IODevice::IODATA data;
    HostAddr peer;
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

enum class IPVersion{
    ANY,
    IPv4,
    IPv6
};

class TRANSPORT_CPP_EXPORT NetworkDevice :
        public IODevice
{
    using RX_CALLBACK = std::function<void(const NetworkMessage &message)>;

private:
    RX_CALLBACK mCallback;

public:
    void setGenericNetworkCallback(RX_CALLBACK callback);

protected:
    NetworkDevice();

    ERROR receiveMessage(NetworkMessage &message);
    void notifyCallback(const NetworkMessage &message);

private:
    void readyRead() override;
};
}

#endif // NETWORKDEVICE_H
