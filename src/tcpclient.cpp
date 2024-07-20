#include <transport-cpp/networking/tcpclient.h>

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

namespace Context::Devices::IO::Networking::TCP {

Client::Client() :
    NetworkDevice()
{

}

ConnectedHost Client::getSetHostAddr()
{
    return mHost;
}

void Client::disconnect()
{
    destroyHandle();
    mIsConnected = false;
}

RETURN_CODE Client::connectToHost(const HostAddr &host, IPVersion ip_hint)
{
    disconnect();

    if(createAndConnectSocket(host, ip_hint, SOCK_STREAM) == RETURN::OK){
        mIsConnected    = true;
        mHost           = {host, ip_hint};

        return RETURN::OK;
    }

    return RETURN::NOK;
}

RETURN_CODE Client::connectToHost(const ConnectedHost &host)
{
    return connectToHost(host.addr, host.ip_hint);
}

void Client::setDisconnectNotification(const DISCONNECT_NOTIFY handler)
{
    mToNotify = handler;
}

IODevice::SYNC_RX_DATA Client::syncRequestResponse(const IODATA &data)
{
    auto send_ret = syncSend(data);

    if(send_ret == RETURN::NOK){
        return SYNC_RX_DATA{RETURN::NOK, {}};
    }

    return syncReceive();
}

IODevice::SYNC_RX_DATA Client::syncRequestResponse(
        const IODATA &data, const std::chrono::milliseconds &timeout)
{
    auto send_ret = syncSend(data);

    if(send_ret == RETURN::NOK){
        return SYNC_RX_DATA{RETURN::NOK, {}};
    }

    return syncReceive(timeout);
}

void Client::readyRead()
{
    logDebug("TCPClient\readyReady", "incoming data");

    NetworkMessage message;

    auto read_resp = readIOData(message.data);

    if(message.data.empty()){
        logDebug("TCPClient\readyRead", "Peer closed connection");
        peerDisconnected();
        return;
    }

    if(!std::holds_alternative<ERROR_CODE>(read_resp.code) ||
            std::get<ERROR_CODE>(read_resp.code) != ERROR_CODE::NO_ERROR){
        logError("TCPClient/readyRead", "Error reading descriptor. " + read_resp.description);
        return;
    }

    message.peer = mHost.addr;

    notifyCallback(message);
}

void Client::readyHangup()
{
    logDebug("TCPClient\readyHangup", "Peer closed connection");
    peerDisconnected();
}

void Client::readyPeerDisconnect()
{
    logDebug("TCPClient\readyPeerDisconnect", "Peer closed connection");
    peerDisconnected();
}

void Client::notifyOfDisconnect()
{
    if(mToNotify){
        mToNotify(this);
    }
}

void Client::peerDisconnected()
{
    mIsConnected = false;

    destroyHandle();
    notifyOfDisconnect();
}

}
