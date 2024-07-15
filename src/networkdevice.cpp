#include <transport-cpp/networking/networkdevice.h>

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

static constexpr int32_t RECV_BUFFER_LEN    = 65536;
static constexpr int32_t IP_NAME_BUF_LEN    = 256;
static constexpr int32_t PORT_BUF_LEN       = 256;

namespace Context::Devices::IO::Networking {
void NetworkDevice::setGenericNetworkCallback(RX_CALLBACK callback)
{
    mCallback = callback;
}

NetworkDevice::NetworkDevice() :
    IODevice()
{

}

Device::ERROR NetworkDevice::receiveMessage(NetworkMessage &message)
{
    ERROR err;
    err.code = ERROR_CODE::NO_ERROR;

    static thread_local char buffer[RECV_BUFFER_LEN];

    auto handle = getDeviceHandle().value();

    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len = sizeof(peer_addr);

    auto nbytes = recvfrom(handle, buffer, RECV_BUFFER_LEN, 0,
                           reinterpret_cast<sockaddr *>(&peer_addr),
                           &peer_addr_len);

    if(nbytes == -1){
        err.code = errno;
        err.description = "read error";
    }else{
        char ip[IP_NAME_BUF_LEN];
        char port[PORT_BUF_LEN];

        auto peer = reinterpret_cast<sockaddr *>(&peer_addr);

        int rc = getnameinfo(peer, peer_addr_len, ip, IP_NAME_BUF_LEN, port, PORT_BUF_LEN,
                             NI_NUMERICHOST | NI_NUMERICSERV);

        if(rc == -1){
            err.code = errno;
            err.description = "Unable to extract peer address";
            return err;
        }

        message.peer.ip = ip;
        message.peer.port = static_cast<decltype(message.peer.port)>(std::stoul(port));
    }


    message.data.reserve(message.data.capacity() + static_cast<size_t>(nbytes));

    for(int x = 0; x < nbytes; x++){
        message.data.push_back(buffer[x]);
    }

    return err;
}

void NetworkDevice::notifyCallback(const NetworkMessage &message)
{
    if(mCallback){
        mCallback(message);
    }
}

void NetworkDevice::readyRead()
{
    NetworkMessage data;

    auto read_resp = receiveMessage(data);

    if(!std::holds_alternative<ERROR_CODE>(read_resp.code) ||
            std::get<ERROR_CODE>(read_resp.code) != ERROR_CODE::NO_ERROR){
        logError("NetworkDevice/readyRead", "Error reading descriptor. " + read_resp.description);
        return;
    }

    if(mCallback){
        mCallback(data);
    }
}

AddrInfo::~AddrInfo()
{
    if(!mInfo){
        return;
    }

    freeaddrinfo(mInfo);
}

AddrInfo::AddrInfoReturn AddrInfo::loadHints(const addrinfo &hints, const HostAddr &addr)
{
    if(mInfo){
        freeaddrinfo(mInfo);
        mInfo       = nullptr;
        mCurrent    = nullptr;
    }

    auto status = getaddrinfo(
                addr.ip.c_str(),
                std::to_string(addr.port).c_str(),
                &hints, &mInfo);

    if(status != 0){
        mInfo = nullptr;
    }

    return status;
}

addrinfo *AddrInfo::next()
{
    if(mCurrent == nullptr){
        mCurrent = mInfo;
        return mCurrent;
    }

    if(mCurrent->ai_next == nullptr){
        return nullptr;
    }

    mCurrent    = mCurrent->ai_next;
    return mCurrent;
}

}
