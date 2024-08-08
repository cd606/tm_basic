#ifndef TM_KIT_BASIC_STRUCT_FIELD_INFO_WITH_MASKING_FILTER_HPP_
#define TM_KIT_BASIC_STRUCT_FIELD_INFO_WITH_MASKING_FILTER_HPP_

#include <tm_kit/basic/StructFieldInfoHelper.hpp>
#include <tm_kit/basic/StructFieldInfoBasedTuplefy.hpp>
#include <tm_kit/basic/StructFieldInfoUtils.hpp>
#include <tm_kit/basic/ByteData.hpp>
#include <string_view>

namespace dev { namespace cd606 { namespace tm { namespace basic { 
    namespace struct_field_info_masking {
        namespace internal {
            template <std::size_t N, std::size_t K>
            inline constexpr std::size_t masking_filter_result_size_impl(
                std::array<std::string_view, N> const &input 
                , bool (*f)(std::string_view const &)
                , std::size_t accum
            ) {
                if (f == nullptr) {
                    return N;
                } else {
                    if constexpr (K>=0 && K<N) {
                        return masking_filter_result_size_impl<N,K+1>(
                            input, f, accum+(f(input[K])?1:0)
                        );
                    } else {
                        return accum;
                    }
                }
            }
            template <std::size_t N>
            inline constexpr std::size_t masking_filter_result_size(
                std::array<std::string_view, N> const &input 
                , bool (*f)(std::string_view const &)
            ) {
                return masking_filter_result_size_impl<N,0>(input, f, 0);
            }

            template <std::size_t N, std::size_t K>
            class MaskingFilterResults {
            public:
                std::array<std::size_t, K> filteredToOriginal {0};
                std::array<std::size_t, N> originalToFiltered {0};
                std::array<std::string_view, K> filteredNames {""};

                constexpr MaskingFilterResults(
                    std::array<std::string_view, N> const &input 
                    , bool (*f)(std::string_view const &)
                ) {
                    if (f == nullptr) {
                        for (int ii=0; ii<N; ++ii) {
                            originalToFiltered[ii] = ii;
                            filteredToOriginal[ii] = ii;
                            filteredNames[ii] = input[ii];
                        }
                    } else {
                        int jj=0;
                        for (int ii=0; ii<N; ++ii) {
                            if (f(input[ii])) {
                                originalToFiltered[ii] = jj;
                                filteredToOriginal[jj] = ii;
                                filteredNames[jj] = input[ii];
                                ++jj;
                            } else {
                                originalToFiltered[ii] = N;
                            }
                        }
                    }
                }
            };
        }

        template <
            class T
            , bool (*maskF)(std::string_view const &) 
            , typename=std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo>
        >
        struct MaskedStruct : public T {};

        template <
            class T
            , bool (*maskF)(std::string_view const &) 
            , typename=std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo>
        >
        struct MaskedStructInfo {
            static constexpr std::size_t N = StructFieldInfo<T>::FIELD_NAMES.size();
            static constexpr std::size_t K = struct_field_info_masking::internal::masking_filter_result_size<N>(
                StructFieldInfo<T>::FIELD_NAMES, maskF
            );
            static constexpr struct_field_info_masking::internal::MaskingFilterResults<N,K> filterResults = {
                StructFieldInfo<T>::FIELD_NAMES, maskF
            };
        };
    }

