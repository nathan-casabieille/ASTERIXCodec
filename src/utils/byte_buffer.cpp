#include "asterix/utils/byte_buffer.hpp"
#include "asterix/utils/exceptions.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace asterix {

// ============================================================================
// Constructeurs
// ============================================================================

ByteBuffer::ByteBuffer(std::vector<std::uint8_t> data)
    : data_(std::move(data)) {
}

ByteBuffer::ByteBuffer(const std::string& hex_string)
    : data_(hexStringToBytes(hex_string)) {
}

ByteBuffer::ByteBuffer(const std::uint8_t* data, std::size_t size)
    : data_(data, data + size) {
}

// ============================================================================
// Méthodes de lecture
// ============================================================================

void ByteBuffer::checkBounds(std::size_t offset, std::size_t length) const {
    if (offset + length > data_.size()) {
        std::ostringstream oss;
        oss << "ByteBuffer: Read out of bounds (offset: " << offset
            << ", length: " << length
            << ", buffer size: " << data_.size() << ")";
        throw DecodingException(oss.str());
    }
}

std::uint8_t ByteBuffer::readByte(std::size_t offset) const {
    checkBounds(offset, 1);
    return data_[offset];
}

std::uint16_t ByteBuffer::readUInt16BE(std::size_t offset) const {
    checkBounds(offset, 2);
    
    // Big-endian: MSB first
    std::uint16_t value = 0;
    value |= static_cast<std::uint16_t>(data_[offset]) << 8;
    value |= static_cast<std::uint16_t>(data_[offset + 1]);
    
    return value;
}

std::uint32_t ByteBuffer::readUInt24BE(std::size_t offset) const {
    checkBounds(offset, 3);
    
    // Big-endian: MSB first
    std::uint32_t value = 0;
    value |= static_cast<std::uint32_t>(data_[offset]) << 16;
    value |= static_cast<std::uint32_t>(data_[offset + 1]) << 8;
    value |= static_cast<std::uint32_t>(data_[offset + 2]);
    
    return value;
}

std::uint32_t ByteBuffer::readUInt32BE(std::size_t offset) const {
    checkBounds(offset, 4);
    
    // Big-endian: MSB first
    std::uint32_t value = 0;
    value |= static_cast<std::uint32_t>(data_[offset]) << 24;
    value |= static_cast<std::uint32_t>(data_[offset + 1]) << 16;
    value |= static_cast<std::uint32_t>(data_[offset + 2]) << 8;
    value |= static_cast<std::uint32_t>(data_[offset + 3]);
    
    return value;
}

std::uint64_t ByteBuffer::readUInt64BE(std::size_t offset) const {
    checkBounds(offset, 8);
    
    // Big-endian: MSB first
    std::uint64_t value = 0;
    value |= static_cast<std::uint64_t>(data_[offset]) << 56;
    value |= static_cast<std::uint64_t>(data_[offset + 1]) << 48;
    value |= static_cast<std::uint64_t>(data_[offset + 2]) << 40;
    value |= static_cast<std::uint64_t>(data_[offset + 3]) << 32;
    value |= static_cast<std::uint64_t>(data_[offset + 4]) << 24;
    value |= static_cast<std::uint64_t>(data_[offset + 5]) << 16;
    value |= static_cast<std::uint64_t>(data_[offset + 6]) << 8;
    value |= static_cast<std::uint64_t>(data_[offset + 7]);
    
    return value;
}

std::vector<std::uint8_t> ByteBuffer::readBytes(std::size_t offset, std::size_t length) const {
    checkBounds(offset, length);
    
    std::vector<std::uint8_t> result;
    result.reserve(length);
    
    for (std::size_t i = 0; i < length; ++i) {
        result.push_back(data_[offset + i]);
    }
    
    return result;
}

// ============================================================================
// Utilitaires
// ============================================================================

std::string ByteBuffer::toHexString(bool with_spaces) const {
    return bytesToHexString(data_, with_spaces);
}

ByteBuffer ByteBuffer::slice(std::size_t offset, std::size_t length) const {
    if (offset > data_.size()) {
        throw DecodingException(
            "ByteBuffer::slice: offset " + std::to_string(offset) +
            " exceeds buffer size " + std::to_string(data_.size())
        );
    }
    
    // Si length == 0, prendre jusqu'à la fin
    if (length == 0) {
        length = data_.size() - offset;
    }
    
    if (offset + length > data_.size()) {
        throw DecodingException(
            "ByteBuffer::slice: slice [" + std::to_string(offset) + ", " +
            std::to_string(offset + length) + ") exceeds buffer size " +
            std::to_string(data_.size())
        );
    }
    
    std::vector<std::uint8_t> sliced_data(
        data_.begin() + offset,
        data_.begin() + offset + length
    );
    
    return ByteBuffer(std::move(sliced_data));
}

// ============================================================================
// Fonctions utilitaires globales
// ============================================================================

std::vector<std::uint8_t> hexStringToBytes(const std::string& hex_string) {
    std::vector<std::uint8_t> bytes;
    
    // Retirer les espaces et valider les caractères
    std::string cleaned;
    cleaned.reserve(hex_string.size());
    
    for (char c : hex_string) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            continue;  // Ignorer les espaces
        }
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            throw DecodingException(
                "Invalid hexadecimal character: '" + std::string(1, c) + "'"
            );
        }
        cleaned.push_back(c);
    }
    
    // Vérifier que la longueur est paire
    if (cleaned.size() % 2 != 0) {
        throw DecodingException(
            "Hexadecimal string must have an even number of characters (got " +
            std::to_string(cleaned.size()) + ")"
        );
    }
    
    // Convertir par paires
    bytes.reserve(cleaned.size() / 2);
    
    for (std::size_t i = 0; i < cleaned.size(); i += 2) {
        std::string byte_str = cleaned.substr(i, 2);
        
        try {
            std::uint8_t byte = static_cast<std::uint8_t>(
                std::stoul(byte_str, nullptr, 16)
            );
            bytes.push_back(byte);
        } catch (const std::exception& e) {
            throw DecodingException(
                "Failed to convert hex string '" + byte_str + "' to byte: " +
                e.what()
            );
        }
    }
    
    return bytes;
}

std::string bytesToHexString(const std::vector<std::uint8_t>& bytes, bool with_spaces) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0');
    
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (i > 0 && with_spaces) {
            oss << ' ';
        }
        oss << std::setw(2) << static_cast<int>(bytes[i]);
    }
    
    return oss.str();
}

} // namespace asterix