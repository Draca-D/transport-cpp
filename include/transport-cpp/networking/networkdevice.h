#ifndef NETWORKDEVICE_H
#define NETWORKDEVICE_H

#include "../iodevice.h"

namespace Context::Devices::IO::Networking {

enum class IPVersion{
    ANY,
    IPv4,
    IPv6
};

class TRANSPORT_CPP_EXPORT NetworkDevice :
        public IODevice
{
protected:
    NetworkDevice();
};
}

#endif // NETWORKDEVICE_H
