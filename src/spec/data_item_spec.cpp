#include "asterix/spec/data_item_spec.hpp"
#include "asterix/data/field.hpp"
#include "asterix/utils/bit_reader.hpp"
#include "asterix/utils/exceptions.hpp"
#include <sstream>

namespace asterix {

std::unique_ptr<DataItem> DataItemSpec::decode(const ByteBuffer& buffer, std::size_t& offset) const {
    if (offset >= buffer.size()) {
        throw DecodingException("Cannot decode Data Item '" + id_ + 
            "': offset beyond buffer size");
    }

    try {
        switch (structure_) {
            case ItemStructure::FixedLength:
                return decodeFixed(buffer, offset);
            
            case ItemStructure::VariableLength:
                return decodeVariable(buffer, offset);
            
            case ItemStructure::RepetitiveFixed:
                return decodeRepetitiveFixed(buffer, offset);
            
            case ItemStructure::RepetitiveVariable:
                return decodeRepetitiveVariable(buffer, offset);
            
            default:
                throw DecodingException("Unknown item structure for Data Item '" + id_ + "'");
        }
    } catch (const DecodingException&) {
        throw; // Re-throw DecodingException as-is
    } catch (const std::exception& e) {
        throw DecodingException("Failed to decode Data Item '" + id_ + "': " + e.what());
    }
}

std::unique_ptr<DataItem> DataItemSpec::decodeFixed(const ByteBuffer& buffer, std::size_t& offset) const {
    // Vérifier qu'il y a assez de données
    if (offset + fixed_length_ > buffer.size()) {
        throw DecodingException("Not enough data to decode fixed-length Data Item '" + id_ + 
            "': need " + std::to_string(fixed_length_) + " bytes, have " + 
            std::to_string(buffer.size() - offset));
    }

    // Créer un BitReader pour lire les champs
    BitReader reader(buffer, offset);
    
    // Décoder tous les champs
    auto decoded_fields = decodeFields(reader, fields_);
    
    // Avancer l'offset
    offset += fixed_length_;
    
    // Créer et retourner le DataItem
    return std::make_unique<DataItem>(id_, title_, std::move(decoded_fields));
}

std::unique_ptr<DataItem> DataItemSpec::decodeVariable(const ByteBuffer& buffer, std::size_t& offset) const {
    std::size_t start_offset = offset;
    std::size_t item_length;
    
    if (has_fx_bits_) {
        // Structure variable avec bits FX
        item_length = calculateVariableLengthWithFX(buffer, offset);
    } else {
        // Structure variable classique : premier octet = longueur
        if (offset >= buffer.size()) {
            throw DecodingException("Cannot read length byte for variable Data Item '" + id_ + "'");
        }
        
        item_length = buffer.readByte(offset);
        offset++;
        
        if (item_length == 0) {
            throw DecodingException("Variable Data Item '" + id_ + "' has zero length");
        }
        
        // La longueur inclut l'octet de longueur lui-même
        item_length--; // Soustraire l'octet de longueur pour obtenir la longueur des données
    }
    
    // Vérifier qu'il y a assez de données
    if (offset + item_length > buffer.size()) {
        throw DecodingException("Not enough data for variable Data Item '" + id_ + 
            "': need " + std::to_string(item_length) + " bytes, have " + 
            std::to_string(buffer.size() - offset));
    }
    
    // Créer un BitReader
    BitReader reader(buffer, offset);
    
    // Décoder les champs
    auto decoded_fields = decodeFields(reader, fields_);
    
    // Avancer l'offset
    offset += item_length;
    
    return std::make_unique<DataItem>(id_, title_, std::move(decoded_fields));
}

std::unique_ptr<DataItem> DataItemSpec::decodeRepetitiveFixed(
    const ByteBuffer& buffer, std::size_t& offset) const {
    
    // Lire le REP (nombre de répétitions) - 1 octet
    if (offset >= buffer.size()) {
        throw DecodingException("Cannot read REP byte for repetitive Data Item '" + id_ + "'");
    }
    
    std::uint8_t rep_count = buffer.readByte(offset);
    offset++;
    
    if (rep_count == 0) {
        // Pas de répétitions - retourner un DataItem vide
        return std::make_unique<DataItem>(id_, title_, 
            std::unordered_map<FieldName, Field>(), std::vector<DataItem>());
    }
    
    std::vector<DataItem> repetitions;
    repetitions.reserve(rep_count);
    
    // Décoder chaque répétition
    for (std::uint8_t i = 0; i < rep_count; ++i) {
        // Vérifier qu'il y a assez de données
        if (offset + fixed_length_ > buffer.size()) {
            throw DecodingException("Not enough data for repetition " + std::to_string(i + 1) + 
                " of Data Item '" + id_ + "'");
        }
        
        BitReader reader(buffer, offset);
        auto decoded_fields = decodeFields(reader, fields_);
        
        offset += fixed_length_;
        
        repetitions.emplace_back(id_, title_, std::move(decoded_fields));
    }
    
    return std::make_unique<DataItem>(id_, title_, 
        std::unordered_map<FieldName, Field>(), std::move(repetitions));
}

std::unique_ptr<DataItem> DataItemSpec::decodeRepetitiveVariable(
    const ByteBuffer& buffer, std::size_t& offset) const {
    
    // Lire le REP (nombre de répétitions) - 1 octet
    if (offset >= buffer.size()) {
        throw DecodingException("Cannot read REP byte for repetitive Data Item '" + id_ + "'");
    }
    
    std::uint8_t rep_count = buffer.readByte(offset);
    offset++;
    
    if (rep_count == 0) {
        return std::make_unique<DataItem>(id_, title_, 
            std::unordered_map<FieldName, Field>(), std::vector<DataItem>());
    }
    
    std::vector<DataItem> repetitions;
    repetitions.reserve(rep_count);
    
    // Décoder chaque répétition
    for (std::uint8_t i = 0; i < rep_count; ++i) {
        // Chaque répétition commence par sa longueur
        if (offset >= buffer.size()) {
            throw DecodingException("Cannot read length for repetition " + 
                std::to_string(i + 1) + " of Data Item '" + id_ + "'");
        }
        
        std::uint8_t rep_length = buffer.readByte(offset);
        offset++;
        
        if (rep_length == 0) {
            throw DecodingException("Repetition " + std::to_string(i + 1) + 
                " of Data Item '" + id_ + "' has zero length");
        }
        
        rep_length--; // Soustraire l'octet de longueur
        
        if (offset + rep_length > buffer.size()) {
            throw DecodingException("Not enough data for repetition " + 
                std::to_string(i + 1) + " of Data Item '" + id_ + "'");
        }
        
        BitReader reader(buffer, offset);
        auto decoded_fields = decodeFields(reader, fields_);
        
        offset += rep_length;
        
        repetitions.emplace_back(id_, title_, std::move(decoded_fields));
    }
    
    return std::make_unique<DataItem>(id_, title_, 
        std::unordered_map<FieldName, Field>(), std::move(repetitions));
}

std::unordered_map<FieldName, Field> DataItemSpec::decodeFields(
    BitReader& reader,
    const std::vector<FieldSpec>& fields_to_decode) const {
    
    std::unordered_map<FieldName, Field> decoded_fields;
    
    for (const auto& field_spec : fields_to_decode) {
        try {
            FieldValue value = field_spec.decode(reader);
            decoded_fields.emplace(
                field_spec.getName(),
                Field(field_spec.getName(), std::move(value), field_spec.getUnit())
            );
        } catch (const std::exception& e) {
            throw DecodingException("Failed to decode field '" + field_spec.getName() + 
                "' in Data Item '" + id_ + "': " + e.what());
        }
    }
    
    return decoded_fields;
}

std::size_t DataItemSpec::calculateVariableLengthWithFX(
    const ByteBuffer& buffer, std::size_t offset) const {
    
    std::size_t length = 0;
    bool has_fx = true;
    
    // Compter les octets en suivant les bits FX
    while (has_fx) {
        if (offset + length >= buffer.size()) {
            throw DecodingException("Unexpected end of buffer while reading FX bits for Data Item '" + id_ + "'");
        }
        
        std::uint8_t byte = buffer.readByte(offset + length);
        length++;
        
        // Le bit FX est le bit 1 (LSB)
        has_fx = (byte & 0x01) != 0;
    }
    
    return length;
}

std::size_t DataItemSpec::getTotalBitSize() const {
    std::size_t total = 0;
    for (const auto& field : fields_) {
        total += field.getBitSize();
    }
    return total;
}

} // namespace asterix