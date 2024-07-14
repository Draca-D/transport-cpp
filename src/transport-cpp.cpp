#include <transport-cpp/transport-cpp.h>
#include <cstring>

#include <iostream>

namespace Transport{
namespace Information{

const char* version(){
    return "0.1";
}

LibInformation all_info()
{
    LibInformation info;

    std::strncat(info.version, version(), VERSION_STR_LEN);

    info.build          = build();
    info.architecture   = architecture();
    info.cx11           = cx11_abi();
    info.compiler       = compiler();
    info.subsystem      = subsystem();

    return info;
}

Build build()
{
#ifdef NDEBUG
    return Build::RELEASE;
#else
    return Build::DEBUG;
#endif
}

Architecture architecture()
{
    Architecture arch = Architecture::UNKNOWN;

#ifdef _M_X64
    arch = Architecture::x64;
#endif

#ifdef _M_IX86
    arch = Architecture::ix86;
#endif

#ifdef _M_ARM64
    arch = Architecture::arm64;
#endif

#ifdef __i386__
    arch = Architecture::i386;
#endif

#ifdef __x86_64__
    arch = Architecture::x86_64;
#endif

#ifdef __aarch64__
    arch = Architecture::arch_64;
#endif

    return arch;
}

bool cx11_abi()
{
#ifdef _GLIBCXX_USE_CXX11_ABI
    return true;
#else
    return false;
#endif
}

CompilerInformation compiler()
{
    CompilerInformation comp;

    comp.type               = CompilerType::UNKNOWN;
    comp.cpp_version_str[0] = 0;

    // COMPILER VERSIONS
#ifdef _MSC_VER
    comp.type = CompilerType::MSVC;
    //    std::cout << "  transport-cpp/0.1: _MSC_VER" << _MSC_VER<< "\n";
#endif

#ifdef _MSVC_LANG
    comp.type = CompilerType::MSVC;
    //    std::cout << "  transport-cpp/0.1: _MSVC_LANG" << _MSVC_LANG<< "\n";
#endif

#ifdef __cplusplus
    std::snprintf(comp.cpp_version_str, VERSION_STR_LEN, "%ld", __cplusplus);
    comp.cpp_version = __cplusplus;
#endif

#ifdef __INTEL_COMPILER
    comp.type = CompilerType::INTEL;
    //    std::cout << "  transport-cpp/0.1: __INTEL_COMPILER" << __INTEL_COMPILER<< "\n";
#endif

#ifdef __GNUC__
    comp.type = CompilerType::GNUC;
    comp.version_major = __GNUC__;
#endif

#ifdef __GNUC_MINOR__
    comp.version_minor = __GNUC_MINOR__;
#endif

#ifdef __clang_major__
    comp.type = CompilerType::CLANG;
    comp.version_major = __clang_major__;
#endif

#ifdef __clang_minor__
    comp.version_minor = __clang_minor__;
#endif

#ifdef __apple_build_version__
    comp.type = CompilerType::APPLE;
    //    std::cout << "  transport-cpp/0.1: __apple_build_version__" << __apple_build_version__<< "\n";
#endif

    return comp;
}

Subsystem subsystem()
{
    Subsystem system = Subsystem::NONE;

#ifdef __MSYS__
    system = Subsystem::MSYS;
#endif

#ifdef __MINGW32__
    system = Subsystem::MINGW32;
#endif

#ifdef __MINGW64__
    system = Subsystem::MINGW64;
#endif

#ifdef __CYGWIN__
    system = Subsystem::CYGWIN;
#endif

    return system;
}

}
}
