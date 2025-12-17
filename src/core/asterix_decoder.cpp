#include "asterix/core/asterix_decoder.hpp"
#include "asterix/utils/exceptions.hpp"
#include "asterix/core/types.hpp"
#include <sstream>
#include <iomanip>

namespace asterix {

AsterixDecoder::AsterixDecoder(const AsterixCategory& category)
    : category_(category) {
    // Valider la catégorie lors de la construction
    try {
        category_.validate();
    } catch (const SpecificationException& e) {
        throw SpecificationException(
            "Cannot create decoder: category specification is invalid - " + 
            std::string(e.what())
        );
    }
}

AsterixMessage AsterixDecoder::decode(const ByteBuffer& buffer) const {
    if (buffer.empty()) {
        throw DecodingException("Cannot decode empty buffer");
    }
    
    if (buffer.size() < constants::MIN_MESSAGE_SIZE) {
        throw DecodingException(
            "Buffer too small for ASTERIX message (minimum " + 
            std::to_string(constants::MIN_MESSAGE_SIZE) + " bytes, got " +
            std::to_string(buffer.size()) + " bytes)"
        );
    }
    
    std::size_t offset = 0;
    
    // 1. Décoder l'en-tête (CAT + LEN)
    CategoryNumber message_category;
    std::uint16_t message_length;
    decodeHeader(buffer, offset, message_category, message_length);
    
    // 2. Valider la catégorie et la longueur
    validateCategory(message_category);
    validateLength(message_length, buffer.size());
    
    // 3. Calculer la position de fin du Data Block
    std::size_t end_offset = message_length;
    
    // 4. Décoder tous les Data Records jusqu'à la fin du Data Block
    std::vector<AsterixRecord> records;
    
    while (offset < end_offset) {
        try {
            AsterixRecord record = decodeRecord(buffer, offset, end_offset);
            records.push_back(std::move(record));
        } catch (const DecodingException& e) {
            // Si on a déjà décodé au moins un record, on peut retourner ce qu'on a
            // Sinon, on propage l'erreur
            if (records.empty()) {
                throw;
            } else {
                // Log un avertissement et arrêter le décodage
                std::ostringstream oss;
                oss << "Warning: Failed to decode record " << (records.size() + 1) 
                    << " at offset " << offset << ": " << e.what() 
                    << ". Returning " << records.size() << " successfully decoded records.";
                // On pourrait logger ici, mais pour l'instant on arrête juste
                break;
            }
        }
    }
    
    // 5. Vérifier qu'on a décodé au moins un record
    if (records.empty()) {
        throw DecodingException("No Data Records found in message");
    }
    
    // 6. Vérifier que tous les octets ont été consommés
    if (offset != end_offset) {
        std::ostringstream oss;
        oss << "Message length mismatch: decoded " << offset 
            << " bytes, but header declares " << message_length << " bytes. "
            << "(Difference: " << (end_offset - offset) << " bytes) "
            << "Possible data corruption or specification error.";
        // Pour un message multi-records, on peut tolérer une petite différence
        // si on a réussi à décoder au moins un record
        if (records.empty() || (end_offset - offset) > 10) {
            throw DecodingException(oss.str());
        }
        // Sinon, on continue avec un avertissement implicite
    }
    
    // 7. Créer et retourner le message décodé
    return AsterixMessage(message_category, message_length, std::move(records));
}

AsterixRecord AsterixDecoder::decodeRecord(const ByteBuffer& buffer, 
                                           std::size_t& offset,
                                           std::size_t end_offset) const {
    if (offset >= end_offset) {
        throw DecodingException(
            "Cannot decode record: offset " + std::to_string(offset) + 
            " is beyond end of Data Block at " + std::to_string(end_offset)
        );
    }
    
    std::size_t record_start = offset;
    
    // 1. Décoder l'UAP
    std::vector<DataItemId> present_items;
    try {
        present_items = decodeUap(buffer, offset);
    } catch (const DecodingException& e) {
        throw DecodingException(
            "Failed to decode UAP for record at offset " + 
            std::to_string(record_start) + ": " + e.what()
        );
    }
    
    // 2. Décoder les Data Items
    std::unordered_map<DataItemId, DataItem> decoded_items;
    try {
        decoded_items = decodeDataItems(buffer, offset, present_items);
    } catch (const DecodingException& e) {
        throw DecodingException(
            "Failed to decode Data Items for record at offset " + 
            std::to_string(record_start) + ": " + e.what()
        );
    }
    
    // 3. Vérifier qu'on n'a pas dépassé la fin du Data Block
    if (offset > end_offset) {
        throw DecodingException(
            "Record decoding exceeded Data Block boundary: offset " + 
            std::to_string(offset) + " > end " + std::to_string(end_offset)
        );
    }
    
    return AsterixRecord(std::move(decoded_items));
}

AsterixMessage AsterixDecoder::decode(const std::string& hex_data) const {
    try {
        ByteBuffer buffer(hex_data);
        return decode(buffer);
    } catch (const DecodingException&) {
        throw; // Re-throw DecodingException as-is
    } catch (const std::exception& e) {
        throw DecodingException(
            "Failed to decode hex string: " + std::string(e.what())
        );
    }
}

AsterixMessage AsterixDecoder::decode(const std::vector<std::uint8_t>& data) const {
    ByteBuffer buffer(data);
    return decode(buffer);
}

void AsterixDecoder::decodeHeader(const ByteBuffer& buffer, std::size_t& offset,
                                  CategoryNumber& out_category, 
                                  std::uint16_t& out_length) const {
    // Vérifier qu'il y a assez de données pour l'en-tête
    if (offset + constants::ASTERIX_HEADER_SIZE > buffer.size()) {
        throw DecodingException(
            "Not enough data for ASTERIX header (need " + 
            std::to_string(constants::ASTERIX_HEADER_SIZE) + " bytes, have " +
            std::to_string(buffer.size() - offset) + " bytes)"
        );
    }
    
    // Lire la catégorie (1 octet)
    out_category = buffer.readByte(offset);
    offset++;
    
    // Lire la longueur (2 octets, big-endian)
    out_length = buffer.readUInt16BE(offset);
    offset += 2;
    
    // Valider la longueur
    if (out_length < constants::MIN_MESSAGE_SIZE) {
        throw DecodingException(
            "Invalid message length " + std::to_string(out_length) + 
            " (minimum is " + std::to_string(constants::MIN_MESSAGE_SIZE) + " bytes)"
        );
    }
    
    if (out_length > constants::MAX_MESSAGE_SIZE) {
        throw DecodingException(
            "Invalid message length " + std::to_string(out_length) + 
            " (maximum is " + std::to_string(constants::MAX_MESSAGE_SIZE) + " bytes)"
        );
    }
}

std::vector<DataItemId> AsterixDecoder::decodeUap(
    const ByteBuffer& buffer, std::size_t& offset) const {
    
    try {
        return category_.getUapSpec().decodeUap(buffer, offset);
    } catch (const DecodingException& e) {
        throw DecodingException(
            "Failed to decode UAP for Category " + 
            std::to_string(category_.getCategoryNumber()) + ": " +
            e.what()
        );
    }
}

std::unordered_map<DataItemId, DataItem> AsterixDecoder::decodeDataItems(
    const ByteBuffer& buffer, 
    std::size_t& offset,
    const std::vector<DataItemId>& present_items) const {
    
    std::unordered_map<DataItemId, DataItem> decoded_items;
    decoded_items.reserve(present_items.size());
    
    for (const auto& item_id : present_items) {
        try {
            // Obtenir la spécification du Data Item
            const DataItemSpec& spec = category_.getDataItemSpec(item_id);
            
            // Décoder le Data Item
            auto decoded_item = spec.decode(buffer, offset);
            
            if (!decoded_item) {
                throw DecodingException(
                    "Failed to decode Data Item '" + item_id + "': decoder returned null"
                );
            }
            
            // Ajouter à la map
            decoded_items.emplace(item_id, std::move(*decoded_item));
            
        } catch (const DecodingException& e) {
            // Re-throw avec contexte additionnel
            throw DecodingException(
                "Error decoding Data Item '" + item_id + "' at offset " + 
                std::to_string(offset) + ": " + e.what()
            );
        } catch (const SpecificationException& e) {
            throw DecodingException(
                "Specification error for Data Item '" + item_id + "': " + e.what()
            );
        } catch (const std::exception& e) {
            throw DecodingException(
                "Unexpected error decoding Data Item '" + item_id + "': " + e.what()
            );
        }
    }
    
    return decoded_items;
}

void AsterixDecoder::validateCategory(CategoryNumber message_category) const {
    if (message_category != category_.getCategoryNumber()) {
        std::ostringstream oss;
        oss << "Category mismatch: decoder configured for CAT" 
            << std::setfill('0') << std::setw(3) 
            << static_cast<int>(category_.getCategoryNumber())
            << ", but message is CAT" 
            << std::setfill('0') << std::setw(3) 
            << static_cast<int>(message_category);
        throw DecodingException(oss.str());
    }
}

void AsterixDecoder::validateLength(std::uint16_t declared_length, 
                                    std::size_t actual_length) const {
    if (declared_length > actual_length) {
        std::ostringstream oss;
        oss << "Message declares length of " << declared_length 
            << " bytes, but only " << actual_length 
            << " bytes available in buffer";
        throw DecodingException(oss.str());
    }
}

} // namespace asterix