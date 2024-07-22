#include <transport-cpp/networking/udpsender.h>

#include <cstring>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <net/if.h>
#include <ifaddrs.h>

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

RETURN_CODE Sender::connectToHost(const HostAddr &host, IPVersion ip_hint)
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
