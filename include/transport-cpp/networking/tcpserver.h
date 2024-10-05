#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "networkdevice.h"

namespace Context::Devices::IO::Networking::TCP::Server {

class Acceptor;

class TRANSPORT_CPP_EXPORT Peer : public NetworkDevice {
  friend class Acceptor;

  using NEW_REQUEST_HANDLER =
      std::function<std::optional<IODATA>(const NetworkMessage &)>;
  using PEER_DISCONNECT_HANDLER = std::function<void(Peer *disconnected_peer)>;

private:
  NEW_REQUEST_HANDLER mRequestHandler;
  PEER_DISCONNECT_HANDLER mDisconnectHandler;
  const HostAddr mPeerAddr;
  bool mIsConnected = false;

public:
  void setRequestHandler(NEW_REQUEST_HANDLER handler) noexcept;
  void setDisconnectHandler(PEER_DISCONNECT_HANDLER handler) noexcept;
  HostAddr getPeerAddr() const noexcept;

private:
  Peer(DEVICE_HANDLE_ handle, const HostAddr &peer_addr) noexcept;

  void readyRead() override;
  void readyHangup() override;
  void readyPeerDisconnect() override;

  void peerDisconnected();
  void notifyServerHandler(const NetworkMessage &request);
};

class TRANSPORT_CPP_EXPORT Acceptor final : public NetworkDevice {
  using NEW_PEER_HANDLER = std::function<void(std::unique_ptr<Peer> new_peer)>;
  using DEVICE_HANDLE = std::optional<DEVICE_HANDLE_>;

public:
  struct ConnectedHost {
    HostAddr addr;
    IPVersion ip_hint;
  };

private:
  ConnectedHost mAddr;
  NEW_PEER_HANDLER mHandleNewPeer;
  bool mIsBound = false;

public:
  Acceptor() noexcept;
  void disconnect();

  void setNewPeerHandler(NEW_PEER_HANDLER handler) noexcept;
  [[nodiscard]] bool isBound() const noexcept;

  [[nodiscard]] RETURN_CODE bind(PORT port,
                                 IPVersion ip_hint = IPVersion::ANY) noexcept;
  [[nodiscard]] RETURN_CODE bind(const HostAddr &host,
                                 IPVersion ip_hint = IPVersion::ANY) noexcept;
  [[nodiscard]] RETURN_CODE bind(const ConnectedHost &host) noexcept;

private:
  void listen();

  void readyRead() override;
  void readyHangup() override;

  void notifyNewPeer(std::unique_ptr<Peer> new_peer) const;
};

} // namespace Context::Devices::IO::Networking::TCP::Server

#endif // TCPSERVER_H
