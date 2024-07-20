#ifndef UDPRECEIVER_H
#define UDPRECEIVER_H

#include "networkdevice.h"

namespace Context::Devices::IO::Networking::UDP {


class Receiver :
        public NetworkDevice
{
public:
    Receiver();
};

}

#endif // UDPRECEIVER_H
