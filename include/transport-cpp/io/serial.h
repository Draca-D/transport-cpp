#ifndef SERIAL_H
#define SERIAL_H

#include "../iodevice.h"

namespace Context::Devices::IO {

class TRANSPORT_CPP_EXPORT Serial :
        public IODevice
{
public:
    Serial();
};

}

#endif // SERIAL_H
