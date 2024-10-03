cmake_minimum_required(VERSION 3.15)

project(transport-cpp LANGUAGES CXX VERSION 0.1)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HEADER_DIR include/transport-cpp)

set(HEADERS
    ${HEADER_DIR}/device.h
    ${HEADER_DIR}/engine.h
    ${HEADER_DIR}/iodevice.h
    ${HEADER_DIR}/timer.h
    ${HEADER_DIR}/transport-cpp.h
    ${HEADER_DIR}/io/serial.h
    ${HEADER_DIR}/networking/networkdevice.h
    ${HEADER_DIR}/networking/tcpclient.h
    ${HEADER_DIR}/networking/tcpserver.h

    ${HEADER_DIR}/networking/udpsender.h
    ${HEADER_DIR}/networking/udpreceiver.h
    ${HEADER_DIR}/networking/udpclient.h
    ${HEADER_DIR}/networking/udpserver.h
    ${HEADER_DIR}/networking/udpbroadcaster.h
    ${HEADER_DIR}/networking/udpmulticaster.h
)

add_library(transport-cpp SHARED
    ${HEADERS}

    src/transport-cpp.cpp
    src/engine.cpp
    src/device.cpp
    src/timer.cpp
    src/iodevice.cpp
    src/networkdevice.cpp
    src/serial.cpp
    src/tcpclient.cpp
    src/tcpserver.cpp
    src/udpsender.cpp
    src/udpreceiver.cpp
    src/udpclient.cpp
    src/udpserver.cpp
    src/udpbroadcaster.cpp
    src/udpmulticaster.cpp
)

# set_target_properties(transport-cpp PROPERTIES PUBLIC_HEADER "include/transport-cpp.h")

# install(TARGETS transport-cpp)

# target_include_directories(transport-cpp PUBLIC include)

target_include_directories(transport-cpp PUBLIC
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
   $<INSTALL_INTERFACE:include>)

install(TARGETS transport-cpp
    EXPORT "transport-cppTargets"
    DESTINATION lib
    INCLUDES DESTINATION include)

install(DIRECTORY include/
    DESTINATION include)

## Put export set to right place
install(EXPORT "transport-cppTargets"
  FILE "transport-cppTargets.cmake"
  NAMESPACE "transport::"
  DESTINATION "lib/cmake/transport-cpp"
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "transport-cppConfigVersion.cmake"
    VERSION ${transport-cpp_VERSION}
    COMPATIBILITY AnyNewerVersion)

install(FILES "transport-cppConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/transport-cppConfigVersion.cmake"
    DESTINATION lib/cmake/transport-cpp)
