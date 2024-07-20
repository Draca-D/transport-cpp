#ifndef UDPSERVER_H
#define UDPSERVER_H

#include "networkdevice.h"

namespace Context::Devices::IO::Networking::UDP {

class Server :
        public NetworkDevice
{
public:
    Server();
};

}

#endif // UDPSERVER_H
