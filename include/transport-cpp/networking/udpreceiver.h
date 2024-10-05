#ifndef UDPRECEIVER_H
#define UDPRECEIVER_H

#include "networkdevice.h"

namespace Context::Devices::IO::Networking::UDP {

class TRANSPORT_CPP_EXPORT Receiver final : public NetworkDevice {
private:
  bool mIsBound = false;
  ConnectedHost mAddr;

public:
  Receiver();

  void disconnect() noexcept;

  [[nodiscard]] bool isConnected() const noexcept;
  [[nodiscard]] ConnectedHost getBoundAddr() const noexcept;

  [[nodiscard]] RETURN_CODE bind(const HostAddr &host,
                                 const IPVersion &ip_hint = IPVersion::ANY);
  [[nodiscard]] RETURN_CODE bind(const PORT &port,
                                 const IPVersion &ip_hint = IPVersion::ANY);
  [[nodiscard]] RETURN_CODE bind(const ConnectedHost &addr);
};

} // namespace Context::Devices::IO::Networking::UDP

#endif // UDPRECEIVER_H