    template <
        class T
        , bool (*maskF)(std::string_view const &)
    >
    class StructFieldInfo<struct_field_info_masking::MaskedStruct<T,maskF>> {
    public:
        static constexpr bool HasGeneratedStructFieldInfo = true;
        static constexpr bool EncDecWithFieldNames = StructFieldInfo<T>::EncDecWithFieldNames;
        static constexpr std::string_view REFERENCE_NAME = "struct_field_info_masking::MaskedStruct<>" ; \
    private:
        using MSI = struct_field_info_masking::MaskedStructInfo<T,maskF>;
        static constexpr std::size_t N = MSI::N;
        static constexpr std::size_t K = MSI::K;
        static constexpr struct_field_info_masking::internal::MaskingFilterResults<N,K> filterResults = MSI::filterResults;
    public:
        static constexpr std::array<std::string_view, K> FIELD_NAMES = filterResults.filteredNames;
        static constexpr int getFieldIndex(std::string_view const &fieldName) { 
            for (int ii=0; ii<K; ++ii) {
                if (fieldName == FIELD_NAMES[ii]) {
                    return ii;
                }
            }
            return -1;
        }
    };
    template <
        class T
        , bool (*maskF)(std::string_view const &)
        , int Idx
    >
    class StructFieldTypeInfo<struct_field_info_masking::MaskedStruct<T,maskF>, Idx> {
    private:
        using MSI = struct_field_info_masking::MaskedStructInfo<T,maskF>;
        static constexpr std::size_t N = MSI::N;
        static constexpr std::size_t K = MSI::K;
        static constexpr struct_field_info_masking::internal::MaskingFilterResults<N,K> filterResults = MSI::filterResults;
    public:
        using TheType = typename StructFieldTypeInfo<
            T, filterResults.filteredToOriginal[Idx]
        >::TheType; 
        static TheType &access(struct_field_info_masking::MaskedStruct<T,maskF> &data) {
            return StructFieldTypeInfo<
                T, filterResults.filteredToOriginal[Idx]
            >::access(static_cast<T &>(data));
        }
        static TheType const &constAccess(struct_field_info_masking::MaskedStruct<T,maskF> const &data) {
            return StructFieldTypeInfo<
                T, filterResults.filteredToOriginal[Idx]
            >::constAccess(static_cast<T const &>(data));
        }
        static TheType &&moveAccess(struct_field_info_masking::MaskedStruct<T,maskF> &&data) {
            return StructFieldTypeInfo<
                T, filterResults.filteredToOriginal[Idx]
            >::moveAccess(static_cast<T &&>(data));
        }
    };

    template <
        class T
        , bool (*maskF)(std::string_view const &)
    >
    class PrintHelper<struct_field_info_masking::MaskedStruct<T,maskF>> {
    private:
        using MSI = struct_field_info_masking::MaskedStructInfo<T,maskF>;
        static constexpr std::size_t N = MSI::N;
        static constexpr std::size_t K = MSI::K;
        static constexpr struct_field_info_masking::internal::MaskingFilterResults<N,K> filterResults = MSI::filterResults;
        template <std::size_t Idx>
        static void printOne(std::ostream &os, struct_field_info_masking::MaskedStruct<T,maskF> const &t) {
            if constexpr (Idx >= 0 && Idx < K) {
                if constexpr (Idx > 0) {
                    os << ',';
                }
                os << filterResults.filteredNames[Idx] << '=';
                PrintHelper<typename StructFieldTypeInfo<
                    T, filterResults.filteredToOriginal[Idx]
                >::TheType>::print(os, StructFieldTypeInfo<
                    T, filterResults.filteredToOriginal[Idx]
                >::constAccess(static_cast<T const &>(t)));
                printOne<Idx+1>(os, t);
            }
        }
    public:
        static void print(std::ostream &os, struct_field_info_masking::MaskedStruct<T,maskF> const &t) {
            os << '{';
            printOne<0>(os, t);
            os << '}';
        }
    };
    
