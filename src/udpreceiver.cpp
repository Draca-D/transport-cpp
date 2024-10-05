#include <transport-cpp/networking/udpreceiver.h>

#include <sys/socket.h>

namespace Context::Devices::IO::Networking::UDP {
Receiver::Receiver() : NetworkDevice(), mAddr({}) {}

void Receiver::disconnect() noexcept {
  destroyHandle();
  mIsBound = false;
}

bool Receiver::isConnected() const noexcept { return mIsBound; }

ConnectedHost Receiver::getBoundAddr() const noexcept { return mAddr; }

RETURN_CODE Receiver::bind(const HostAddr &host, const IPVersion &ip_hint) {
  disconnect();

  if (createAndBindSocket(host, ip_hint, SOCK_DGRAM) == RETURN::OK) {
    mIsBound = true;
    mAddr = {host, ip_hint};

    return RETURN::OK;
  }

  return RETURN::NOK;
}

RETURN_CODE Receiver::bind(const PORT &port, const IPVersion &ip_hint) {
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

RETURN_CODE Receiver::bind(const ConnectedHost &addr) {
  return bind(addr.addr, addr.ip_hint);
}
} // namespace Context::Devices::IO::Networking::UDP
