#ifndef TRANSPORT_CPP
#define TRANSPORT_CPP

#ifdef _WIN32
#define TRANSPORT_CPP_EXPORT __declspec(dllexport)
#else
#define TRANSPORT_CPP_EXPORT
#endif

using RETURN_CODE       = int;
using DEVICE_HANDLE_    = int;

namespace RETURN {
static constexpr RETURN_CODE OK         = 0;
static constexpr RETURN_CODE NOK        = 1;
static constexpr RETURN_CODE PASSABLE   = 2;
}

namespace Transport {

namespace Information{

enum class Build {
    DEBUG,
    RELEASE
};

enum class Architecture {
    UNKNOWN,
    x64,
    ix86,
    arm64,
    i386,
    x86_64,
    arch_64
};

enum class CompilerType{
    UNKNOWN,
    MSVC,
    INTEL,
    GNUC,
    CLANG,
    APPLE
};

enum class Subsystem{
    NONE,
    MSYS,
    MINGW32,
    MINGW64,
    CYGWIN
};

static constexpr int VERSION_STR_LEN = 256;

struct TRANSPORT_CPP_EXPORT CompilerStruct_{
    CompilerType        type;
    char                cpp_version_str[VERSION_STR_LEN];
    long                cpp_version;
    long                version_major;
    long                version_minor;
};

using CompilerInformation = CompilerStruct_;

struct TRANSPORT_CPP_EXPORT InfoStruct_{
    char                version[VERSION_STR_LEN];
    Build               build;
    Architecture        architecture;
    bool                cx11;
    CompilerInformation compiler;
    Subsystem           subsystem;
};


using LibInformation = InfoStruct_;

TRANSPORT_CPP_EXPORT const char*            version();
TRANSPORT_CPP_EXPORT Build                  build();
TRANSPORT_CPP_EXPORT Architecture           architecture();
TRANSPORT_CPP_EXPORT bool                   cx11_abi();
TRANSPORT_CPP_EXPORT CompilerInformation    compiler();
TRANSPORT_CPP_EXPORT Subsystem              subsystem();
TRANSPORT_CPP_EXPORT LibInformation         all_info();

}
}


#endif /* TRANSPORT_CPP */