    namespace bytedata_utils {
        template <
            class T
            , bool (*maskF)(std::string_view const &)
        >
        struct RunCBORSerializer<struct_field_info_masking::MaskedStruct<T,maskF>, void> {
            static std::string apply(struct_field_info_masking::MaskedStruct<T,maskF> const &x) {
                std::string s;
                s.resize(calculateSize(x));
                apply(x, const_cast<char *>(s.data()));
                return s;
            }
            static std::size_t apply(struct_field_info_masking::MaskedStruct<T,maskF> const &x, char *output) {
                return RunCBORSerializer<
                    typename struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_masking::MaskedStruct<T,maskF>
                    >::ConstPtrTupleType
                >::apply(
                    struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_masking::MaskedStruct<T,maskF>
                    >::toConstPtrTuple(x)
                    , output
                );
            }
            static std::size_t calculateSize(struct_field_info_masking::MaskedStruct<T,maskF> const &x) {
                return RunCBORSerializer<
                    typename struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_masking::MaskedStruct<T,maskF>
                    >::ConstPtrTupleType
                >::calculateSize(
                    struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_masking::MaskedStruct<T,maskF>
                    >::toConstPtrTuple(x)
                );
            }
        };
        template <
            class T
            , bool (*maskF)(std::string_view const &)
        >
        struct RunSerializer<struct_field_info_masking::MaskedStruct<T,maskF>, void> {
            static std::string apply(struct_field_info_masking::MaskedStruct<T,maskF> const &x) {
                return RunCBORSerializer<struct_field_info_masking::MaskedStruct<T,maskF>, void>::apply(x);
            }
        };

        template <
            class T
            , bool (*maskF)(std::string_view const &)
        >
        struct RunCBORDeserializer<struct_field_info_masking::MaskedStruct<T,maskF>, void> {
            static std::optional<std::tuple<struct_field_info_masking::MaskedStruct<T,maskF>, size_t>> apply(std::string_view const &s, size_t start) {
                auto t = RunCBORDeserializer<
                    typename struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_masking::MaskedStruct<T,maskF>
                    >::TupleType
                >::apply(s, start);
                if (!t) {
                    return std::nullopt; 
                }
                return std::tuple<struct_field_info_masking::MaskedStruct<T,maskF>, size_t> {
                    struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_masking::MaskedStruct<T,maskF>
                    >::fromTupleByMove(std::move(std::get<0>(*t)))
                    , std::get<1>(*t)
                };
            }
            static std::optional<size_t> applyInPlace(struct_field_info_masking::MaskedStruct<T,maskF> &x, std::string_view const &s, size_t start) {
                return RunCBORDeserializer<
                    typename struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_masking::MaskedStruct<T,maskF>
                    >::PtrTupleTypeType
                >::applyInPlace(
                    struct_field_info_utils::StructFieldInfoBasedTuplefy<
                        struct_field_info_masking::MaskedStruct<T,maskF>
                    >::toPtrTuple(x)
                    , s
                    , start
                );
            }
        };
        template <
            class T
            , bool (*maskF)(std::string_view const &)
        >
        struct RunDeserializer<struct_field_info_masking::MaskedStruct<T,maskF>, void> {
            static std::optional<struct_field_info_masking::MaskedStruct<T,maskF>> apply(std::string_view const &s) {
                auto t = RunDeserializer<CBOR<struct_field_info_masking::MaskedStruct<T,maskF>>, void>::apply(s);
                if (!t) {
                    return std::nullopt;
                }
                return std::move(t->value);
            }
            static std::optional<struct_field_info_masking::MaskedStruct<T,maskF>> apply(std::string const &s) {
                return apply(std::string_view {s});
            }
            static bool applyInPlace(struct_field_info_masking::MaskedStruct<T,maskF> &output, std::string_view const &s) {
                auto t = RunCBORDeserializer<struct_field_info_masking::MaskedStruct<T,maskF>, void>::applyInPlace(output, s, 0);
                if (!t) {
                    return false;
                }
                if (*t != s.length()) {
                    return false;
                }
                return true;
            }
            static bool applyInPlace(struct_field_info_masking::MaskedStruct<T,maskF> &output, std::string const &s) {
                return applyInPlace(output, std::string_view {s});
            }
        };
    }

