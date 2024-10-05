#include <transport-cpp/networking/udpclient.h>

#include <sys/socket.h>

namespace Context::Devices::IO::Networking::UDP {
Client::Client() : NetworkDevice(), mHost({}) {}

void Client::disconnect() noexcept {
  mIsConnected = false;
  destroyHandle();
}

bool Client::isConnected() const noexcept { return mIsConnected; }

ConnectedHost Client::getSetHostAddr() const noexcept { return mHost; }

RETURN_CODE Client::connectToHost(const HostAddr &host,
                                  const IPVersion &ip_hint) {
  disconnect();

  if (createAndConnectSocket(host, ip_hint, SOCK_DGRAM) == RETURN::OK) {
    mIsConnected = true;
    mHost = {host, ip_hint};

    return RETURN::OK;
  }

  return RETURN::NOK;
}

IODevice::SYNC_RX_DATA Client::syncRequestResponse(const IODATA &data) {
  SYNC_RX_DATA resp = {RETURN::NOK, {}};

  if (syncSend(data) == RETURN::NOK) {
    return resp;
  }

  resp = syncReceive();

  return resp;
}

IODevice::SYNC_RX_DATA
Client::syncRequestResponse(const IODATA &data,
                            const std::chrono::milliseconds &timeout) {
  SYNC_RX_DATA resp = {RETURN::NOK, {}};

  if (syncSend(data) == RETURN::NOK) {
    return resp;
  }

  resp = syncReceive(timeout);

  return resp;
}

void Client::readyError() {
  if (connectToHost(mHost.addr, mHost.ip_hint) == RETURN::NOK) {
    logLastError("UDPClient::readyError");
  }
}
} // namespace Context::Devices::IO::Networking::UDP
