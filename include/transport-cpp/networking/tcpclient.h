#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "networkdevice.h"

namespace Context::Devices::IO::Networking::TCP {

class TRANSPORT_CPP_EXPORT Client :
        public NetworkDevice
{
    using DISCONNECT_NOTIFY = std::function<void(Client*)>;

private:
    HostAddr            mHost;
    DISCONNECT_NOTIFY   mToNotify;
    bool                mIsConnected = false;

public:
    Client();

    HostAddr getSetHostAddr();

    RETURN_CODE connectToHost(const HostAddr &host, IPVersion ip_hint = IPVersion::ANY);
    void setDisconnectNotification(const DISCONNECT_NOTIFY handler);

    SYNC_RX_DATA syncRequestResponse(const IODATA &data);
    SYNC_RX_DATA syncRequestResponse(const IODATA &data, const std::chrono::milliseconds &timeout);

private:
    void readyRead() override;
    void readyHangup() override;
    void readyPeerDisconnect() override;

    void notifyOfDisconnect();
    void peerDisconnected();

};

}

#endif // TCPCLIENT_H
