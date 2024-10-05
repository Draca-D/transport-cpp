#include <cstring>
#include <transport-cpp/io/serial.h>

#include <fcntl.h>
#include <filesystem>
#include <termios.h>
#include <unistd.h>

speed_t num2Baud(int32_t num) {
  switch (num) {
  case 0:
    return B0;
  case 50:
    return B50;
  case 75:
    return B75;
  case 110:
    return B110;
  case 134:
    return B134;
  case 150:
    return B150;
  case 200:
    return B200;
  case 300:
    return B300;
  case 600:
    return B600;
  case 1200:
    return B1200;
  case 1800:
    return B1800;
  case 2400:
    return B2400;
  case 4800:
    return B4800;
  case 9600:
    return B9600;
  case 19200:
    return B19200;
  case 38400:
    return B38400;
  case 57600:
    return B57600;
  case 115200:
    return B115200;
  case 230400:
    return B230400;
  case 460800:
    return B460800;
  case 500000:
    return B500000;
  case 576000:
    return B576000;
  case 921600:
    return B921600;
  case 1000000:
    return B1000000;
  case 1152000:
    return B1152000;
  case 1500000:
    return B1500000;
  case 2000000:
    return B2000000;
  case 2500000:
    return B2500000;
  case 3000000:
    return B3000000;
  case 3500000:
    return B3500000;
  case 4000000:
    return B4000000;
  }

  return 0;
}