    namespace struct_field_info_masking {
        namespace internal {
            template <class T>
            class MaskedStructViewHelpers {
            public:
                template <class DestMSI, std::size_t DestIdx, class SrcMSI>
                static void makeCopy(
                    typename struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::PtrTupleType &destTup
                    , typename struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::ConstPtrTupleType const &srcTup
                ) {
                    if constexpr (DestIdx >= 0 && DestIdx < DestMSI::K) {
                        constexpr std::size_t RealIdx = DestMSI::filterResults.filteredToOriginal[DestIdx];
                        constexpr std::size_t SrcIdx = SrcMSI::filterResults.originalToFiltered[RealIdx];
                        if constexpr (SrcIdx >= 0 && SrcIdx < SrcMSI::K) {
                            *(std::get<RealIdx>(destTup)) = *(std::get<RealIdx>(srcTup));
                        }
                        makeCopy<DestMSI, DestIdx+1, SrcMSI>(destTup, srcTup);
                    }
                }
                template <class DestMSI, std::size_t DestIdx, class SrcMSI>
                static void makeMove(
                    typename struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::PtrTupleType &destTup
                    , typename struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::PtrTupleType &srcTup
                ) {
                    if constexpr (DestIdx >= 0 && DestIdx < DestMSI::K) {
                        constexpr std::size_t RealIdx = DestMSI::filterResults.filteredToOriginal[DestIdx];
                        constexpr std::size_t SrcIdx = SrcMSI::filterResults.originalToFiltered[RealIdx];
                        if constexpr (SrcIdx >= 0 && SrcIdx < SrcMSI::K) {
                            *(std::get<RealIdx>(destTup)) = std::move(*(std::get<RealIdx>(srcTup)));
                        }
                        makeMove<DestMSI, DestIdx+1, SrcMSI>(destTup, srcTup);
                    }
                }
            };
        }
        template <
            class T
            , bool (*maskF)(std::string_view const &) 
            , typename=std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo>
        >
        class MaskedStructConstView {
        private:
            T const *t_;
        public:
            MaskedStructConstView(T const *t) : t_(t) {}
            MaskedStructConstView(T const &t) : t_(&t) {}
            ~MaskedStructConstView() = default;
            MaskedStructConstView(MaskedStructConstView const &) = default;
            MaskedStructConstView(MaskedStructConstView &&) = default;
            MaskedStructConstView &operator=(MaskedStructConstView const &) = delete;
            MaskedStructConstView &operator=(MaskedStructConstView &&) = delete;

            T const *ptr() {return t_;}
            T const *ptr() const {return t_;}
            operator MaskedStruct<T,maskF>() const {
                MaskedStruct<T,maskF> m;
                T &t = *static_cast<T *>(&m);
                struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(t);

                using MSI = MaskedStructInfo<T, nullptr>;
                using MSI1 = MaskedStructInfo<T, maskF>;

                auto destTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toPtrTuple(t);
                auto srcTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toConstPtrTuple(*ptr());

                internal::MaskedStructViewHelpers<T>::template makeCopy<MSI,0,MSI1>(destTup, srcTup);

                return m;
            }
        };
        template <
            class T
            , bool (*maskF)(std::string_view const &) 
            , typename=std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo>
        >
        class MaskedStructMoveView {
        private:
            T *t_;
        public:
            MaskedStructMoveView(T *t) : t_(t) {}
            MaskedStructMoveView(T &&t) : t_(&t) {}
            ~MaskedStructMoveView() = default;
            MaskedStructMoveView(MaskedStructMoveView const &) = default;
            MaskedStructMoveView(MaskedStructMoveView &&) = default;
            MaskedStructMoveView &operator=(MaskedStructMoveView const &) = delete;
            MaskedStructMoveView &operator=(MaskedStructMoveView &&) = delete;

            T *ptr() {return t_;}
            T *ptr() const {return t_;}
            operator MaskedStruct<T,maskF>() && {
                MaskedStruct<T,maskF> m;
                T &t = *static_cast<T *>(&m);
                struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(t);

                using MSI = MaskedStructInfo<T, nullptr>;
                using MSI1 = MaskedStructInfo<T, maskF>;

                auto destTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toPtrTuple(t);
                auto srcTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toPtrTuple(*ptr());

                internal::MaskedStructViewHelpers<T>::template makeMove<MSI,0,MSI1>(destTup, srcTup);

                return m;
            }
        };

