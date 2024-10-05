#ifndef SERIAL_H
#define SERIAL_H

#include "../iodevice.h"

namespace Context::Devices::IO {

class TRANSPORT_CPP_EXPORT Serial : public IODevice {
public:
  enum class Bits { B5, B6, B7, B8 };

  struct Settings { // with default settings
    int32_t baud = 9600;

    // C-Flags (Contol modes)
    bool enableParity = false;
    bool parityEven = true;
    bool use2StopBits = false;
    bool flowControl = false;
    bool hangUp = false;
    bool cRead = true;
    bool cLocal = true;
    Bits bitsPerByte = Bits::B8;

    // I-Flags (Local Modes)
    bool cannonMode = false;
    bool iSig = false;
    bool echo = false;
    bool erasure = false;
    bool newLineEcho = false;

    // i-Flags (Input modes)
    bool swFlowControl = false;
    bool specialHandle = false;

    // O-Flags (Output modes)
    bool NLCR = false;
    bool outInterpret = false;
  };

  struct Device {
    std::string path;
    Settings settings;
  };

#if __GNUC__ > 8
  std::vector<Device> getSystemSerialDevices();
#endif

private:
  bool mIsConnected;

public:
  Serial();

  bool isConnected();

  [[nodiscard]] RETURN_CODE openDevice(const Serial::Device &device);
  void disconnect();
};

} // namespace Context::Devices::IO

#endif // SERIAL_H
