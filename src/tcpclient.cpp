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

HostAddr Client::getSetHostAddr()
{
    return mHost;
}

RETURN_CODE Client::connectToHost(const HostAddr &host, IPVersion ip_hint)
{
    AddrInfo info;

    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));

    if(ip_hint == IPVersion::IPv4){
        hints.ai_family = AF_INET;
    }else if(ip_hint == IPVersion::IPv6){
        hints.ai_family = AF_INET6;
    }else{
        hints.ai_family = AF_UNSPEC; // ipv4 or ipv6
    }

    hints.ai_socktype = SOCK_STREAM; //SOCK_DGRAM for UDP | SOCK_STREAM for TCP
    hints.ai_flags = AI_PASSIVE; // fill in ip for me

    auto status = info.loadHints(hints, host);

    if(status != 0){
        setError(ERROR_CODE::GENERAL_ERROR, std::string("unable to get address information: ")
                 + gai_strerror(status));

        return RETURN::NOK;
    }

    auto next_info = info.next();

    if(next_info == nullptr){
        setError(ERROR_CODE::GENERAL_ERROR, "No addresses return in getaddrinfo");
        return RETURN::NOK;
    }

    while(next_info != nullptr){
        auto sock = socket(next_info->ai_family,
                           next_info->ai_socktype,
                           next_info->ai_protocol);

        if(sock == -1){
            setError(errno, "Unable to open socket");
            return RETURN::NOK;
        }

        auto addr = reinterpret_cast<sockaddr *>(malloc(next_info->ai_addrlen));
        memcpy(addr, next_info->ai_addr, next_info->ai_addrlen);
        auto res = connect(sock, addr, next_info->ai_addrlen);

        if(res != 0){
            setError(errno, "Unable to connect socket");
            free(addr);

            return RETURN::NOK;
        }

        free(addr);

        registerNewHandle(sock);

        mIsConnected    = true;
        mHost           = host;

        return RETURN::OK;
    }

    setError(ERROR_CODE::GENERAL_ERROR, "Unkown error");
    return RETURN::NOK;
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

    message.peer = mHost;

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
    close(getDeviceHandle().value());
    destroyHandle();
    notifyOfDisconnect();
}

}
