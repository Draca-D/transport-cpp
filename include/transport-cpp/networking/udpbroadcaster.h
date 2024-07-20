#ifndef UDPBROADCASTER_H
#define UDPBROADCASTER_H

#include "networkdevice.h"

namespace Context::Devices::IO::Networking::UDP {

class Broadcaster :
        public NetworkDevice
{
public:
    Broadcaster();
};

}

#endif // UDPBROADCASTER_H
