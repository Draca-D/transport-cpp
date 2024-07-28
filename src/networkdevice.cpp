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

static constexpr int32_t RECV_BUFFER_LEN = 65536;
static constexpr int32_t IP_NAME_BUF_LEN = 256;
static constexpr int32_t PORT_BUF_LEN = 256;

namespace Context::Devices::IO::Networking {
    void NetworkDevice::setGenericNetworkCallback(const RX_CALLBACK &callback) {
        mCallback = callback;
    }

    RETURN_CODE NetworkDevice::sendTo(const HostAddr &dest, const IODATA &message, const IPVersion &ip_hint) {
        if (!isValidForOutgoinAsync()) {
            return RETURN::NOK;
        }

        if (!ioDataChoiceValid(message)) {
            setError(ERROR_CODE::INVALID_LOGIC, "Provided data has not been initialised");
            return RETURN::NOK;
        }

        mOutgoingQueue.emplace(dest, message, ip_hint);

        requestWrite();

        return RETURN::OK;
    }

    RETURN_CODE NetworkDevice::sendTo(const HostAddr &dest, const std::shared_ptr<IODATA> &message,
                                      const IPVersion &ip_hint) {
        if (!isValidForOutgoinAsync()) {
            return RETURN::NOK;
        }

        if (!ioDataChoiceValid(message)) {
            setError(ERROR_CODE::INVALID_LOGIC, "Provided data has not been initialised");
            return RETURN::NOK;
        }

        mOutgoingQueue.emplace(dest, message, ip_hint);

        requestWrite();

        return RETURN::OK;
    }

    RETURN_CODE NetworkDevice::sendTo(const HostAddr &dest, std::unique_ptr<IODATA> message, const IPVersion &ip_hint) {
        if (!isValidForOutgoinAsync()) {
            return RETURN::NOK;
        }

        if (!message) {
            setError(ERROR_CODE::INVALID_LOGIC, "Provided data has not been initialised");
            return RETURN::NOK;
        }

        mOutgoingQueue.emplace(dest, std::move(message), ip_hint);

        requestWrite();

        return RETURN::OK;
    }

    RETURN_CODE NetworkDevice::syncSendTo(const HostAddr &dest, const IODATA_CHOICE &message,
                                          const IPVersion &ip_hint) {
        if (!ioDataChoiceValid(message)) {
            setError(ERROR_CODE::INVALID_LOGIC, "Provided data has not been initialised");
            return RETURN::NOK;
        }

        return performSendTo(dest, message, ip_hint);
    }

    NetworkDevice::NetworkDevice() : IODevice() {
    }

    Device::ERROR NetworkDevice::receiveMessage(NetworkMessage &message) {
        ERROR err;
        err.code = ERROR_CODE::NO_ERROR;

        thread_local char buffer[RECV_BUFFER_LEN];

        const auto handle = getDeviceHandle().value();

        sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);

        auto nbytes = recvfrom(handle, buffer, RECV_BUFFER_LEN, 0,
                               reinterpret_cast<sockaddr *>(&peer_addr),
                               &peer_addr_len);

        if (nbytes == -1) {
            err.code = errno;
            err.description = "read error";
        } else {
            char ip[IP_NAME_BUF_LEN];
            char port[PORT_BUF_LEN];

            const auto peer = reinterpret_cast<sockaddr *>(&peer_addr);

            if (getnameinfo(peer, peer_addr_len, ip, IP_NAME_BUF_LEN, port, PORT_BUF_LEN,
                            NI_NUMERICHOST | NI_NUMERICSERV) == -1) {
                err.code = errno;
                err.description = "Unable to extract peer address";
                return err;
            }

            message.peer.ip = ip;
            message.peer.port = static_cast<decltype(message.peer.port)>(std::stoul(port));
        }

        while (nbytes > 0) {
            message.data.reserve(message.data.capacity() + static_cast<size_t>(nbytes));

            for (int x = 0; x < nbytes; x++) {
                message.data.push_back(buffer[x]);
            }

            nbytes = recvfrom(handle, buffer, RECV_BUFFER_LEN, 0,
                              reinterpret_cast<sockaddr *>(&peer_addr),
                              &peer_addr_len);
        }

