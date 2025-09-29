#include "asterix/utils/bit_reader.hpp"
#include "asterix/utils/exceptions.hpp"
#include <sstream>

namespace asterix {

BitReader::BitReader(const ByteBuffer& buffer, std::size_t offset)
    : buffer_(buffer), byte_offset_(offset), bit_offset_(0) {
    
    if (offset > buffer.size()) {
        throw DecodingException(
            "BitReader: Invalid offset " + std::to_string(offset) + 
            " (buffer size: " + std::to_string(buffer.size()) + ")"
        );
    }
}

void BitReader::checkAvailable(std::uint8_t num_bits) const {
    std::size_t remaining = getRemainingBits();
    if (num_bits > remaining) {
        std::ostringstream oss;
        oss << "BitReader: Not enough data to read " 
            << static_cast<int>(num_bits) << " bits "
            << "(only " << remaining << " bits remaining at byte offset " 
            << byte_offset_ << ", bit offset " << static_cast<int>(bit_offset_) << ")";
        throw DecodingException(oss.str());
    }
}

std::size_t BitReader::getRemainingBits() const {
    if (byte_offset_ >= buffer_.size()) {
        return 0;
    }
    
    std::size_t remaining_bytes = buffer_.size() - byte_offset_;
    std::size_t total_bits = remaining_bytes * 8;
    
    // Soustraire les bits déjà lus dans l'octet courant
    if (total_bits >= bit_offset_) {
        total_bits -= bit_offset_;
    } else {
        return 0;
    }
    
    return total_bits;
}

bool BitReader::readBit() {
    checkAvailable(1);
    
    // Lire l'octet courant
    std::uint8_t byte = buffer_.readByte(byte_offset_);
    
    // Les bits sont numérotés de 8 (MSB) à 1 (LSB)
    // bit_offset_ va de 0 à 7, où 0 correspond au bit 8 (MSB)
    std::uint8_t bit_position = 7 - bit_offset_;
    bool bit_value = (byte >> bit_position) & 0x01;
    
    // Avancer la position
    bit_offset_++;
    if (bit_offset_ >= 8) {
        bit_offset_ = 0;
        byte_offset_++;
    }
    
    return bit_value;
}

std::uint64_t BitReader::readBits(std::uint8_t count) {
    if (count == 0) {
        return 0;
    }
    
    if (count > 64) {
        throw DecodingException(
            "BitReader: Cannot read more than 64 bits at once (requested: " + 
            std::to_string(count) + ")"
        );
    }
    
    checkAvailable(count);
    
    std::uint64_t result = 0;
    
    // Lire bit par bit
    for (std::uint8_t i = 0; i < count; ++i) {
        result <<= 1;  // Décaler à gauche pour faire de la place
        if (readBit()) {
            result |= 1;  // Mettre le bit à 1 si nécessaire
        }
    }
    
    return result;
}

std::uint64_t BitReader::readUnsigned(std::uint8_t bits) {
    return readBits(bits);
}

std::int64_t BitReader::readSigned(std::uint8_t bits) {
    if (bits == 0) {
        return 0;
    }
    
    if (bits > 64) {
        throw DecodingException(
            "BitReader: Cannot read more than 64 bits at once (requested: " + 
            std::to_string(bits) + ")"
        );
    }
    
    // Lire la valeur brute
    std::uint64_t raw_value = readBits(bits);
    
    // Vérifier le bit de signe (MSB)
    std::uint64_t sign_bit_mask = std::uint64_t(1) << (bits - 1);
    bool is_negative = (raw_value & sign_bit_mask) != 0;
    
    if (is_negative) {
        // Étendre le signe : remplir les bits de poids fort avec des 1
        // Créer un masque avec des 1 à gauche et des 0 à droite
        std::uint64_t extension_mask = ~((std::uint64_t(1) << bits) - 1);
        raw_value |= extension_mask;
    }
    
    // Convertir en int64_t par réinterprétation
    std::int64_t result;
    std::memcpy(&result, &raw_value, sizeof(result));
    
    return result;
}

void BitReader::skipBits(std::uint8_t count) {
    if (count == 0) {
        return;
    }
    
    checkAvailable(count);
    
    // Calculer la nouvelle position
    std::uint16_t total_bits = bit_offset_ + count;
    std::uint16_t bytes_to_skip = total_bits / 8;
    std::uint8_t remaining_bits = total_bits % 8;
    
    byte_offset_ += bytes_to_skip;
    bit_offset_ = remaining_bits;
}

void BitReader::alignToByte() {
    if (bit_offset_ != 0) {
        bit_offset_ = 0;
        byte_offset_++;
    }
}

std::size_t BitReader::getRemainingBytes() const {
    if (byte_offset_ >= buffer_.size()) {
        return 0;
    }
    
    std::size_t remaining = buffer_.size() - byte_offset_;
    
    // Si on est au milieu d'un octet, cet octet est partiellement consommé
    if (bit_offset_ > 0 && remaining > 0) {
        remaining--;
    }
    
    return remaining;
}

bool BitReader::hasData() const {
    return getRemainingBits() > 0;
}

void BitReader::reset(std::size_t offset) {
    if (offset > buffer_.size()) {
        throw DecodingException(
            "BitReader::reset: Invalid offset " + std::to_string(offset) + 
            " (buffer size: " + std::to_string(buffer_.size()) + ")"
        );
    }
    
    byte_offset_ = offset;
    bit_offset_ = 0;
}

} // namespace asterix