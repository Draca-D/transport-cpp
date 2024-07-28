#include <transport-cpp/networking/udpsender.h>

#include <sys/socket.h>

namespace Context::Devices::IO::Networking::UDP {
Sender::Sender() :
    NetworkDevice(), mHost({})
{

}

void Sender::disconnect() noexcept
{
    destroyHandle();
    mIsConnected = false;
}

bool Sender::isConnected() const noexcept
{
    return mIsConnected;
}

ConnectedHost Sender::getSetHostAddr() const noexcept
{
    return mHost;
}

RETURN_CODE Sender::connectToHost(const HostAddr &host, const IPVersion &ip_hint)
{
    disconnect();

    if(createAndConnectSocket(host, ip_hint, SOCK_DGRAM) == RETURN::OK){
        mIsConnected    = true;
        mHost           = {host, ip_hint};

        return RETURN::OK;
    }

    return RETURN::NOK;
}

void Sender::readyError()
{
    connectToHost(mHost.addr, mHost.ip_hint);
}

}
