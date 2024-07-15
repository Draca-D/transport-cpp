#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "networkdevice.h"

namespace Context::Devices::IO::TCP {

class Client :
        public NetworkDevice
{
public:
    Client();
};

}

#endif // TCPCLIENT_H