        return err;
    }

    void NetworkDevice::notifyCallback(const NetworkMessage &message) {
        if (mCallback) {
            mCallback(message);
        }

        notifyIOCallback(message.data);
    }

    RETURN_CODE NetworkDevice::createAndConnectSocket(
        const HostAddr &host,
        const IPVersion &ip_hint,
        const SOCK_STYLE &sock_style) {
        AddrInfo info;

        addrinfo hints;

        memset(&hints, 0, sizeof(hints));

        if (ip_hint == IPVersion::IPv4) {
            hints.ai_family = AF_INET;
        } else if (ip_hint == IPVersion::IPv6) {
            hints.ai_family = AF_INET6;
        } else {
            hints.ai_family = AF_UNSPEC; // ipv4 or ipv6
        }

        hints.ai_socktype = sock_style; //SOCK_DGRAM for UDP | SOCK_STREAM for TCP
        hints.ai_flags = AI_PASSIVE; // fill in ip for me

        auto status = info.loadHints(hints, host);

        if (status != 0) {
            setError(ERROR_CODE::GENERAL_ERROR, std::string("unable to get address information: ")
                                                + gai_strerror(status));

            return RETURN::NOK;
        }

        auto next_info = info.next();

        if (next_info == nullptr) {
            setError(ERROR_CODE::GENERAL_ERROR, "No addresses return in getaddrinfo");
            return RETURN::NOK;
        }

        for (; next_info != nullptr; next_info = info.next()) {
            auto sock = socket(next_info->ai_family,
                               next_info->ai_socktype,
                               next_info->ai_protocol);

            if (sock == -1) {
                setError(errno, "Unable to open socket");
                return RETURN::NOK;
            }

            auto addr = reinterpret_cast<sockaddr *>(malloc(next_info->ai_addrlen));
            memcpy(addr, next_info->ai_addr, next_info->ai_addrlen);
            auto res = connect(sock, addr, next_info->ai_addrlen);

            if (res != 0) {
                close(sock);
                free(addr);

                continue;
            }

            free(addr);

            registerNewHandle(sock);

            return RETURN::OK;
        }

        setError(errno, "Unable to connect socket");
        return RETURN::NOK;
    }

    RETURN_CODE NetworkDevice::createAndBindSocket(
        const HostAddr &host,
        const IPVersion &ip_hint,
        const SOCK_STYLE &sock_style) {
        AddrInfo info;

        addrinfo hints;

        memset(&hints, 0, sizeof(hints));

        if (ip_hint == IPVersion::IPv4) {
            hints.ai_family = AF_INET;
        } else if (ip_hint == IPVersion::IPv6) {
            hints.ai_family = AF_INET6;
        } else {
            hints.ai_family = AF_UNSPEC; // ipv4 or ipv6
        }

        hints.ai_socktype = sock_style; //SOCK_DGRAM for UDP | SOCK_STREAM for TCP
        hints.ai_flags = AI_PASSIVE; // fill in ip for me

        auto status = info.loadHints(hints, host);

        if (status != 0) {
            setError(ERROR_CODE::GENERAL_ERROR, std::string("unable to get address information: ")
                                                + gai_strerror(status));

            return RETURN::NOK;
        }

        auto next_info = info.next();

        if (next_info == nullptr) {
            setError(ERROR_CODE::GENERAL_ERROR, "No addresses return in getaddrinfo");
            return RETURN::NOK;
        }

        for (; next_info != nullptr; next_info = info.next()) {
            auto sock = socket(next_info->ai_family,
                               next_info->ai_socktype,
                               next_info->ai_protocol);

            if (sock == -1) {
                setError(errno, "Unable to open socket");
                return RETURN::NOK;
            }

            auto res = ::bind(sock, next_info->ai_addr,
                              next_info->ai_addrlen);

            if (res != 0) {
                close(sock);
                continue;
            }

            if (sockToReuse(sock) == RETURN::NOK) {
                close(sock);
                return RETURN::NOK;
            }

            registerNewHandle(sock);

            return RETURN::OK;
        }

        setError(errno, "Unable to bind socket");

        return RETURN::NOK;
    }

    void NetworkDevice::readyRead() {
        NetworkMessage data;

        auto read_resp = receiveMessage(data);

        if (!std::holds_alternative<ERROR_CODE>(read_resp.code) ||
            std::get<ERROR_CODE>(read_resp.code) != ERROR_CODE::NO_ERROR) {
            logError("NetworkDevice/readyRead", "Error reading descriptor. " + read_resp.description);
            return;
        }

        if (mCallback) {
            mCallback(data);
        }
    }

    void NetworkDevice::readyWrite() {
        if (mOutgoingQueue.empty()) {
            IODevice::readyWrite();
            return;
        }

        const auto &[dest, message, ip_hint] = mOutgoingQueue.front();
        auto ret = performSendTo(dest, message, ip_hint);

        if (ret == RETURN::NOK) {
            auto last_error = getLastError();

            if (std::holds_alternative<int>(last_error.code)) {
                logError("NetworkDevice/readyWrite", "[Sys] Unable to send to an address. "
                                                     "Error code description: " + std::string(
                                                         strerror(std::get<int>(last_error.code))));
            } else {
                logError("NetworkDevice/readyWrite", "[Internal error] Unable to send to. Desc: " +
                                                     last_error.description);
            }
        }

        mOutgoingQueue.pop();
        requestWrite();
    }

    RETURN_CODE NetworkDevice::performSendTo(const HostAddr &dest, const IODATA_CHOICE &data,
                                             const IPVersion &ip_hint) {
        if (!getDeviceHandle()) {
            setError(ERROR_CODE::INVALID_LOGIC, "Cannot send without first initialising a socket");
            return RETURN::NOK;
        }

        auto sock = getDeviceHandle().value();

        AddrInfo info;

        addrinfo hints;

        memset(&hints, 0, sizeof(hints));

        if (ip_hint == IPVersion::IPv4) {
            hints.ai_family = AF_INET;
        } else if (ip_hint == IPVersion::IPv6) {
            hints.ai_family = AF_INET6;
        } else {
            hints.ai_family = AF_UNSPEC; // ipv4 or ipv6
        }

        hints.ai_flags = AI_PASSIVE; // fill in ip for me

        auto status = info.loadHints(hints, dest);

        if (status != 0) {
            setError(ERROR_CODE::GENERAL_ERROR, std::string("unable to get address information: ")
                                                + gai_strerror(status));

            return RETURN::NOK;
        }

        auto next_info = info.next();

        if (next_info == nullptr) {
            setError(ERROR_CODE::GENERAL_ERROR, "No addresses return in getaddrinfo");
            return RETURN::NOK;
        }

        const IODATA *data_ptr;

        if (std::holds_alternative<IODATA>(data)) {
            data_ptr = &std::get<IODATA>(data);
        } else if (std::holds_alternative<std::shared_ptr<IODATA> >(data)) {
            data_ptr = std::get<std::shared_ptr<IODATA> >(data).get();
        } else {
            data_ptr = std::get<std::unique_ptr<IODATA> >(data).get();
        }

        auto nWrote = sendto(sock,
                             data_ptr->data(), data_ptr->size(), 0, next_info->ai_addr,
                             next_info->ai_addrlen);

        if (nWrote < 0) {
            setError(errno, "System error returned when performing a sendTo");
            return RETURN::NOK;
        }

        return RETURN::OK;
    }

    RETURN_CODE NetworkDevice::sockToReuse(DEVICE_HANDLE handle) {
        if (!handle) {
            setError(ERROR_CODE::INVALID_LOGIC, "reuse requested, but no device handle present");
            return RETURN::NOK;
        }

        const auto sock = handle.value();
        constexpr int yes = 1;

        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            // allow reuse of port
            setError(errno, "Unable to configure socket for reuse");
            return RETURN::NOK;
        }

        return RETURN::OK;
    }

    AddrInfo::~AddrInfo() {
        if (!mInfo) {
            return;
        }

        freeaddrinfo(mInfo);
    }

    AddrInfo::AddrInfoReturn AddrInfo::loadHints(const addrinfo &hints, const HostAddr &addr) {
        if (mInfo) {
            freeaddrinfo(mInfo);
            mInfo = nullptr;
            mCurrent = nullptr;
        }

        int status = getaddrinfo(
            addr.ip.c_str(),
            std::to_string(addr.port).c_str(),
            &hints, &mInfo);

        if (status != 0) {
            mInfo = nullptr;
        }

        return status;
    }

    addrinfo *AddrInfo::next() {
        if (mCurrent == nullptr) {
            mCurrent = mInfo;
            return mCurrent;
        }

        if (mCurrent->ai_next == nullptr) {
            return nullptr;
        }

        mCurrent = mCurrent->ai_next;
        return mCurrent;
    }
}
