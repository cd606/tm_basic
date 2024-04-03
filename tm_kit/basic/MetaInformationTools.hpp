#ifndef TM_KIT_BASIC_MULTI_APP_META_INFORMATION_TOOLS_HPP_
#define TM_KIT_BASIC_MULTI_APP_META_INFORMATION_TOOLS_HPP_

#include <tm_kit/basic/MetaInformation.hpp>
#include <iostream>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    class MetaInformationTools {
    public:
        static void writePythonCode(MetaInformation const &, std::ostream &);
    };
} } } }

#endif