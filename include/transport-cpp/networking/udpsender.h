#ifndef UDPSENDER_H
#define UDPSENDER_H

#include "networkdevice.h"

namespace Context::Devices::IO::Networking::UDP {

class TRANSPORT_CPP_EXPORT Sender final :
        public NetworkDevice
{
private:
    bool            mIsConnected = false;
    ConnectedHost   mHost;
public:
    Sender();

    void disconnect() noexcept;
    [[nodiscard]] bool isConnected() const noexcept;
    [[nodiscard]] ConnectedHost getSetHostAddr() const noexcept;

    RETURN_CODE connectToHost(const HostAddr &host, const IPVersion &ip_hint = IPVersion::ANY);

private:
    void readyError() override;
};
}

#endif // UDPSENDER_H
