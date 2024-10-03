#ifndef UDPMULTICASTER_H
#define UDPMULTICASTER_H

#include "networkdevice.h"

struct sockaddr;

namespace Context::Devices::IO::Networking::UDP {


class TRANSPORT_CPP_EXPORT Multicaster final :
        public NetworkDevice
{
private:
    std::optional<HostAddr> mSubscribedAddr;
    std::optional<HostAddr> mPublishedAddr;

    IFACE mSetInterface;

    bool mInitalised = false;
    IPVersion mIpVersion;

    sockaddr *mPublishedSockAddr = nullptr;
    size_t mPublishedSockAddrLen;

public:
    Multicaster();
    ~Multicaster() override;

    void deInitialise();
    [[nodiscard]] RETURN_CODE initialise(const IPVersion &ip_version = IPVersion::IPv4);

    [[nodiscard]] RETURN_CODE publishToGroup(const HostAddr &group);
    [[nodiscard]] RETURN_CODE subscribeToGroup(const HostAddr &group);

    [[nodiscard]] RETURN_CODE setInterface(const std::string &iface_name);
    [[nodiscard]] RETURN_CODE setInterface(const IFACE &iface);

    [[nodiscard]] RETURN_CODE setLoopback(const bool &enable);
private:
    [[nodiscard]] bool deviceIsReady() const override;

    void readyWrite() override;
};
}

#endif // UDPMULTICASTER_H
