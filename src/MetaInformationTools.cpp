#include "tm_kit/basic/MetaInformationTools.hpp"
#include <boost/algorithm/string.hpp>

namespace dev { namespace cd606 { namespace tm { namespace basic {

    namespace {
        void allNamedTypesHelper(MetaInformation const &info, std::unordered_map<std::string, MetaInformation> &output) {
            std::visit([&info,&output](auto const &x) {
                using T = std::decay_t<decltype(x)>;
                if constexpr (std::is_same_v<T, MetaInformation_Enum>) {
                    if (output.find(x.TypeID) == output.end()) {
                        output[x.TypeID] = info;
                    }
                } else if constexpr (std::is_same_v<T, MetaInformation_Structure>) {
                    if (output.find(x.TypeID) == output.end()) {
                        output[x.TypeID] = info;
                    }
                    for (auto const &y : x.Fields) {
                        allNamedTypesHelper(*(y.FieldType), output);
                    }
                } else if constexpr (std::is_same_v<T, MetaInformation_Tuple>) {
                    for (auto const &y : x.TupleContent) {
                        allNamedTypesHelper(*y, output);
                    }
                } else if constexpr (std::is_same_v<T, MetaInformation_Variant>) {
                    for (auto const &y : x.VariantChoices) {
                        allNamedTypesHelper(*y, output);
                    }
                } else if constexpr (std::is_same_v<T, MetaInformation_Optional>) {
                    allNamedTypesHelper(*(x.UnderlyingType), output);
                } else if constexpr (std::is_same_v<T, MetaInformation_Collection>) {
                    allNamedTypesHelper(*(x.UnderlyingType), output);
                } else if constexpr (std::is_same_v<T, MetaInformation_Dictionary>) {
                    allNamedTypesHelper(*(x.KeyType), output);
                    allNamedTypesHelper(*(x.ValueType), output);
                } 
            }, info);
        }
        void allUnnamedTypesHelper(MetaInformation const &info, std::map<std::string, std::tuple<std::size_t, MetaInformation>> &output) {
            std::visit([&info,&output](auto const &x) {
                using T = std::decay_t<decltype(x)>;
                if constexpr (std::is_same_v<T, MetaInformation_Tuple>) {
                    output[x.TypeID] = {0, info};
                    for (auto const &item : x.TupleContent) {
                        allUnnamedTypesHelper(*item, output);
                    }
                } else if constexpr (std::is_same_v<T, MetaInformation_Variant>) {
                    output[x.TypeID] = {0, info};
                    for (auto const &item : x.VariantChoices) {
                        allUnnamedTypesHelper(*item, output);
                    }
                } else if constexpr (std::is_same_v<T, MetaInformation_Optional>) {
                    output[x.TypeID] = {0, info};
                    allUnnamedTypesHelper(*(x.UnderlyingType), output);
                } else if constexpr (std::is_same_v<T, MetaInformation_Collection>) {
                    output[x.TypeID] = {0, info};
                    allUnnamedTypesHelper(*(x.UnderlyingType), output);
                } else if constexpr (std::is_same_v<T, MetaInformation_Dictionary>) {
                    output[x.TypeID] = {0, info};
                    allUnnamedTypesHelper(*(x.KeyType), output);
                    allUnnamedTypesHelper(*(x.ValueType), output);
                } else if constexpr (std::is_same_v<T, MetaInformation_Structure>) {
                    for (auto const &y : x.Fields) {
                        allUnnamedTypesHelper(*(y.FieldType), output);
                    }
                } 
            }, info);
        }

        std::unordered_map<std::string, MetaInformation> allNamedTypes(MetaInformation const &info) {
            std::unordered_map<std::string, MetaInformation> m;
            allNamedTypesHelper(info, m);
            return m;
        }
        std::map<std::string, std::tuple<std::size_t, MetaInformation>> allUnnamedTypes(MetaInformation const &info) {
            std::map<std::string, std::tuple<std::size_t, MetaInformation>> m;
            allUnnamedTypesHelper(info, m);
            std::size_t counter = 0;
            for (auto &item : m) {
                std::get<0>(item.second) = (++counter);
            }
            return m;
        }