namespace Context::Devices::IO {

#if __GNUC__ >= 8
std::optional<Context::Devices::IO::Serial::Settings>
getPortSettings(const std::string &path);

std::vector<Context::Devices::IO::Serial::Device>
Context::Devices::IO::Serial::getSystemSerialDevices() {
  static const std::string path = "/dev/serial/by-path";

  std::vector<Context::Devices::IO::Serial::Device> devices;

  if (!std::filesystem::exists(path)) {
    return devices;
  }

  for (const auto &entry : std::filesystem::directory_iterator(path)) {
    auto filename =
        std::filesystem::read_symlink(entry.path()).filename().string();
    auto settings = getPortSettings("/dev/" + filename);

    if (!settings.has_value()) {
      continue;
    }

    devices.push_back({filename, settings.value()});
  }

  return devices;
}

std::optional<Context::Devices::IO::Serial::Settings>
getPortSettings(const std::string &path) {
  std::optional<Context::Devices::IO::Serial::Settings> setting;

  auto serialfd = open(path.c_str(), O_RDWR);

  if (serialfd == -1) {
    return setting;
  }

  //  Create new termios struct, we call it 'tty' for convention
  struct termios tty;

  // Read in existing settings, and handle any error
  if (tcgetattr(serialfd, &tty) != 0) {
    close(serialfd);
    return setting;
  }

  Context::Devices::IO::Serial::Settings ser;

  if (tty.c_cflag | PARENB) {
    ser.enableParity = true;
  } else {
    ser.enableParity = false;
  }

  /// TODO FILL IN ALL THE SETTINGS

  setting = ser;

  close(serialfd);
  return setting;
}
#endif

Serial::Serial() {}

bool Serial::isConnected() { return mIsConnected; }

RETURN_CODE Serial::openDevice(const Device &device) {
  disconnect();

  auto baud = num2Baud(device.settings.baud);
  auto path = device.path;

  if (baud == 0) {
    setError(ERROR_CODE::INVALID_ARGUMENT, "Unsupported board rate");
    return RETURN::NOK;
  }

  auto serialfd = open(path.c_str(), O_RDWR);

  if (serialfd == -1) {
    setError(errno,
             std::string("Unable to open serial port: ") + strerror(errno));
    close(serialfd);
    return RETURN::NOK;
  }

  //  Create new termios struct, we call it 'tty' for convention
  struct termios tty;

  // Read in existing settings, and handle any error
  if (tcgetattr(serialfd, &tty) != 0) {
    setError(errno, "Unable to get serial settings");
    close(serialfd);
    return RETURN::NOK;
  }

  // Set C-Flags
  if (device.settings.enableParity) {
    tty.c_cflag |= PARENB; // enable parity
    if (!device.settings.parityEven) {
      tty.c_cflag |= PARODD; // odd parity
    } else {
      tty.c_cflag &= static_cast<decltype(tty.c_cflag)>(~PARODD); // even parity
    }
  } else {
    tty.c_cflag &= static_cast<decltype(tty.c_cflag)>(~PARENB); // enable parity
  }

  if (device.settings.use2StopBits) { // 2 stop bits
    tty.c_cflag |=
        CSTOPB; // Set stop field, two stop bits used in communication
  } else {
    tty.c_cflag &= static_cast<decltype(tty.c_cflag)>(
        ~CSTOPB); // Clear stop field, only one stop bit used in communication
                  // (most common)
  }

  if (device.settings.flowControl) {
    tty.c_cflag |= CRTSCTS; // Enable RTS/CTS hardware flow control
  } else {
    tty.c_cflag &=
        ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
  }

  if (device.settings.hangUp) { // hangup on close
    tty.c_cflag |= HUPCL;
  } else {
    tty.c_cflag &= static_cast<decltype(tty.c_cflag)>(~HUPCL);
  }

  if (device.settings.cLocal) { // ignore modem control lines
    tty.c_cflag |= CLOCAL;
  } else {
    tty.c_cflag &= static_cast<decltype(tty.c_cflag)>(~CLOCAL);
  }

  if (device.settings.cRead) { // Enable Receiver
    tty.c_cflag |= CREAD;
  } else {
    tty.c_cflag &= static_cast<decltype(tty.c_cflag)>(~CREAD);
  }

  tty.c_cflag &= static_cast<decltype(tty.c_cflag)>(
      ~CSIZE); // Clear all the size bits, then set bits per byte

  if (device.settings.bitsPerByte == Serial::Bits::B5) {
    tty.c_cflag |= CS5; // 5 bits per byte
  } else if (device.settings.bitsPerByte == Serial::Bits::B6) {
    tty.c_cflag |= CS6; // 6 bits per byte
  } else if (device.settings.bitsPerByte == Serial::Bits::B7) {
    tty.c_cflag |= CS7; // 7 bits per byte
  } else if (device.settings.bitsPerByte == Serial::Bits::B8) {
    tty.c_cflag |= CS8; // 8 bits per byte
  }

  // Local Mode Flags

  if (device.settings.cannonMode) {
    tty.c_lflag |= ICANON; // enable cannonical mode
  } else {
    tty.c_lflag &=
        static_cast<decltype(tty.c_lflag)>(~ICANON); // disable cannonical mode
  }

  if (device.settings.iSig) {
    tty.c_lflag |= ISIG; // enable interpretation of INTR, QUIT and SUSP
  } else {
    tty.c_lflag &= static_cast<decltype(tty.c_lflag)>(
        ~ISIG); // Disable interpretation of INTR, QUIT and SUSP
  }

  if (device.settings.echo) {
    tty.c_lflag |= ECHO;
  } else {
    tty.c_lflag &= static_cast<decltype(tty.c_lflag)>(~ECHO); // Disable echo
  }

  if (device.settings.erasure) {
    tty.c_lflag |= ECHOE;
  } else {
    tty.c_lflag &=
        static_cast<decltype(tty.c_lflag)>(~ECHOE); // Disable erasure
  }

  if (device.settings.newLineEcho) {
    tty.c_lflag |= ECHONL;
  } else {
    tty.c_lflag &=
        static_cast<decltype(tty.c_lflag)>(~ECHONL); // Disable new-line echo
  }

  if (device.settings.swFlowControl) {
    tty.c_iflag |= (IXON | IXOFF | IXANY); // Turn on s/w flow ctrl
  } else {
    tty.c_iflag &= static_cast<decltype(tty.c_lflag)>(
        ~(IXON | IXOFF | IXANY)); // Turn off s/w flow ctrl
  }

  if (device.settings.specialHandle) {
    tty.c_iflag |= (IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
                    ICRNL); // Enable any special handling of received bytes
  } else {
    tty.c_iflag &= static_cast<decltype(tty.c_lflag)>(
        ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
          ICRNL)); // Disable any special handling of received bytes
  }

  if (device.settings.outInterpret) {
    tty.c_oflag |=
        OPOST; // special interpretation of output bytes (e.g. newline chars)
  } else {
    tty.c_oflag &= static_cast<decltype(tty.c_oflag)>(
        ~OPOST); // Prevent special interpretation of output bytes (e.g. newline
                 // chars)
  }

  if (device.settings.NLCR) {
    tty.c_oflag |= ONLCR; // conversion of newline to carriage return/line feed
  } else {
    tty.c_oflag &= static_cast<decltype(tty.c_oflag)>(
        ~ONLCR); // Prevent conversion of newline to carriage return/line feed
  }
  // // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT
  // PRESENT ON LINUX)
  // // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in
  // output (NOT PRESENT ON LINUX)

  // tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning
  // as soon as any data is received. tty.c_cc[VMIN] = 0;

  // Set in/out baud rate to be the set baud rate
  cfsetispeed(&tty, baud);
  cfsetospeed(&tty, baud);

  // Save tty settings, also checking for error
  if (tcsetattr(serialfd, TCSANOW, &tty) != 0) {
    setError(errno, "Unable to set serial settings");
    close(serialfd);
    return RETURN::NOK;
  }

  registerNewHandle(serialfd);

  return RETURN::OK;
}

void Serial::disconnect() {
  destroyHandle();
  mIsConnected = false;
}
} // namespace Context::Devices::IO
