#ifndef TM_KIT_BASIC_NOT_CONSTRUCTIBLE_STRUCT_HPP_
#define TM_KIT_BASIC_NOT_CONSTRUCTIBLE_STRUCT_HPP_

#include <iostream>
#include <functional>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    class NotConstructibleStruct {
    private:
        NotConstructibleStruct() {}
    public:
        ~NotConstructibleStruct() = default;
        NotConstructibleStruct(NotConstructibleStruct const &) = default;
        NotConstructibleStruct &operator=(NotConstructibleStruct const &) = default;
        NotConstructibleStruct(NotConstructibleStruct &&) = default;
        NotConstructibleStruct &operator=(NotConstructibleStruct &&) = default;
    };
} } } }

#endif