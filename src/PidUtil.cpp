#include <tm_kit/basic/PidUtil.hpp>

#ifdef _MSC_VER
#include <windows.h>
#include <processthreadsapi.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

namespace dev { namespace cd606 { namespace tm { namespace basic { namespace pid_util {
    int64_t getpid() {
        #ifdef _MSC_VER
            return (int64_t) GetCurrentProcessId();
        #else
            return (int64_t) ::getpid();
        #endif
    }
} } } } }