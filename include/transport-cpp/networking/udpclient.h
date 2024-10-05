#ifndef UDPCLIENT_H
#define UDPCLIENT_H

#include "networkdevice.h"

namespace Context::Devices::IO::Networking::UDP {

class TRANSPORT_CPP_EXPORT Client final : public NetworkDevice {
private:
  bool mIsConnected = false;
  ConnectedHost mHost;

public:
  Client();

  void disconnect() noexcept;
  [[nodiscard]] bool isConnected() const noexcept;
  [[nodiscard]] ConnectedHost getSetHostAddr() const noexcept;

  [[nodiscard]] RETURN_CODE
  connectToHost(const HostAddr &host,
                const IPVersion &ip_hint = IPVersion::ANY);

  [[nodiscard]] SYNC_RX_DATA syncRequestResponse(const IODATA &data);
  [[nodiscard]] SYNC_RX_DATA
  syncRequestResponse(const IODATA &data,
                      const std::chrono::milliseconds &timeout);

private:
  void readyError() override;
};

} // namespace Context::Devices::IO::Networking::UDP

#endif // UDPCLIENT_H