        std::unordered_map<std::string, std::string> assignTypeNames(std::unordered_map<std::string, MetaInformation> const &m) {
            std::unordered_map<std::string, std::string> output;
            std::unordered_map<std::string, int> nameCount;
            for (auto const &x : m) {
                std::string referenceName = std::visit([](auto const &y) -> std::string {
                    using T = std::decay_t<decltype(y)>;
                    if constexpr (std::is_same_v<T, MetaInformation_Enum>) {
                        return y.TypeReferenceName;
                    } else if constexpr (std::is_same_v<T, MetaInformation_Structure>) {
                        return y.TypeReferenceName;
                    } else {
                        return "";
                    }
                    return "";
                }, x.second);
                boost::replace_all(referenceName, "::", "__");
                auto iter = nameCount.find(referenceName);
                if (iter == nameCount.end()) {
                    output[x.first] = referenceName;
                    nameCount[referenceName] = 1;
                } else {
                    ++(iter->second);
                    output[x.first] = referenceName+"_"+std::to_string(iter->second);
                }
            }
            return output;
        }

        void writePythonEnumDef(std::string const &typeName, MetaInformation_Enum const &info, std::unordered_map<std::string, std::string> const &assignedNames, std::map<std::string, std::tuple<std::size_t, MetaInformation>> const &unnamedTypes, std::ostream &os, std::size_t offset) {
            os << std::string(offset, ' ') << "class " << typeName << "(IntEnum):\n";
            for (auto const &item : info.EnumInfo) {
                os << std::string(offset+4, ' ') << item.CppValueName << " = " << item.Value << '\n';
            }
            os << std::string(offset+4, ' ') << "def to_cbor_value(self):\n";
            auto atStart = true;
            for (auto const &item : info.EnumInfo) {
                os << std::string(offset+8, ' ') << (atStart?"if":"elif") << " self == " << typeName << "." << item.CppValueName << ":\n";
                os << std::string(offset+12, ' ') << "return \"" << item.Name << "\"\n";
                atStart = false;
            }
            os << std::string(offset+8, ' ') << "else:\n";
            os << std::string(offset+12, ' ') << "raise ValueError\n";
            os << std::string(offset+4, ' ') << "@staticmethod\n";
            os << std::string(offset+4, ' ') << "def from_cbor_value(s):\n";
            atStart = true;
            for (auto const &item : info.EnumInfo) {
                os << std::string(offset+8, ' ') << (atStart?"if":"elif") << " s == \"" << item.Name << "\":\n";
                os << std::string(offset+12, ' ') << "return " << typeName << "." << item.CppValueName << "\n";
                atStart = false;
            }
            os << std::string(offset+8, ' ') << "else:\n";
            os << std::string(offset+12, ' ') << "raise ValueError\n";
        }
        void writePythonToCbor(std::string const &fieldName, MetaInformation const &fieldType, std::unordered_map<std::string, std::string> const &assignedNames, std::map<std::string, std::tuple<std::size_t, MetaInformation>> const &unnamedTypes, std::ostream &os) {
            std::visit([&unnamedTypes,&fieldName,&os](auto const &x) {
                using T = std::decay_t<decltype(x)>;
                if constexpr (std::is_same_v<T, MetaInformation_BuiltIn>) {
                    if (x.DataType == "std::chrono::system_clock::time_point") {
                        os << "int(round(" << fieldName << ".timestamp()*1000000))";
                    } else if (x.DataType == "std::tm") {
                        os << "{\"sec\": " << fieldName << ".second"
                            << ", \"min\": " << fieldName << ".minute"
                            << ", \"hour\": " << fieldName << ".hour"
                            << ", \"mday\": " << fieldName << ".day"
                            << ", \"mon\": " << fieldName << ".month-1"
                            << ", \"year\": " << fieldName << ".year-1900"
                            << ", \"wday\": " << fieldName << ".weekday()"
                            << ", \"yday\": " << fieldName << ".toordinal() - date(" << fieldName << ".year, 1, 1).toordinal() + 1"
                            << ", \"isdst\": " << fieldName << ".dst()"
                            << "}";
                    } else if (x.DataType == "std::monotype") {
                        os << 0;
                    } else {
                        os << fieldName;
                    }
                } else if constexpr (std::is_same_v<T, MetaInformation_TM>) {
                    if (x.DataType == "basic::DateHolder") {
                        os << "{\"year\": " << fieldName << ".year, \"month\": " << fieldName << ".month, \"day\": " << fieldName << ".day}";
                    } else if (x.DataType == "basic::FixedPrecisionShortDecimal") {
                        os << "[((-" << fieldName << ".as_tuple().exponent) if (" << fieldName << ".as_tuple().exponent < 0) else 0)"
                            << ",int((" << fieldName << "*(Decimal(10)**(-" << fieldName << ".as_tuple().exponent))) if (" << fieldName << ".as_tuple().exponent < 0) else " << fieldName << ")"
                            << "]";
                    } else {
                        //TODO
                        os << fieldName;
                    }
                } else if constexpr (std::is_same_v<T, MetaInformation_Enum>) {
                    os << fieldName << ".to_cbor_value()";
                } else if constexpr (std::is_same_v<T, MetaInformation_Tuple>) {            
                    os << "SubTypeHelpers.";
                    auto iter = unnamedTypes.find(x.TypeID);
                    os << "SubType" << std::get<0>(iter->second) << "_to_cbor_value(" << fieldName << ")";
                } else if constexpr (std::is_same_v<T, MetaInformation_Variant>) {            
                    os << "SubTypeHelpers.";
                    auto iter = unnamedTypes.find(x.TypeID);
                    os << "SubType" << std::get<0>(iter->second) << "_to_cbor_value(" << fieldName << ")";
                } else if constexpr (std::is_same_v<T, MetaInformation_Optional>) { 
                    os << "SubTypeHelpers.";
                    auto iter = unnamedTypes.find(x.TypeID);
                    os << "SubType" << std::get<0>(iter->second) << "_to_cbor_value(" << fieldName << ")";           
                } else if constexpr (std::is_same_v<T, MetaInformation_Collection>) {
                    os << "SubTypeHelpers.";
                    auto iter = unnamedTypes.find(x.TypeID);
                    os << "SubType" << std::get<0>(iter->second) << "_to_cbor_value(" << fieldName << ")";
                } else if constexpr (std::is_same_v<T, MetaInformation_Dictionary>) {
                    os << "SubTypeHelpers.";
                    auto iter = unnamedTypes.find(x.TypeID);
                    os << "SubType" << std::get<0>(iter->second) << "_to_cbor_value(" << fieldName << ")";
                } else if constexpr (std::is_same_v<T, MetaInformation_Structure>) {
                    os << fieldName << ".to_cbor_value()";
                }
            }, fieldType);
        }
        void writePythonFromCbor(std::string const &fieldName, MetaInformation const &fieldType, std::unordered_map<std::string, std::string> const &assignedNames, std::map<std::string, std::tuple<std::size_t, MetaInformation>> const &unnamedTypes, std::ostream &os) {
            std::visit([&assignedNames,&unnamedTypes,&fieldName,&os](auto const &x) {
                using T = std::decay_t<decltype(x)>;
                if constexpr (std::is_same_v<T, MetaInformation_BuiltIn>) {
                    if (x.DataType == "std::chrono::system_clock::time_point") {
                        os << "datetime.fromtimestamp(" << fieldName << "/1000000.0)";
                    } else if (x.DataType == "std::tm") {
                        os << "datetime(" << fieldName << "[\"year\"]+1900"
                            << ", " << fieldName << "[\"mon\"]+1"
                            << ", " << fieldName << "[\"mday\"]"
                            << ", " << fieldName << "[\"hour\"]"
                            << ", " << fieldName << "[\"min\"]"
                            << ", " << fieldName << "[\"sec\"]"
                            << ")";
                    } else if (x.DataType == "std::monotype") {
                        os << 0;
                    } else {
                        os << fieldName;
                    }
                } else if constexpr (std::is_same_v<T, MetaInformation_TM>) {
                    if (x.DataType == "basic::DateHolder") {
                        os << "date(" << fieldName << "[\"year\"], " << fieldName << "[\"month\"], " << fieldName << "[\"day\"])";
                    } else if (x.DataType == "basic::FixedPrecisionShortDecimal") {
                        os << "(Decimal(" << fieldName << "[1])*(Decimal(10)**(-" << fieldName << "[0])))";
                    } else {
                        //TODO
                        os << fieldName;
                    }
                } else if constexpr (std::is_same_v<T, MetaInformation_Enum>) {
                    auto iter = assignedNames.find(x.TypeID);
                    os << iter->second << ".from_cbor_value(" << fieldName << ")";
                } else if constexpr (std::is_same_v<T, MetaInformation_Tuple>) {            
                    os << "SubTypeHelpers.";
                    auto iter = unnamedTypes.find(x.TypeID);
                    os << "SubType" << std::get<0>(iter->second) << "_from_cbor_value(" << fieldName << ")";
                } else if constexpr (std::is_same_v<T, MetaInformation_Variant>) {            
                    os << "SubTypeHelpers.";
                    auto iter = unnamedTypes.find(x.TypeID);
                    os << "SubType" << std::get<0>(iter->second) << "_from_cbor_value(" << fieldName << ")";
                } else if constexpr (std::is_same_v<T, MetaInformation_Optional>) { 
                    os << "SubTypeHelpers.";
                    auto iter = unnamedTypes.find(x.TypeID);
                    os << "SubType" << std::get<0>(iter->second) << "_from_cbor_value(" << fieldName << ")";          
                } else if constexpr (std::is_same_v<T, MetaInformation_Collection>) {
                    os << "SubTypeHelpers.";
                    auto iter = unnamedTypes.find(x.TypeID);
                    os << "SubType" << std::get<0>(iter->second) << "_from_cbor_value(" << fieldName << ")";
                } else if constexpr (std::is_same_v<T, MetaInformation_Dictionary>) {
                    os << "SubTypeHelpers.";
                    auto iter = unnamedTypes.find(x.TypeID);
                    os << "SubType" << std::get<0>(iter->second) << "_from_cbor_value(" << fieldName << ")";
                } else if constexpr (std::is_same_v<T, MetaInformation_Structure>) {
                    auto iter = assignedNames.find(x.TypeID);
                    os << iter->second << ".from_cbor_value(" << fieldName << ")";
                }
            }, fieldType);
        }
        void writePythonStructureDef(std::string const &typeName, MetaInformation_Structure const &info, std::unordered_map<std::string, std::string> const &assignedNames, std::map<std::string, std::tuple<std::size_t, MetaInformation>> const &unnamedTypes, std::ostream &os, std::size_t offset) {
            os << std::string(offset, ' ') << "@add_objprint\n";
            os << std::string(offset, ' ') << "class " << typeName << ":\n";
            os << std::string(offset+4, ' ') << "def __init__(self";
            for (auto const &item : info.Fields) {
                os << ", " << item.FieldName;
            }
            os << "):\n";
            for (auto const &item : info.Fields) {
                os << std::string(offset+8, ' ') << "self." << item.FieldName << " = " << item.FieldName << '\n';
            }
            os << std::string(offset+4, ' ') << "def to_cbor_value(self):\n";
            if (info.EncDecWithFieldNames) {
                os << std::string(offset+8, ' ') << "return {\n";
                auto atBeginning = true;
                for (auto const &item : info.Fields) {
                    os << std::string(offset+12, ' ') << (atBeginning?' ':',') << " '" << item.FieldName << "': ";
                    atBeginning = false;
                    writePythonToCbor(item.FieldName, *item.FieldType, assignedNames, unnamedTypes, os);
                    os << '\n';
                }
                os << std::string(offset+8, ' ') << "}\n";
            } else {            
                os << std::string(offset+8, ' ') << "return [\n";
                auto atBeginning = true;
                for (auto const &item : info.Fields) {
                    os << std::string(offset+12, ' ') << (atBeginning?' ':',') << ' ';
                    atBeginning = false;
                    writePythonToCbor(item.FieldName, *item.FieldType, assignedNames, unnamedTypes, os);
                    os << '\n';
                }
                os << std::string(offset+8, ' ') << "]\n";
            }
            os << std::string(offset+4, ' ') << "@staticmethod\n";
            os << std::string(offset+4, ' ') << "def from_cbor_value(val):\n";            
            if (info.EncDecWithFieldNames) {            
                os << std::string(offset+8, ' ') << "return " << typeName << "(\n";
                auto atBeginning = true;
                for (auto const &item : info.Fields) {
                    os << std::string(offset+12, ' ') << (atBeginning?' ':',') << " ";
                    atBeginning = false;
                    writePythonFromCbor("val[\""+item.FieldName+"\"]", *item.FieldType, assignedNames, unnamedTypes, os);
                    os << '\n';
                }
                os << std::string(offset+8, ' ') << ")\n";
            } else {
                os << std::string(offset+8, ' ') << "return " << typeName << "(\n";
                auto atBeginning = true;
                for (auto ii=0; ii<info.Fields.size(); ++ii) {
                    os << std::string(offset+12, ' ') << (atBeginning?' ':',') << " ";
                    atBeginning = false;
                    writePythonFromCbor("val["+std::to_string(ii)+"]", *info.Fields[ii].FieldType, assignedNames, unnamedTypes, os);
                    os << '\n';
                }
                os << std::string(offset+8, ' ') << ")\n";
            }
        }
        void writePythonTupleCbor(std::string const &typeName, MetaInformation_Tuple const &info, std::unordered_map<std::string, std::string> const &assignedNames, std::map<std::string, std::tuple<std::size_t, MetaInformation>> const &unnamedTypes, std::ostream &os, std::size_t offset) {
            os << std::string(offset, ' ') << "@staticmethod\n";
            os << std::string(offset, ' ') << "def " << typeName << "_to_cbor_value(v):\n";
            os << std::string(offset+4, ' ') << "return [\n";
            for (std::size_t ii=0; ii<info.TupleContent.size(); ++ii) {
                os << std::string(offset+8, ' ') 
                    << (ii==0?"  ":", ");
                writePythonToCbor("v["+std::to_string(ii)+"]", *info.TupleContent[ii], assignedNames, unnamedTypes, os);
            }
            os << std::string(offset+4, ' ') << "]\n";
            os << std::string(offset, ' ') << "@staticmethod\n";
            os << std::string(offset, ' ') << "def " << typeName << "_from_cbor_value(v):\n";
            os << std::string(offset+4, ' ') << "return (\n";
            for (std::size_t ii=0; ii<info.TupleContent.size(); ++ii) {
                os << std::string(offset+8, ' ') 
                    << (ii==0?"  ":", ");
                writePythonFromCbor("v["+std::to_string(ii)+"]", *info.TupleContent[ii], assignedNames, unnamedTypes, os);
            }
            os << std::string(offset+4, ' ') << ")\n";
        }
        void writePythonVariantCbor(std::string const &typeName, MetaInformation_Variant const &info, std::unordered_map<std::string, std::string> const &assignedNames, std::map<std::string, std::tuple<std::size_t, MetaInformation>> const &unnamedTypes, std::ostream &os, std::size_t offset) {
            os << std::string(offset, ' ') << "@staticmethod\n";
            os << std::string(offset, ' ') << "def " << typeName << "_to_cbor_value(v):\n";
            os << std::string(offset+4, ' ') << "item2 = None\n";
            auto atStart = true;
            for (std::size_t ii=0; ii<info.VariantChoices.size(); ++ii) {
                os << std::string(offset+4, ' ') << (atStart?"if":"elif") << " v[0] == " << ii << ":\n";
                os << std::string(offset+8, ' ') << "item2 = ";
                writePythonToCbor("v[1]", *info.VariantChoices[ii], assignedNames, unnamedTypes, os);
                os << "\n";
                atStart = false;
            }
            os << std::string(offset+4, ' ') << "return [v[0], item2]\n";
            os << std::string(offset, ' ') << "@staticmethod\n";
            os << std::string(offset, ' ') << "def " << typeName << "_from_cbor_value(v):\n";
            os << std::string(offset+4, ' ') << "val = [v[0], None]\n";
            atStart = true;
            for (std::size_t ii=0; ii<info.VariantChoices.size(); ++ii) {
                os << std::string(offset+4, ' ') << (atStart?"if":"elif") << " v[0] == " << ii << ":\n";
                os << std::string(offset+8, ' ') << "val[1] = ";
                writePythonFromCbor("v[1]", *info.VariantChoices[ii], assignedNames, unnamedTypes, os);
                os << "\n";
                atStart = false;
            }
            os << std::string(offset+4, ' ') << "return val\n";
        }
        void writePythonOptionalCbor(std::string const &typeName, MetaInformation_Optional const &info, std::unordered_map<std::string, std::string> const &assignedNames, std::map<std::string, std::tuple<std::size_t, MetaInformation>> const &unnamedTypes, std::ostream &os, std::size_t offset) {
            os << std::string(offset, ' ') << "@staticmethod\n";
            os << std::string(offset, ' ') << "def " << typeName << "_to_cbor_value(v):\n";
            os << std::string(offset+4, ' ') << "if v is None:\n";
            os << std::string(offset+8, ' ') << "return []\n";
            os << std::string(offset+4, ' ') << "else:\n";
            os << std::string(offset+8, ' ') << "return [";
            writePythonToCbor("v", *info.UnderlyingType, assignedNames, unnamedTypes, os);
            os << "]\n";
            os << std::string(offset, ' ') << "@staticmethod\n";
            os << std::string(offset, ' ') << "def " << typeName << "_from_cbor_value(v):\n";
            os << std::string(offset+4, ' ') << "if len(v) == 0:\n";
            os << std::string(offset+8, ' ') << "return None\n";
            os << std::string(offset+4, ' ') << "else:\n";
            os << std::string(offset+8, ' ') << "return ";
            writePythonFromCbor("v[0]", *info.UnderlyingType, assignedNames, unnamedTypes, os);
            os << "\n";
        }
        void writePythonCollectionCbor(std::string const &typeName, MetaInformation_Collection const &info, std::unordered_map<std::string, std::string> const &assignedNames, std::map<std::string, std::tuple<std::size_t, MetaInformation>> const &unnamedTypes, std::ostream &os, std::size_t offset) {
            os << std::string(offset, ' ') << "@staticmethod\n";
            os << std::string(offset, ' ') << "def " << typeName << "_to_cbor_value(v):\n";
            os << std::string(offset+4, ' ') << "val = []\n";
            os << std::string(offset+4, ' ') << "for x in v:\n";
            os << std::string(offset+8, ' ') << "val.append(";
            writePythonToCbor("x", *info.UnderlyingType, assignedNames, unnamedTypes, os);
            os << ")\n";
            os << std::string(offset+4, ' ') << "return val\n";
            os << std::string(offset, ' ') << "@staticmethod\n";
            os << std::string(offset, ' ') << "def " << typeName << "_from_cbor_value(v):\n";
            os << std::string(offset+4, ' ') << "val = []\n";
            os << std::string(offset+4, ' ') << "for x in v:\n";
            os << std::string(offset+8, ' ') << "val.append(";
            writePythonFromCbor("x", *info.UnderlyingType, assignedNames, unnamedTypes, os);
            os << ")\n";
            os << std::string(offset+4, ' ') << "return val\n";
        }
        void writePythonDictionaryCbor(std::string const &typeName, MetaInformation_Dictionary const &info, std::unordered_map<std::string, std::string> const &assignedNames, std::map<std::string, std::tuple<std::size_t, MetaInformation>> const &unnamedTypes, std::ostream &os, std::size_t offset) {
            os << std::string(offset, ' ') << "@staticmethod\n";
            os << std::string(offset, ' ') << "def " << typeName << "_to_cbor_value(v):\n";
            os << std::string(offset+4, ' ') << "val = {}\n";
            os << std::string(offset+4, ' ') << "for x, y in v.items():\n";
            os << std::string(offset+8, ' ') << "val[";
            writePythonToCbor("x", *info.KeyType, assignedNames, unnamedTypes, os);
            os << "] = ";
            writePythonToCbor("y", *info.ValueType, assignedNames, unnamedTypes, os);
            os << "\n";
            os << std::string(offset+4, ' ') << "return val\n";
            os << std::string(offset, ' ') << "@staticmethod\n";
            os << std::string(offset, ' ') << "def " << typeName << "_from_cbor_value(v):\n";
            os << std::string(offset+4, ' ') << "val = {}\n";
            os << std::string(offset+4, ' ') << "for x, y in v.items():\n";
            os << std::string(offset+8, ' ') << "val[";
            writePythonFromCbor("x", *info.KeyType, assignedNames, unnamedTypes, os);
            os << "] = ";
            writePythonFromCbor("y", *info.ValueType, assignedNames, unnamedTypes, os);
            os << "\n";
        }
        std::optional<std::string> mainTypeID(MetaInformation const &info) {
            return std::visit([](auto const &x) -> std::optional<std::string> {
                using T = std::decay_t<decltype(x)>;
                if constexpr (std::is_same_v<T, MetaInformation_Tuple> || std::is_same_v<T, MetaInformation_Variant> || std::is_same_v<T, MetaInformation_Optional> || std::is_same_v<T, MetaInformation_Collection> || std::is_same_v<T, MetaInformation_Dictionary> || std::is_same_v<T, MetaInformation_Structure>) {
                    return x.TypeID;
                } else {
                    return std::nullopt;
                }
            }, info);
        }
        void writePythonTypeDefs(MetaInformation const &info, std::ostream &os, std::size_t offset) {
            auto namedTypes = allNamedTypes(info);
            auto assignedNames = assignTypeNames(namedTypes);
            auto unnamedTypes = allUnnamedTypes(info);
            os << std::string(offset, ' ') << "from enum import IntEnum\n";
            os << std::string(offset, ' ') << "from decimal import *\n";
            os << std::string(offset, ' ') << "from objprint import add_objprint\n\n";
            os << std::string(offset, ' ') << "class SubTypeHelpers:\n";
            for (auto const &t : unnamedTypes) {
                std::string theName = "SubType"+std::to_string(std::get<0>(t.second));
                std::visit([&assignedNames,&unnamedTypes,&os,offset,&theName](auto const &x) {
                    using T = std::decay_t<decltype(x)>;
                    if constexpr (std::is_same_v<T, MetaInformation_Tuple>) {
                        writePythonTupleCbor(theName, x, assignedNames, unnamedTypes, os, offset+4);
                    } else if constexpr (std::is_same_v<T, MetaInformation_Variant>) {
                        writePythonVariantCbor(theName, x, assignedNames, unnamedTypes, os, offset+4);
                    } else if constexpr (std::is_same_v<T, MetaInformation_Optional>) {
                        writePythonOptionalCbor(theName, x, assignedNames, unnamedTypes, os, offset+4);
                    } else if constexpr (std::is_same_v<T, MetaInformation_Collection>) {
                        writePythonCollectionCbor(theName, x, assignedNames, unnamedTypes, os, offset+4);
                    } else if constexpr (std::is_same_v<T, MetaInformation_Dictionary>) {
                        writePythonDictionaryCbor(theName, x, assignedNames, unnamedTypes, os, offset+4);
                    }
                }, std::get<1>(t.second));
            }
            for (auto const &t : namedTypes) {                
                auto theName = assignedNames[t.first];
                std::visit([&assignedNames,&unnamedTypes,&os,offset,&theName](auto const &x) {
                    using T = std::decay_t<decltype(x)>;
                    if constexpr (std::is_same_v<T, MetaInformation_Enum>) {
                        writePythonEnumDef(theName, x, assignedNames, unnamedTypes, os, offset);
                    }
                }, t.second);  
            }
            for (auto const &t : namedTypes) {                
                auto theName = assignedNames[t.first];
                std::visit([&assignedNames,&unnamedTypes,&os,offset,&theName](auto const &x) {
                    using T = std::decay_t<decltype(x)>;
                    if constexpr (std::is_same_v<T, MetaInformation_Structure>) {
                        writePythonStructureDef(theName, x, assignedNames, unnamedTypes, os, offset);
                    }
                }, t.second);  
            }      
            auto mainT = mainTypeID(info);
            if (mainT) {
                auto unnamedIter = unnamedTypes.find(*mainT);
                if (unnamedIter != unnamedTypes.end()) {
                    os << std::string(offset, ' ') << "def MainType_to_cbor_value(v):\n";
                    os << std::string(offset+4, ' ') << "return SubTypeHelpers.SubType" << std::to_string(std::get<0>(unnamedIter->second)) << "_to_cbor_value(v)\n";
                    os << std::string(offset, ' ') << "def MainType_from_cbor_value(v):\n";
                    os << std::string(offset+4, ' ') << "return SubTypeHelpers.SubType" << std::to_string(std::get<0>(unnamedIter->second)) << "_from_cbor_value(v)\n";
                } else {
                    auto namedIter = assignedNames.find(*mainT);
                    if (namedIter != assignedNames.end()) {
                        auto theName = namedIter->second;
                        os << std::string(offset, ' ') << "def MainType_to_cbor_value(v):\n";
                        os << std::string(offset+4, ' ') << "return v.to_cbor_value()\n";
                        os << std::string(offset, ' ') << "def MainType_from_cbor_value(v):\n";
                        os << std::string(offset+4, ' ') << "return " << theName << ".from_cbor_value(v)\n";
                    }
                }
            }      
        }
        void writePythonCodeHelper(MetaInformation const &info, std::ostream &os, std::size_t offset) {
            writePythonTypeDefs(info, os, offset);
        }
    }

    void MetaInformationTools::writePythonCode(MetaInformation const &info, std::ostream &os) {
        writePythonCodeHelper(info, os, 0);
    }

} } } }
