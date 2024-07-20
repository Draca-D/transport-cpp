#ifndef UDPSENDER_H
#define UDPSENDER_H

#include "networkdevice.h"

namespace Context::Devices::IO::Networking::UDP {

class Sender :
        public NetworkDevice
{
private:
    bool            mIsConnected = false;
    ConnectedHost   mHost;
public:
    Sender();

    void disconnect() noexcept;
    bool isConnected() const noexcept;

    ConnectedHost getSetHostAddr() const noexcept;

    RETURN_CODE connectToHost(const HostAddr &host, IPVersion ip_hint = IPVersion::ANY);

private:
    void readyError() override;
};
}

#endif // UDPSENDER_H
