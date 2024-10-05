#include <arpa/inet.h>
#include <cstring>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <transport-cpp/networking/udpmulticaster.h>

namespace Multicasting {
namespace IPv4 {
[[maybe_unused]] static constexpr uint32_t NetmaskHostOrder = 0xF0000000;
[[maybe_unused]] static constexpr uint32_t NetworkHostOrder = 0xE0000000;

static constexpr uint32_t NetmaskNetOrder = 0x000000F0;
static constexpr uint32_t NetworkNetOrder = 0x000000E0;
} // namespace IPv4

namespace IPv6 {
static constexpr auto MajorByteNetwork = 0xFF;
static constexpr auto MajorByteNetmask = 0xFF;
} // namespace IPv6
} // namespace Multicasting

namespace Context::Devices::IO::Networking::UDP {
Multicaster::Multicaster()
    : NetworkDevice(), mSetInterface({}), mIpVersion(IPVersion::ANY),
      mPublishedSockAddrLen(0) {}

Multicaster::~Multicaster() {
  if (mPublishedSockAddr != nullptr) {
    free(mPublishedSockAddr);
    mPublishedSockAddr = nullptr;
  }
}

void Multicaster::deInitialise() {
  mInitalised = false;
  destroyHandle();
}

RETURN_CODE Multicaster::initialise(const IPVersion &ip_version) {
  deInitialise();
  int domain;

  if (ip_version == IPVersion::IPv4) {
    domain = AF_INET;
    mIpVersion = ip_version;
  } else if (ip_version == IPVersion::IPv6) {
    domain = AF_INET6;
    mIpVersion = ip_version;
  } else {
    setError(ERROR_CODE::INVALID_LOGIC, "IPversion cannot be 'Any'");
    return RETURN::NOK;
  }

  const auto sock = socket(domain, SOCK_DGRAM, IPPROTO_UDP);

  if (sock == -1) {
    setError(errno, "Unable to create socket");
    return RETURN::NOK;
  }

  registerNewHandle(sock);

  mInitalised = true;

  return RETURN::OK;
}

RETURN_CODE Multicaster::publishToGroup(const HostAddr &group) {
  if (!mInitalised) {
    setError(ERROR_CODE::INVALID_LOGIC,
             "Multicaster has not been initialised yet");
    return RETURN::NOK;
  }

  if (mPublishedSockAddr != nullptr) {
    free(mPublishedSockAddr);
    mPublishedSockAddr = nullptr;
  }

  int domain;
  size_t addr_len;

  if (mIpVersion == IPVersion::IPv4) {
    domain = AF_INET;
    addr_len = sizeof(in_addr);
  } else if (mIpVersion == IPVersion::IPv6) {
    domain = AF_INET6;
    addr_len = sizeof(in6_addr);
  } else {
    setError(ERROR_CODE::GENERAL_ERROR,
             "Multicaster was somehow initialised as 'Any'");
    return RETURN::NOK;
  }

  const auto output = malloc(addr_len);

  if (const auto res = inet_pton(domain, group.ip.c_str(), output); res == 0) {
    setError(ERROR_CODE::INVALID_ARGUMENT, "Provided address is invalid");
    free(output);
    return RETURN::NOK;
  } else if (res == -1) {
    setError(errno, "inet_pton returned an error");
    free(output);
    return RETURN::NOK;
  }

  if (mIpVersion == IPVersion::IPv4) {
    const uint32_t address = static_cast<in_addr *>(output)->s_addr;

    if ((address & Multicasting::IPv4::NetmaskNetOrder) !=
        Multicasting::IPv4::NetworkNetOrder) {
      setError(ERROR_CODE::INVALID_ARGUMENT,
               "Provided address is not a multicast address");
      free(output);
      return RETURN::NOK;
    }

    const auto local_addr =
        static_cast<sockaddr_in *>(malloc(sizeof(sockaddr_in)));

    memset(local_addr, 0, sizeof(sockaddr_in));

    local_addr->sin_family = AF_INET;
    local_addr->sin_port = htons(group.port);
    local_addr->sin_addr = *static_cast<in_addr *>(output);

    mPublishedSockAddr = reinterpret_cast<sockaddr *>(local_addr);
    mPublishedSockAddrLen = sizeof(sockaddr_in);
  } else {
    const auto address = static_cast<in6_addr *>(output)->s6_addr;

    if ((address[0] & Multicasting::IPv6::MajorByteNetmask) !=
        Multicasting::IPv6::MajorByteNetwork) {
      setError(ERROR_CODE::INVALID_ARGUMENT,
               "Provided address is not a multicast address");
      free(output);
      return RETURN::NOK;
    }

    const auto local_addr =
        static_cast<sockaddr_in6 *>(malloc(sizeof(sockaddr_in6)));

    memset(local_addr, 0, sizeof(sockaddr_in6));

    local_addr->sin6_family = AF_INET6;
    local_addr->sin6_port = htons(group.port);
    local_addr->sin6_addr = *static_cast<in6_addr *>(output);

    mPublishedSockAddr = reinterpret_cast<sockaddr *>(local_addr);
    mPublishedSockAddrLen = sizeof(sockaddr_in6);
  }

  free(output);
  mPublishedAddr = group;
  return RETURN::OK;
}

RETURN_CODE Multicaster::subscribeToGroup(const HostAddr &group) {
  if (!mInitalised) {
    setError(ERROR_CODE::INVALID_LOGIC,
             "Multicaster has not been initialised yet");
    return RETURN::NOK;
  }

  if (mSetInterface.if_name.empty()) {
    setError(ERROR_CODE::INVALID_LOGIC, "Interface has not been set");
    return RETURN::NOK;
  }

  int domain;
  size_t addr_len;

  if (mIpVersion == IPVersion::IPv4) {
    domain = AF_INET;
    addr_len = sizeof(in_addr);
  } else if (mIpVersion == IPVersion::IPv6) {
    domain = AF_INET6;
    addr_len = sizeof(in6_addr);
  } else {
    setError(ERROR_CODE::GENERAL_ERROR,
             "Multicaster was somehow initialised as 'Any'");
    return RETURN::NOK;
  }

  const auto output = malloc(addr_len);

  if (const auto res = inet_pton(domain, group.ip.c_str(), output); res == 0) {
    setError(ERROR_CODE::INVALID_ARGUMENT, "Provided address is invalid");
    free(output);
    return RETURN::NOK;
  } else if (res == -1) {
    setError(errno, "inet_pton returned an error");
    free(output);
    return RETURN::NOK;
  }

  sockaddr *sub_addr;
  size_t sub_addr_len;

  if (mIpVersion == IPVersion::IPv4) {
    const uint32_t address = static_cast<in_addr *>(output)->s_addr;

    if ((address & Multicasting::IPv4::NetmaskNetOrder) !=
        Multicasting::IPv4::NetworkNetOrder) {
      setError(ERROR_CODE::INVALID_ARGUMENT,
               "Provided address is not a multicast address");
      free(output);
      return RETURN::NOK;
    }

    const auto local_addr =
        static_cast<sockaddr_in *>(malloc(sizeof(sockaddr_in)));

    memset(local_addr, 0, sizeof(sockaddr_in));

    local_addr->sin_family = AF_INET;
    local_addr->sin_port = htons(group.port);
    local_addr->sin_addr = *static_cast<in_addr *>(output);

    sub_addr = reinterpret_cast<sockaddr *>(local_addr);
    sub_addr_len = sizeof(sockaddr_in);
  } else {
    const auto address = static_cast<in6_addr *>(output)->s6_addr;

    if ((address[0] & Multicasting::IPv6::MajorByteNetmask) !=
        Multicasting::IPv6::MajorByteNetwork) {
      setError(ERROR_CODE::INVALID_ARGUMENT,
               "Provided address is not a multicast address");
      free(output);
      return RETURN::NOK;
    }

    const auto local_addr =
        static_cast<sockaddr_in6 *>(malloc(sizeof(sockaddr_in6)));

    memset(local_addr, 0, sizeof(sockaddr_in6));

    local_addr->sin6_family = AF_INET6;
    local_addr->sin6_port = htons(group.port);
    local_addr->sin6_addr = *static_cast<in6_addr *>(output);

    sub_addr = reinterpret_cast<sockaddr *>(local_addr);
    sub_addr_len = sizeof(sockaddr_in6);
  }

  void *mcast_registration;
  size_t mcast_size;

  int sock_opt_level;
  int sock_opt_name;

  if (mIpVersion == IPVersion::IPv4) {
    mcast_size = sizeof(ip_mreq);

    const auto local_addr = static_cast<ip_mreq *>(malloc(mcast_size));
    local_addr->imr_multiaddr =
        reinterpret_cast<sockaddr_in *>(sub_addr)->sin_addr;
    inet_pton(AF_INET, mSetInterface.if_addr.c_str(),
              &local_addr->imr_interface);

    mcast_registration = local_addr;

    sock_opt_level = IPPROTO_IP;
    sock_opt_name = IP_ADD_MEMBERSHIP;
  } else {
    mcast_size = sizeof(ipv6_mreq);
    const auto local_addr = static_cast<ipv6_mreq *>(malloc(mcast_size));
    local_addr->ipv6mr_multiaddr =
        reinterpret_cast<sockaddr_in6 *>(sub_addr)->sin6_addr;
    local_addr->ipv6mr_interface =
        if_nametoindex(mSetInterface.if_name.c_str());

    mcast_registration = local_addr;

    sock_opt_level = IPPROTO_IPV6;
    sock_opt_name = IPV6_ADD_MEMBERSHIP;
  }

  if (setsockopt(getDeviceHandle().value(), sock_opt_level, sock_opt_name,
                 mcast_registration, mcast_size) == -1) {
    setError(errno, "Unable to register to address");

    free(sub_addr);
    free(output);
    free(mcast_registration);

    return RETURN::NOK;
  }

  if (::bind(getDeviceHandle().value(), sub_addr, sub_addr_len) == -1) {
    setError(errno, "Unable to bind address");

    free(sub_addr);
    free(output);
    free(mcast_registration);

    return RETURN::NOK;
  }

  free(output);
  free(sub_addr);
  free(mcast_registration);

  mSubscribedAddr = group;

  return RETURN::OK;
}

RETURN_CODE Multicaster::setInterface(const std::string &iface_name) {
  mSetInterface = {};

  if (!mInitalised) {
    setError(ERROR_CODE::INVALID_LOGIC,
             "Multicaster has not been initialised yet");
    return RETURN::NOK;
  }

  auto interfaces = getAllInterfaces();

  const auto ipv4_iface =
      std::find_if(interfaces.begin(), interfaces.end(),
                   [&iface_name, this](const IFACE &iface) {
                     return (iface_name == iface.if_name) &&
                            (iface.ip_version == IPVersion::IPv4);
                   });

  const auto ipv6_iface =
      std::find_if(interfaces.begin(), interfaces.end(),
                   [&iface_name, this](const IFACE &iface) {
                     return (iface_name == iface.if_name) &&
                            (iface.ip_version == IPVersion::IPv6);
                   });

  if (ipv4_iface == interfaces.end() && ipv6_iface == interfaces.end()) {
    setError(ERROR_CODE::INVALID_ARGUMENT, "Provided interface does not exist");
    return RETURN::NOK;
  }

  if (mIpVersion == IPVersion::IPv4 && ipv4_iface == interfaces.end() &&
      ipv6_iface != interfaces.end()) {
    setError(ERROR_CODE::INVALID_LOGIC,
             "Multicaster was initialised as ipv4 but provided interface "
             "only supports ipv6");
    return RETURN::NOK;
  }

  if (mIpVersion == IPVersion::IPv6 && ipv6_iface == interfaces.end() &&
      ipv4_iface != interfaces.end()) {
    setError(ERROR_CODE::INVALID_LOGIC,
             "Multicaster was initialised as ipv6 but provided interface "
             "only supports ipv4");
    return RETURN::NOK;
  }

  const auto socket = getDeviceHandle().value();

  size_t addr_len;
  int domain;
  std::string addr_str;
  int sock_opt_level;
  int sock_opt;

  if (mIpVersion == IPVersion::IPv4) {
    addr_len = sizeof(in_addr);
    domain = AF_INET;
    addr_str = ipv4_iface->if_addr;
    sock_opt_level = IPPROTO_IP;
    sock_opt = IP_MULTICAST_IF;
  } else {
    addr_len = sizeof(unsigned int);
    domain = AF_INET6;
    sock_opt_level = IPPROTO_IPV6;
    sock_opt = IPV6_MULTICAST_IF;
  }

  const auto output = malloc(addr_len);

  if (mIpVersion == IPVersion::IPv4) {
    if (const auto res = inet_pton(domain, addr_str.c_str(), output);
        res == 0) {
      setError(ERROR_CODE::INVALID_ARGUMENT, "Provided address is invalid");
      free(output);
      return RETURN::NOK;
    } else if (res == -1) {
      setError(errno, "inet_pton returned an error");
      free(output);
      return RETURN::NOK;
    }
  } else {
    *static_cast<unsigned int *>(output) = if_nametoindex(iface_name.c_str());
  }

  if (setsockopt(socket, sock_opt_level, sock_opt, output, addr_len) == -1) {
    setError(errno, "Unable to add interface to multicaster");
    free(output);
    return RETURN::NOK;
  }

  if (mIpVersion == IPVersion::IPv4) {
    mSetInterface = *ipv4_iface;
  } else {
    mSetInterface = *ipv6_iface;
  }

  free(output);
  return RETURN::OK;
}

RETURN_CODE Multicaster::setInterface(const IFACE &iface) {
  return setInterface(iface.if_name);
}

RETURN_CODE Multicaster::setLoopback(const bool &enable) {
  if (!mInitalised) {
    setError(ERROR_CODE::INVALID_LOGIC, "Device has not been initialised yet");
    return RETURN::NOK;
  }

  const int loop = enable;

  if (setsockopt(getDeviceHandle().value(), IPPROTO_IP, IP_MULTICAST_LOOP,
                 &loop, sizeof(loop)) == -1) {
    setError(errno, "Unable to set multicast loopback");
    return RETURN::NOK;
  }

  return RETURN::OK;
}

void Multicaster::readyWrite() {
  if (IODevice::mOutgoingQueue.empty()) {
    NetworkDevice::readyWrite();
    return;
  }

  const auto data = &IODevice::mOutgoingQueue.front();

  const IODATA *data_ptr;

  if (std::holds_alternative<IODATA>(*data)) {
    data_ptr = &std::get<IODATA>(*data);
  } else if (std::holds_alternative<std::shared_ptr<IODATA>>(*data)) {
    data_ptr = std::get<std::shared_ptr<IODATA>>(*data).get();
  } else {
    data_ptr = std::get<std::unique_ptr<IODATA>>(*data).get();
  }

  if (const auto nWrote =
          sendto(getDeviceHandle().value(), data_ptr->data(), data_ptr->size(),
                 0, mPublishedSockAddr, mPublishedSockAddrLen);
      nWrote < 0) {
    logError("Multicaster/readyWrite",
             "Unable to perform sendTo: " + std::string(strerror(errno)));
  }

  IODevice::mOutgoingQueue.pop();

  requestWrite();
}

bool Multicaster::deviceIsReady() const {
  return mPublishedSockAddr != nullptr && mInitalised;
}
} // namespace Context::Devices::IO::Networking::UDP
