#ifndef UDPBROADCASTER_H
#define UDPBROADCASTER_H

#include "networkdevice.h"

namespace Context::Devices::IO::Networking::UDP {

class TRANSPORT_CPP_EXPORT Broadcaster final : public NetworkDevice {
public:
  Broadcaster();
};

} // namespace Context::Devices::IO::Networking::UDP

#endif // UDPBROADCASTER_H
