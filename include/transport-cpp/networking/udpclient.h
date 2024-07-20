#ifndef UDPCLIENT_H
#define UDPCLIENT_H

#include "networkdevice.h"

namespace Context::Devices::IO::Networking::UDP {

class Client :
        public NetworkDevice
{
public:
    Client();
};

}

#endif // UDPCLIENT_H
