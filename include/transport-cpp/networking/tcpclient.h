#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "networkdevice.h"

namespace Context::Devices::IO::Networking::TCP {

class TRANSPORT_CPP_EXPORT Client final : public NetworkDevice {
  using DISCONNECT_NOTIFY = std::function<void(Client *)>;

private:
  ConnectedHost mHost;
  DISCONNECT_NOTIFY mToNotify;
  bool mIsConnected = false;

public:
  Client();

  ConnectedHost getSetHostAddr();
  void disconnect();

  [[nodiscard]] RETURN_CODE
  connectToHost(const HostAddr &host,
                const IPVersion &ip_hint = IPVersion::ANY);
  [[nodiscard]] RETURN_CODE connectToHost(const ConnectedHost &host);

  void setDisconnectNotification(const DISCONNECT_NOTIFY &handler);

  [[nodiscard]] SYNC_RX_DATA syncRequestResponse(const IODATA &data);
  [[nodiscard]] SYNC_RX_DATA
  syncRequestResponse(const IODATA &data,
                      const std::chrono::milliseconds &timeout);

private:
  void readyRead() override;
  void readyHangup() override;
  void readyPeerDisconnect() override;

  void notifyOfDisconnect();
  void peerDisconnected();
};

} // namespace Context::Devices::IO::Networking::TCP

#endif // TCPCLIENT_H
