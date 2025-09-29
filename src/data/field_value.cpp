#include "asterix/data/field_value.hpp"
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace asterix {

std::uint64_t FieldValue::asUInt() const {
    try {
        return std::get<std::uint64_t>(value_);
    } catch (const std::bad_variant_access&) {
        throw std::bad_variant_access();
    }
}

std::int64_t FieldValue::asInt() const {
    try {
        return std::get<std::int64_t>(value_);
    } catch (const std::bad_variant_access&) {
        throw std::bad_variant_access();
    }
}

bool FieldValue::asBool() const {
    try {
        return std::get<bool>(value_);
    } catch (const std::bad_variant_access&) {
        throw std::bad_variant_access();
    }
}

std::string FieldValue::asEnum() const {
    try {
        return std::get<std::string>(value_);
    } catch (const std::bad_variant_access&) {
        throw std::bad_variant_access();
    }
}

std::string FieldValue::asString() const {
    try {
        return std::get<std::string>(value_);
    } catch (const std::bad_variant_access&) {
        throw std::bad_variant_access();
    }
}

std::vector<std::uint8_t> FieldValue::asRaw() const {
    try {
        return std::get<std::vector<std::uint8_t>>(value_);
    } catch (const std::bad_variant_access&) {
        throw std::bad_variant_access();
    }
}

std::string FieldValue::toString() const {
    std::ostringstream oss;

    switch (type_) {
        case FieldType::Unsigned: {
            std::uint64_t val = std::get<std::uint64_t>(value_);
            oss << val;
            // Ajouter la représentation hexadécimale pour les grandes valeurs
            if (val > 255) {
                oss << " (0x" << std::hex << std::uppercase << val << std::dec << ")";
            }
            break;
        }

        case FieldType::Signed: {
            std::int64_t val = std::get<std::int64_t>(value_);
            oss << val;
            break;
        }

        case FieldType::Boolean: {
            bool val = std::get<bool>(value_);
            oss << (val ? "true" : "false");
            break;
        }

        case FieldType::Enumeration: {
            std::string val = std::get<std::string>(value_);
            oss << val;
            break;
        }

        case FieldType::String: {
            std::string val = std::get<std::string>(value_);
            oss << "\"" << val << "\"";
            break;
        }

        case FieldType::Raw: {
            const auto& raw = std::get<std::vector<std::uint8_t>>(value_);
            oss << "[";
            for (std::size_t i = 0; i < raw.size(); ++i) {
                if (i > 0) oss << " ";
                oss << std::hex << std::uppercase << std::setw(2) 
                    << std::setfill('0') << static_cast<int>(raw[i]);
            }
            oss << "]" << std::dec;
            break;
        }

        case FieldType::Compound:
        case FieldType::Repetitive:
            oss << "(compound/repetitive - handled at DataItem level)";
            break;

        default:
            oss << "(unknown type)";
            break;
    }

    return oss.str();
}

} // namespace asterix