        template <
            class T
            , bool (*maskF)(std::string_view const &) 
            , typename=std::enable_if_t<StructFieldInfo<T>::HasGeneratedStructFieldInfo>
        >
        class MaskedStructView {
        private:
            T *t_;
        public:
            MaskedStructView(T *t) : t_(t) {}
            MaskedStructView(T &t) : t_(&t) {}
            ~MaskedStructView() = default;
            MaskedStructView(MaskedStructView const &) = default;
            MaskedStructView(MaskedStructView &&) = default;

            T *ptr() {return t_;}
            T const *ptr() const {return t_;}

            MaskedStructView &operator=(MaskedStructView const &other) {
                using MSI = MaskedStructInfo<T, maskF>;
                using MSI1 = MaskedStructInfo<T, maskF>;

                auto destTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toPtrTuple(*ptr());
                auto srcTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toConstPtrTuple(*(other.ptr()));

                internal::MaskedStructViewHelpers<T>::template makeCopy<MSI,0,MSI1>(destTup, srcTup);

                return *this;
            }
            template <bool (*maskF1)(std::string_view const &)>
            MaskedStructView &operator=(MaskedStructView<T,maskF1> const &other) {
                using MSI = MaskedStructInfo<T, maskF>;
                using MSI1 = MaskedStructInfo<T, maskF1>;

                auto destTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toPtrTuple(*ptr());
                auto srcTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toConstPtrTuple(*(other.ptr()));

                internal::MaskedStructViewHelpers<T>::template makeCopy<MSI,0,MSI1>(destTup, srcTup);

                return *this;
            }
            template <bool (*maskF1)(std::string_view const &)>
            MaskedStructView &operator=(MaskedStructConstView<T,maskF1> const &other) {
                using MSI = MaskedStructInfo<T, maskF>;
                using MSI1 = MaskedStructInfo<T, maskF1>;

                auto destTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toPtrTuple(*ptr());
                auto srcTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toConstPtrTuple(*(other.ptr()));

                internal::MaskedStructViewHelpers<T>::template makeCopy<MSI,0,MSI1>(destTup, srcTup);

                return *this;
            }
            template <bool (*maskF1)(std::string_view const &)>
            MaskedStructView &operator=(MaskedStruct<T,maskF1> const &other) {
                T const *t = static_cast<T const *>(&other);
                using MSI = MaskedStructInfo<T, maskF>;
                using MSI1 = MaskedStructInfo<T, maskF1>;

                auto destTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toPtrTuple(*ptr());
                auto srcTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toConstPtrTuple(*t);

                internal::MaskedStructViewHelpers<T>::template makeCopy<MSI,0,MSI1>(destTup, srcTup);

                return *this;
            }
            MaskedStructView &operator=(T const &other) {
                using MSI = MaskedStructInfo<T, maskF>;
                using MSI1 = MaskedStructInfo<T, nullptr>;

                auto destTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toPtrTuple(*ptr());
                auto srcTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toConstPtrTuple(other);

                internal::MaskedStructViewHelpers<T>::template makeCopy<MSI,0,MSI1>(destTup, srcTup);

                return *this;
            }
            operator MaskedStruct<T,maskF>() const & {
                MaskedStruct<T,maskF> m;
                T &t = *static_cast<T *>(&m);
                struct_field_info_utils::StructFieldInfoBasedInitializer<T>::initialize(t);

                using MSI = MaskedStructInfo<T, nullptr>;
                using MSI1 = MaskedStructInfo<T, maskF>;

                auto destTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toPtrTuple(t);
                auto srcTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toConstPtrTuple(*ptr());

                internal::MaskedStructViewHelpers<T>::template makeCopy<MSI,0,MSI1>(destTup, srcTup);

                return m;
            }
            template <bool (*maskF1)(std::string_view const &)>
            MaskedStructView &operator=(MaskedStructMoveView<T,maskF1> &&other) {
                using MSI = MaskedStructInfo<T, maskF>;
                using MSI1 = MaskedStructInfo<T, maskF1>;

                auto destTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toPtrTuple(*ptr());
                auto srcTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toPtrTuple(*(other.ptr()));

                internal::MaskedStructViewHelpers<T>::template makeMove<MSI,0,MSI1>(destTup, srcTup);

                return *this;
            }
            template <bool (*maskF1)(std::string_view const &)>
            MaskedStructView &operator=(MaskedStruct<T,maskF1> &&other) {
                T *t = static_cast<T *>(&other);
                using MSI = MaskedStructInfo<T, maskF>;
                using MSI1 = MaskedStructInfo<T, maskF1>;

                auto destTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toPtrTuple(*ptr());
                auto srcTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toPtrTuple(*t);

                internal::MaskedStructViewHelpers<T>::template makeMove<MSI,0,MSI1>(destTup, srcTup);

                return *this;
            }
            MaskedStructView &operator=(T &&other) {
                using MSI = MaskedStructInfo<T, maskF>;
                using MSI1 = MaskedStructInfo<T, nullptr>;

                auto destTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toPtrTuple(*ptr());
                auto srcTup = struct_field_info_utils::StructFieldInfoBasedTuplefy<T>::toPtrTuple(other);

                internal::MaskedStructViewHelpers<T>::template makeMove<MSI,0,MSI1>(destTup, srcTup);

                return *this;
            }
        };
    }

