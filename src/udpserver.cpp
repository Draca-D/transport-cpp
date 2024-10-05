#include <sys/socket.h>
#include <transport-cpp/networking/udpserver.h>

namespace Context::Devices::IO::Networking::UDP {
Peer::Peer() : NetworkDevice() {}

void Peer::invalidate() {
  mNotifyDestruction = nullptr;
  mSyncSend = nullptr;
  mAsyncSendPlain = nullptr;
  mAsyncSendShared = nullptr;
  mAsyncSendUnique = nullptr;

  mIsValid = false;
}

void Peer::notifyNewData(const NetworkMessage &message) const noexcept {
  if (mNewMessage) {
    mNewMessage(message);
  }
}

Peer::~Peer() {
  invalidate();

  if (mNotifyDestruction) {
    mNotifyDestruction(this);
  }
}

void Peer::setMessageHandler(const NETWORK_MSG_CALLBACK &handler) {
  mNewMessage = handler;
}

HostAddr Peer::getPeerAddress() const noexcept { return mPeerAddr; }

bool Peer::isValid() const noexcept { return mIsValid; }

RETURN_CODE Peer::sendTo(const HostAddr &dest, const IODATA &message,
                         const IPVersion &ip_hint) {
  if (mAsyncSendPlain) {
    return mAsyncSendPlain(dest, message, ip_hint);
  }

  setError(ERROR_CODE::DEVICE_NOT_READY,
           "This peer requires a valid server instance");
  return RETURN::NOK;
}

RETURN_CODE Peer::sendTo(const HostAddr &dest,
                         const std::shared_ptr<IODATA> &message,
                         const IPVersion &ip_hint) {
  if (mAsyncSendShared) {
    return mAsyncSendShared(dest, message, ip_hint);
  }

  setError(ERROR_CODE::DEVICE_NOT_READY,
           "This peer requires a valid server instance");
  return RETURN::NOK;
}

RETURN_CODE Peer::sendTo(const HostAddr &dest, std::unique_ptr<IODATA> message,
                         const IPVersion &ip_hint) {
  if (mAsyncSendUnique) {
    return mAsyncSendUnique(dest, std::move(message), ip_hint);
  }

  setError(ERROR_CODE::DEVICE_NOT_READY,
           "This peer requires a valid server instance");
  return RETURN::NOK;
}

RETURN_CODE Peer::syncSendTo(const HostAddr &dest, const IODATA_CHOICE &message,
                             const IPVersion &ip_hint) {
  if (mSyncSend) {
    return mSyncSend(dest, message, ip_hint);
  }

  setError(ERROR_CODE::DEVICE_NOT_READY,
           "This peer requires a valid server instance");
  return RETURN::NOK;
}

RETURN_CODE Peer::asyncSend(const IODATA &data) {
  if (mAsyncSendPlain) {
    return mAsyncSendPlain(mPeerAddr, data, IPVersion::ANY);
  }

  setError(ERROR_CODE::DEVICE_NOT_READY,
           "This peer requires a valid server instance");
  return RETURN::NOK;
}

RETURN_CODE Peer::asyncSend(const std::shared_ptr<IODATA> &data) {
  if (mAsyncSendShared) {
    return mAsyncSendShared(mPeerAddr, data, IPVersion::ANY);
  }

  setError(ERROR_CODE::DEVICE_NOT_READY,
           "This peer requires a valid server instance");
  return RETURN::NOK;
}

RETURN_CODE Peer::asyncSend(std::unique_ptr<IODATA> data) {
  if (mAsyncSendUnique) {
    return mAsyncSendUnique(mPeerAddr, std::move(data), IPVersion::ANY);
  }

  setError(ERROR_CODE::DEVICE_NOT_READY,
           "This peer requires a valid server instance");
  return RETURN::NOK;
}

RETURN_CODE Peer::syncSend(const IODATA_CHOICE &data) {
  if (mSyncSend) {
    return mSyncSend(mPeerAddr, data, IPVersion::ANY);
  }

  setError(ERROR_CODE::DEVICE_NOT_READY,
           "This peer requires a valid server instance");
  return RETURN::NOK;
}

Server::Server() : NetworkDevice(), mLastPeer({}), mAddr({}) {}

Server::~Server() { disconnect(); }

void Server::disconnect() noexcept {
  destroyHandle();
  mBound = false;

  for (auto &peer : mPeers) {
    peer->invalidate();
  }
}

void Server::setNewPeerHandler(const NEW_PEER_NOTIFY &handler) {
  mNewPeerNotify = handler;
}

RETURN_CODE Server::bind(const HostAddr &host, const IPVersion &ip_hint) {
  disconnect();

  if (createAndBindSocket(host, ip_hint, SOCK_DGRAM) == RETURN::OK) {
    mBound = true;
    mAddr = {host, ip_hint};

    return RETURN::OK;
  }

  return RETURN::NOK;
}

RETURN_CODE Server::bind(const PORT &port, const IPVersion &ip_hint) {
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

RETURN_CODE Server::bind(const ConnectedHost &addr) {
  return bind(addr.addr, addr.ip_hint);
}

RETURN_CODE Server::asyncSend(const IODATA &data) {
  if (!mPeerConnected) {
    setError(ERROR_CODE::DEVICE_NOT_READY,
             "We need to first receive a message from a peer before we can "
             "send messages");
    return RETURN::NOK;
  }

  return sendTo(mLastPeer, data, IPVersion::ANY);
}

RETURN_CODE Server::asyncSend(const std::shared_ptr<IODATA> &data) {
  if (!mPeerConnected) {
    setError(ERROR_CODE::DEVICE_NOT_READY,
             "We need to first receive a message from a peer before we can "
             "send messages");
    return RETURN::NOK;
  }

  return sendTo(mLastPeer, data, IPVersion::ANY);
}

RETURN_CODE Server::asyncSend(std::unique_ptr<IODATA> data) {
  if (!mPeerConnected) {
    setError(ERROR_CODE::DEVICE_NOT_READY,
             "We need to first receive a message from a peer before we can "
             "send messages");
    return RETURN::NOK;
  }

  return sendTo(mLastPeer, std::move(data), IPVersion::ANY);
}

RETURN_CODE Server::syncSend(const IODATA_CHOICE &data) {
  if (!mPeerConnected) {
    setError(ERROR_CODE::DEVICE_NOT_READY,
             "We need to first receive a message from a peer before we can "
             "send messages");
    return RETURN::NOK;
  }

  return syncSendTo(mLastPeer, data, IPVersion::ANY);
}

void Server::peerDestroyed(const Peer *peer) {
  auto peer_it = std::find(mPeers.begin(), mPeers.end(), peer);

  if (peer_it != mPeers.end()) {
    mPeers.erase(peer_it);
  }
}

void Server::readyRead() {
  NetworkMessage data;

  auto read_resp = receiveMessage(data);

  if (!std::holds_alternative<ERROR_CODE>(read_resp.code) ||
      std::get<ERROR_CODE>(read_resp.code) != ERROR_CODE::NO_ERROR) {
    logError("UDPServer/readyRead",
             "Error reading descriptor. " + read_resp.description);
    return;
  }

  notifyCallback(data);

  auto relevant_peer =
      std::find_if(mPeers.begin(), mPeers.end(), [&data](const Peer *pr) {
        return pr->getPeerAddress().ip == data.peer.ip &&
               pr->getPeerAddress().port == data.peer.port;
      });

  mLastPeer = data.peer;
  mPeerConnected = true;

  if (relevant_peer == mPeers.end()) {
    // new peer
    printf("new peer\n");

    if (!mNewPeerNotify) {
      return;
    }

    const auto new_peer_raw = new Peer();
    auto new_peer = PEER(new_peer_raw);

    using namespace std::placeholders;

    new_peer->mPeerAddr = data.peer;
    new_peer->mIsValid = true;

    new_peer->mAsyncSendPlain =
        [this](const HostAddr &dest, const IODATA &message,
               const IPVersion &ip_hint) -> RETURN_CODE {
      return sendTo(dest, message, ip_hint);
    };

    new_peer->mAsyncSendShared =
        [this](const HostAddr &dest, const std::shared_ptr<IODATA> &message,
               const IPVersion &ip_hint) -> RETURN_CODE {
      return sendTo(dest, message, ip_hint);
    };

    new_peer->mAsyncSendUnique =
        [this](const HostAddr &dest, std::unique_ptr<IODATA> message,
               const IPVersion &ip_hint) -> RETURN_CODE {
      return sendTo(dest, std::move(message), ip_hint);
    };

    new_peer->mSyncSend = [this](const HostAddr &dest,
                                 const IODATA_CHOICE &message,
                                 const IPVersion &ip_hint) -> RETURN_CODE {
      return syncSendTo(dest, message, ip_hint);
    };

    mNewPeerNotify(data, std::move(new_peer));
    mPeers.push_back(new_peer_raw);
  } else {
    (*relevant_peer)->notifyNewData(data);
  }
}
} // namespace Context::Devices::IO::Networking::UDP
