#ifndef TM_KIT_BASIC_GRAPH_CONTROL_FACILITY_HPP_
#define TM_KIT_BASIC_GRAPH_CONTROL_FACILITY_HPP_

#include <tm_kit/basic/SerializationHelperMacros.hpp>
#include <tm_kit/basic/VoidStruct.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {
    #define TM_KIT_BASIC_GRAPH_CONTROL_FACILITY_INPUT_FIELDS \
        ((std::optional<std::string>, name)) \
        ((std::optional<std::string>, nameRE)) \
        ((std::string, command)) \
        ((std::vector<std::string>, params))

    TM_BASIC_CBOR_CAPABLE_STRUCT(GraphControlFacilityInput, TM_KIT_BASIC_GRAPH_CONTROL_FACILITY_INPUT_FIELDS);

    template <class R, class Output=VoidStruct>
    inline auto graphControlFacility(
        R &r
        , std::string const &facilityName
    ) {
        return R::AppType::template liftPureOnOrderFacility<
            GraphControlFacilityInput
        >(
            [&r](GraphControlFacilityInput &&input) -> Output {
                if (input.name) {
                    r.controlAll(input.name, input.command, input.params);
                } else if (input.nameRE) {
                    r.controlAllRE(std::regex(input.name), input.command, input.params);
                }
                return Output {};
            }
        );
    }

} } } }

TM_BASIC_CBOR_CAPABLE_STRUCT_SERIALIZE(dev::cd606::tm::basic::GraphControlFacilityInput, TM_KIT_BASIC_GRAPH_CONTROL_FACILITY_INPUT_FIELDS);

#endif