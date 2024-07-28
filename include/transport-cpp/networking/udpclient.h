#ifndef UDPCLIENT_H
#define UDPCLIENT_H

#include "networkdevice.h"

namespace Context::Devices::IO::Networking::UDP {

class TRANSPORT_CPP_EXPORT Client final:
        public NetworkDevice
{
private:
    bool            mIsConnected = false;
    ConnectedHost   mHost;
public:
    Client();

    void disconnect() noexcept;
    [[nodiscard]] bool isConnected() const noexcept;
    [[nodiscard]] ConnectedHost getSetHostAddr() const noexcept;

    RETURN_CODE connectToHost(const HostAddr &host, const IPVersion &ip_hint = IPVersion::ANY);

    SYNC_RX_DATA syncRequestResponse(const IODATA &data);
    SYNC_RX_DATA syncRequestResponse(const IODATA &data, const std::chrono::milliseconds &timeout);

private:
    void readyError() override;
};

}

#endif // UDPCLIENT_H
