#include "asterix/data/field.hpp"
#include <sstream>

namespace asterix {

std::string Field::toString(bool include_unit) const {
    std::ostringstream oss;
    
    oss << value_.toString();
    
    if (include_unit && hasUnit()) {
        oss << " " << unit_;
    }
    
    return oss.str();
}

std::string Field::toDetailedString() const {
    std::ostringstream oss;
    
    oss << name_ << ": " << value_.toString();
    
    if (hasUnit()) {
        oss << " " << unit_;
    }
    
    // Ajouter le type entre parenthèses
    oss << " (";
    switch (value_.getType()) {
        case FieldType::Unsigned:
            oss << "unsigned";
            break;
        case FieldType::Signed:
            oss << "signed";
            break;
        case FieldType::Boolean:
            oss << "boolean";
            break;
        case FieldType::Enumeration:
            oss << "enum";
            break;
        case FieldType::String:
            oss << "string";
            break;
        case FieldType::Raw:
            oss << "raw";
            break;
        case FieldType::Compound:
            oss << "compound";
            break;
        case FieldType::Repetitive:
            oss << "repetitive";
            break;
        default:
            oss << "unknown";
            break;
    }
    oss << ")";
    
    return oss.str();
}

} // namespace asterix