    template <
        class T
        , bool (*maskF)(std::string_view const &)
    >
    class PrintHelper<struct_field_info_masking::MaskedStructView<T,maskF>> {
    private:
        using MSI = struct_field_info_masking::MaskedStructInfo<T,maskF>;
        static constexpr std::size_t N = MSI::N;
        static constexpr std::size_t K = MSI::K;
        static constexpr struct_field_info_masking::internal::MaskingFilterResults<N,K> filterResults = MSI::filterResults;
        template <std::size_t Idx>
        static void printOne(std::ostream &os, struct_field_info_masking::MaskedStructView<T,maskF> const &t) {
            if constexpr (Idx >= 0 && Idx < K) {
                if constexpr (Idx > 0) {
                    os << ',';
                }
                os << filterResults.filteredNames[Idx] << '=';
                PrintHelper<typename StructFieldTypeInfo<
                    T, filterResults.filteredToOriginal[Idx]
                >::TheType>::print(os, StructFieldTypeInfo<
                    T, filterResults.filteredToOriginal[Idx]
                >::constAccess(*(t.ptr())));
                printOne<Idx+1>(os, t);
            }
        }
    public:
        static void print(std::ostream &os, struct_field_info_masking::MaskedStructView<T,maskF> const &t) {
            os << '{';
            printOne<0>(os, t);
            os << '}';
        }
    };
    template <
        class T
        , bool (*maskF)(std::string_view const &)
    >
    class PrintHelper<struct_field_info_masking::MaskedStructConstView<T,maskF>> {
    private:
        using MSI = struct_field_info_masking::MaskedStructInfo<T,maskF>;
        static constexpr std::size_t N = MSI::N;
        static constexpr std::size_t K = MSI::K;
        static constexpr struct_field_info_masking::internal::MaskingFilterResults<N,K> filterResults = MSI::filterResults;
        template <std::size_t Idx>
        static void printOne(std::ostream &os, struct_field_info_masking::MaskedStructConstView<T,maskF> const &t) {
            if constexpr (Idx >= 0 && Idx < K) {
                if constexpr (Idx > 0) {
                    os << ',';
                }
                os << filterResults.filteredNames[Idx] << '=';
                PrintHelper<typename StructFieldTypeInfo<
                    T, filterResults.filteredToOriginal[Idx]
                >::TheType>::print(os, StructFieldTypeInfo<
                    T, filterResults.filteredToOriginal[Idx]
                >::constAccess(*(t.ptr())));
                printOne<Idx+1>(os, t);
            }
        }
    public:
        static void print(std::ostream &os, struct_field_info_masking::MaskedStructConstView<T,maskF> const &t) {
            os << '{';
            printOne<0>(os, t);
            os << '}';
        }
    };
    
} } } }

#endif