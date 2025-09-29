#include "asterix/core/types.hpp"
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace asterix {

std::string fieldTypeToString(FieldType type) {
    switch (type) {
        case FieldType::Unsigned:
            return "unsigned";
        case FieldType::Signed:
            return "signed";
        case FieldType::Boolean:
            return "boolean";
        case FieldType::Enumeration:
            return "enumeration";
        case FieldType::String:
            return "string";
        case FieldType::Raw:
            return "raw";
        case FieldType::Compound:
            return "compound";
        case FieldType::Repetitive:
            return "repetitive";
        default:
            return "unknown";
    }
}

FieldType stringToFieldType(const std::string& type_str) {
    // Convertir en minuscules pour comparaison insensible à la casse
    std::string lower_str = type_str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    if (lower_str == "unsigned" || lower_str == "uint") {
        return FieldType::Unsigned;
    } else if (lower_str == "signed" || lower_str == "int") {
        return FieldType::Signed;
    } else if (lower_str == "boolean" || lower_str == "bool") {
        return FieldType::Boolean;
    } else if (lower_str == "enumeration" || lower_str == "enum") {
        return FieldType::Enumeration;
    } else if (lower_str == "string" || lower_str == "str") {
        return FieldType::String;
    } else if (lower_str == "raw" || lower_str == "bytes") {
        return FieldType::Raw;
    } else if (lower_str == "compound") {
        return FieldType::Compound;
    } else if (lower_str == "repetitive" || lower_str == "rep") {
        return FieldType::Repetitive;
    } else {
        throw std::invalid_argument("Unknown field type: " + type_str);
    }
}

std::string itemStructureToString(ItemStructure structure) {
    switch (structure) {
        case ItemStructure::FixedLength:
            return "fixed_length";
        case ItemStructure::VariableLength:
            return "variable_length";
        case ItemStructure::RepetitiveFixed:
            return "repetitive_fixed";
        case ItemStructure::RepetitiveVariable:
            return "repetitive_variable";
        default:
            return "unknown";
    }
}

ItemStructure stringToItemStructure(const std::string& structure_str) {
    // Convertir en minuscules pour comparaison insensible à la casse
    std::string lower_str = structure_str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    if (lower_str == "fixed_length" || lower_str == "fixed") {
        return ItemStructure::FixedLength;
    } else if (lower_str == "variable_length" || lower_str == "variable") {
        return ItemStructure::VariableLength;
    } else if (lower_str == "repetitive_fixed" || lower_str == "rep_fixed") {
        return ItemStructure::RepetitiveFixed;
    } else if (lower_str == "repetitive_variable" || lower_str == "rep_variable") {
        return ItemStructure::RepetitiveVariable;
    } else {
        throw std::invalid_argument("Unknown item structure: " + structure_str);
    }
}

} // namespace asterix