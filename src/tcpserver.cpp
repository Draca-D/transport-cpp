#include <transport-cpp/engine.h>
#include <transport-cpp/networking/tcpserver.h>

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace Context::Devices::IO::Networking::TCP::Server {

Acceptor::Acceptor() noexcept : NetworkDevice(), mAddr({}) {}

void Acceptor::disconnect() {
  destroyHandle();
  mIsBound = false;
}

void Acceptor::setNewPeerHandler(NEW_PEER_HANDLER handler) noexcept {
  mHandleNewPeer = handler;
}

bool Acceptor::isBound() const noexcept { return mIsBound; }

RETURN_CODE Acceptor::bind(PORT port, IPVersion ip_hint) noexcept {
  HostAddr addr;

  addr.port = port;
  auto hint = IPVersion::IPv6;

  if (ip_hint == IPVersion::IPv4) {
    addr.ip = "0.0.0.0";
    hint = IPVersion::IPv4;
  } else {
    addr.ip = "::";
  }

  return bind(addr, hint);
}

RETURN_CODE Acceptor::bind(const HostAddr &host, IPVersion ip_hint) noexcept {
  disconnect();

  if (createAndBindSocket(host, ip_hint, SOCK_STREAM) == RETURN::OK) {
    mIsBound = true;
    mAddr = {host, ip_hint};

    listen();

    return RETURN::OK;
  }

  return RETURN::NOK;
}

RETURN_CODE Acceptor::bind(const ConnectedHost &host) noexcept {
  return bind(host.addr, host.ip_hint);
}

void Acceptor::listen() {
  if (!getDeviceHandle()) {
    logWarn("Acceptor/listen",
            "Listen requested, but no device handle present");
    return;
  }

  auto sock = getDeviceHandle().value();

  if (::listen(sock, std::numeric_limits<int>::max()) == -1) {
    disconnect();
    logError("Acceptor/listen", "Unable to set socket into listen mode: " +
                                    std::string(strerror(errno)));
  }
}

void Acceptor::readyRead() {
  int peer;

  struct sockaddr_storage their_addr;

  socklen_t addr_size = sizeof(their_addr);
  peer = accept(getDeviceHandle().value(),
                reinterpret_cast<struct sockaddr *>(&their_addr), &addr_size);

  char ip[INET6_ADDRSTRLEN];
  auto maxlen = sizeof(ip);
  PORT port;

  switch (reinterpret_cast<struct sockaddr *>(&their_addr)->sa_family) {
  case AF_INET:
    inet_ntop(
        AF_INET,
        &((reinterpret_cast<struct sockaddr_in *>(&their_addr))->sin_addr), ip,
        static_cast<socklen_t>(maxlen));
    port =
        ntohs((reinterpret_cast<struct sockaddr_in *>(&their_addr))->sin_port);
    break;

  case AF_INET6:
    inet_ntop(
        AF_INET6,
        &((reinterpret_cast<struct sockaddr_in6 *>(&their_addr))->sin6_addr),
        ip, static_cast<socklen_t>(maxlen));
    port = ntohs(
        (reinterpret_cast<struct sockaddr_in6 *>(&their_addr))->sin6_port);
    break;

  default:
    strncpy(ip, "Unknown AF", maxlen);
    port = 0;
  }

  HostAddr peerAddr;

  peerAddr.ip = ip;
  peerAddr.port = port;

  auto peer_raw = new Peer(peer, peerAddr);
  auto tcpPeer = std::unique_ptr<Peer>(peer_raw);

  registerChildDevice(tcpPeer.get());

  notifyNewPeer(std::move(tcpPeer));

  listen();
}

void Acceptor::readyHangup() {
  destroyHandle();
  logError("TCPAcceptor",
           "Device has hungup. Unsure how this can happen in an acceptor");
}

void Acceptor::notifyNewPeer(std::unique_ptr<Peer> new_peer) const {
  if (mHandleNewPeer) {
    mHandleNewPeer(std::move(new_peer));
  }
}

void Peer::setRequestHandler(NEW_REQUEST_HANDLER handler) noexcept {
  mRequestHandler = handler;
}

void Peer::setDisconnectHandler(PEER_DISCONNECT_HANDLER handler) noexcept {
  mDisconnectHandler = handler;
}

Peer::Peer(DEVICE_HANDLE_ handle, const HostAddr &peer_addr) noexcept
    : NetworkDevice(), mPeerAddr(peer_addr) {
  registerNewHandle(handle);
  mIsConnected = true;
}

void Peer::readyRead() {
  logDebug("TCPPeer\readyReady", "incoming data");

  NetworkMessage message;

  auto read_resp = readIOData(message.data);

  if (message.data.empty()) {
    logDebug("TCPPeer\readyRead", "Peer closed connection");
    peerDisconnected();
    return;
  }

  if (!std::holds_alternative<ERROR_CODE>(read_resp.code) ||
      std::get<ERROR_CODE>(read_resp.code) != ERROR_CODE::NO_ERROR) {
    logError("TCPPeer/readyRead",
             "Error reading descriptor. " + read_resp.description);
    return;
  }

  message.peer = mPeerAddr;

  notifyServerHandler(message);
}

void Peer::readyHangup() { peerDisconnected(); }

void Peer::readyPeerDisconnect() { peerDisconnected(); }

void Peer::peerDisconnected() {
  mIsConnected = false;

  destroyHandle();

  if (mDisconnectHandler) {
    mDisconnectHandler(this);
  }
}

void Peer::notifyServerHandler(const NetworkMessage &request) {
  notifyCallback(request);

  if (!mRequestHandler) {
    return;
  }

  auto response = mRequestHandler(request);

  if (response->empty()) {
    logDebug("TCPPeer/notifyserverhandler", "No response provided");
    return;
  }

  auto ret = syncSend(response.value());

  if (ret == RETURN::NOK) {
    auto error = getLastError();

    std::string err_desc;

    if (std::holds_alternative<ERROR_CODE>(error.code)) {
      err_desc = "Internal error: ";
    } else {
      auto err_code = std::get<int>(error.code);

      err_desc = "SYS error (code: " + std::to_string(err_code) +
                 " | sys description: " + strerror(err_code) +
                 "). Description: ";
    }

    err_desc += error.description;

    logError("TCPPeer/notifyserverhandler", err_desc);
  }
}

HostAddr Peer::getPeerAddr() const noexcept { return mPeerAddr; }

} // namespace Context::Devices::IO::Networking::TCP::Server
