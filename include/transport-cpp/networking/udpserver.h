#ifndef UDPSERVER_H
#define UDPSERVER_H

#include "networkdevice.h"

namespace Context::Devices::IO::Networking::UDP {
    class Server;

    class TRANSPORT_CPP_EXPORT Peer final :
            public NetworkDevice {
        friend class Server;
        using DESTROY_NOTIFIER = std::function<void(Peer *)>;
        using SYNC_SEND = std::function<RETURN_CODE(const HostAddr &, const IODATA_CHOICE &, const IPVersion &)>;
        using ASYNC_SEND_PLAIN = std::function<RETURN_CODE(const HostAddr &, const IODATA &, const IPVersion &)>;
        using ASYNC_SEND_SHARED = std::function<RETURN_CODE(const HostAddr &, const std::shared_ptr<IODATA> &,
                                                            const IPVersion &)>;
        using ASYNC_SEND_UNIQUE = std::function<RETURN_CODE(const HostAddr &, std::unique_ptr<IODATA>,
                                                            const IPVersion &)>;

        using NETWORK_MSG_CALLBACK = std::function<void(const NetworkMessage &message)>;

    private:
        DESTROY_NOTIFIER mNotifyDestruction;
        SYNC_SEND mSyncSend;
        ASYNC_SEND_PLAIN mAsyncSendPlain;
        ASYNC_SEND_SHARED mAsyncSendShared;
        ASYNC_SEND_UNIQUE mAsyncSendUnique;
        NETWORK_MSG_CALLBACK mNewMessage;

        bool mIsValid = false;
        HostAddr mPeerAddr;

    private:
        Peer();

        void invalidate();

        void notifyNewData(const NetworkMessage &message) const noexcept;

    public:
        ~Peer() override;

        void setMessageHandler(const NETWORK_MSG_CALLBACK &handler);

        [[nodiscard]] HostAddr getPeerAddress() const noexcept;
        [[nodiscard]] bool isValid() const noexcept;
        [[nodiscard]] RETURN_CODE sendTo(const HostAddr &dest, const IODATA &message,
                                         const IPVersion &ip_hint = IPVersion::ANY) override;

        [[nodiscard]] RETURN_CODE sendTo(const HostAddr &dest, const std::shared_ptr<IODATA> &message,
                                         const IPVersion &ip_hint = IPVersion::ANY) override;

        [[nodiscard]] RETURN_CODE sendTo(const HostAddr &dest, std::unique_ptr<IODATA> message,
                                         const IPVersion &ip_hint = IPVersion::ANY) override;

        [[nodiscard]] RETURN_CODE syncSendTo(const HostAddr &dest, const IODATA_CHOICE &message,
                                             const IPVersion &ip_hint = IPVersion::ANY) override;

        [[nodiscard]] RETURN_CODE asyncSend(const IODATA &data) override;
        [[nodiscard]] RETURN_CODE asyncSend(const std::shared_ptr<IODATA> &data) override;
        [[nodiscard]] RETURN_CODE asyncSend(std::unique_ptr<IODATA> data) override;

        //message will be moved, original value will be invalidated
        [[nodiscard]] RETURN_CODE syncSend(const IODATA_CHOICE &data) override;
    };

    class TRANSPORT_CPP_EXPORT Server final :
            public NetworkDevice {
        using PEER = std::unique_ptr<Peer>;
        using PEER_LIST = std::vector<Peer *>;

        using NEW_PEER_NOTIFY = std::function<void(const NetworkMessage &, PEER)>;

    private:
        HostAddr mLastPeer;
        ConnectedHost mAddr;
        bool mPeerConnected = false;
        bool mBound = false;
        PEER_LIST mPeers;
        NEW_PEER_NOTIFY mNewPeerNotify;

    public:
        Server();

        ~Server() override;

        void disconnect() noexcept;

        void setNewPeerHandler(const NEW_PEER_NOTIFY &handler);

        [[nodiscard]] RETURN_CODE bind(const HostAddr &host, const IPVersion &ip_hint = IPVersion::ANY);
        [[nodiscard]] RETURN_CODE bind(const PORT &port, const IPVersion &ip_hint = IPVersion::ANY);
        [[nodiscard]] RETURN_CODE bind(const ConnectedHost &addr);
        [[nodiscard]] RETURN_CODE asyncSend(const IODATA &data) override;
        [[nodiscard]] RETURN_CODE asyncSend(const std::shared_ptr<IODATA> &data) override;
        [[nodiscard]] RETURN_CODE asyncSend(std::unique_ptr<IODATA> data) override;
        [[nodiscard]] RETURN_CODE syncSend(const IODATA_CHOICE &data) override;

    private:
        void peerDestroyed(const Peer *peer);

        void readyRead() override;
    };
}

#endif // UDPSERVER_H
