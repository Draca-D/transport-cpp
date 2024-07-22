#ifndef UDPMULTICASTER_H
#define UDPMULTICASTER_H

#include "networkdevice.h"

namespace Context::Devices::IO::Networking::UDP {


class TRANSPORT_CPP_EXPORT Multicaster final :
        public NetworkDevice
{
public:
    Multicaster();
};
}

#endif // UDPMULTICASTER_H
