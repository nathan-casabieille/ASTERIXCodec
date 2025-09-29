#include "asterix/spec/field_spec.hpp"
#include "asterix/utils/exceptions.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace asterix {

FieldValue FieldSpec::decode(BitReader& reader) const {
    if (bit_size_ == 0) {
        throw DecodingException("Cannot decode field '" + name_ + "': bit size is 0");
    }

    try {
        switch (type_) {
            case FieldType::Unsigned: {
                std::uint64_t raw_value = reader.readUnsigned(bit_size_);
                
                // Appliquer le facteur d'échelle et l'offset si nécessaire
                if (scale_factor_ != 1.0 || offset_ != 0) {
                    double scaled_value = applyScaling(static_cast<std::int64_t>(raw_value));
                    // Retourner comme unsigned en stockant la valeur scalée
                    // Note: pour les valeurs scalées, on pourrait aussi retourner un double
                    // mais le FieldValue actuel ne le supporte pas selon la spec
                    return FieldValue(raw_value, type_);
                }
                
                return FieldValue(raw_value, type_);
            }

            case FieldType::Signed: {
                std::int64_t raw_value = reader.readSigned(bit_size_);
                
                // Appliquer le facteur d'échelle et l'offset si nécessaire
                if (scale_factor_ != 1.0 || offset_ != 0) {
                    double scaled_value = applyScaling(raw_value);
                    // Même remarque que pour unsigned
                    return FieldValue(raw_value, type_);
                }
                
                return FieldValue(raw_value, type_);
            }

            case FieldType::Boolean: {
                if (bit_size_ != 1) {
                    throw DecodingException("Boolean field '" + name_ + 
                        "' must have exactly 1 bit, got " + std::to_string(bit_size_));
                }
                bool value = reader.readBit();
                return FieldValue(value, type_);
            }

            case FieldType::Enumeration: {
                std::uint64_t key = reader.readUnsigned(bit_size_);
                std::string enum_str = getEnumValue(key);
                return FieldValue(enum_str, type_);
            }

            case FieldType::String: {
                // Lire les bits et les convertir en caractères ASCII
                std::size_t num_bytes = (bit_size_ + 7) / 8;
                std::string str;
                str.reserve(num_bytes);
                
                for (std::size_t i = 0; i < num_bytes; ++i) {
                    std::uint64_t byte_value = reader.readUnsigned(8);
                    // Filtrer les caractères non imprimables
                    if (byte_value >= 32 && byte_value <= 126) {
                        str.push_back(static_cast<char>(byte_value));
                    } else if (byte_value == 0) {
                        // Null terminator - arrêter la lecture
                        // Mais continuer à lire les bits restants pour aligner
                        reader.skipBits((num_bytes - i - 1) * 8);
                        break;
                    } else {
                        str.push_back('?'); // Caractère de remplacement
                    }
                }
                
                return FieldValue(str, type_);
            }

            case FieldType::Raw: {
                // Lire les bits bruts et les stocker dans un vecteur d'octets
                std::size_t num_bytes = (bit_size_ + 7) / 8;
                std::vector<std::uint8_t> raw_data;
                raw_data.reserve(num_bytes);
                
                for (std::size_t i = 0; i < num_bytes; ++i) {
                    std::uint8_t bits_to_read = (i == num_bytes - 1 && bit_size_ % 8 != 0) 
                        ? (bit_size_ % 8) 
                        : 8;
                    std::uint64_t byte_value = reader.readUnsigned(bits_to_read);
                    raw_data.push_back(static_cast<std::uint8_t>(byte_value));
                }
                
                return FieldValue(raw_data, type_);
            }

            case FieldType::Compound:
            case FieldType::Repetitive:
                throw DecodingException("Cannot decode field '" + name_ + 
                    "': Compound and Repetitive types are handled at DataItemSpec level");

            default:
                throw DecodingException("Unknown field type for field '" + name_ + "'");
        }
    } catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "Failed to decode field '" << name_ << "' (type: ";
        
        switch (type_) {
            case FieldType::Unsigned: oss << "unsigned"; break;
            case FieldType::Signed: oss << "signed"; break;
            case FieldType::Boolean: oss << "boolean"; break;
            case FieldType::Enumeration: oss << "enum"; break;
            case FieldType::String: oss << "string"; break;
            case FieldType::Raw: oss << "raw"; break;
            case FieldType::Compound: oss << "compound"; break;
            case FieldType::Repetitive: oss << "repetitive"; break;
        }
        
        oss << ", " << static_cast<int>(bit_size_) << " bits): " << e.what();
        throw DecodingException(oss.str());
    }
}

double FieldSpec::applyScaling(std::int64_t raw_value) const {
    // Formule: valeur_physique = (valeur_brute * facteur_échelle) + offset
    return (static_cast<double>(raw_value) * scale_factor_) + static_cast<double>(offset_);
}

std::string FieldSpec::getEnumValue(std::uint64_t key) const {
    auto it = enum_values_.find(key);
    if (it != enum_values_.end()) {
        return it->second;
    }
    
    // Si la valeur n'est pas dans l'énumération, retourner une représentation
    std::ostringstream oss;
    oss << "Unknown(" << key << ")";
    return oss.str();
}

} // namespace asterix