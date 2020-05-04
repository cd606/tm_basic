#ifndef TM_KIT_BASIC_CONST_GENERATOR_HPP_
#define TM_KIT_BASIC_CONST_GENERATOR_HPP_

namespace dev { namespace cd606 { namespace tm { namespace basic {
    template <class T, class X>
    class ConstGenerator {
    private:
        T t_;
    public:
        ConstGenerator(T const &t) : t_(t) {}
        ConstGenerator(T &&t) : t_(std::move(t)) {}
        ConstGenerator(ConstGenerator const &) = default;
        ConstGenerator &operator=(ConstGenerator const &) = default;
        ConstGenerator(ConstGenerator &&) = default;
        ConstGenerator &operator=(ConstGenerator &&) = default;
        T operator()(X const &x) const {
            return t_;
        }
    };
} } } }

#endif