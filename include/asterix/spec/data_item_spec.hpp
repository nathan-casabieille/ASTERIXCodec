#pragma once

#include "asterix/core/types.hpp"
#include "asterix/spec/field_spec.hpp"
#include "asterix/data/data_item.hpp"
#include "asterix/utils/byte_buffer.hpp"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace asterix {

/**
 * @brief Spécification d'un Data Item ASTERIX
 * 
 * Définit la structure d'un Data Item :
 * - Structure (fixed, variable, repetitive)
 * - Longueur fixe si applicable
 * - Liste des champs
 * - Présence de bits FX pour les structures variables
 */
class DataItemSpec {
private:
    DataItemId id_;
    std::string title_;
    ItemStructure structure_;
    std::uint16_t fixed_length_;       ///< Longueur fixe en octets (pour Fixed et RepetitiveFixed)
    std::vector<FieldSpec> fields_;
    bool has_fx_bits_;                 ///< Indique si la structure utilise des bits FX
    
    /**
     * @brief Décode un Data Item à longueur fixe
     */
    std::unique_ptr<DataItem> decodeFixed(const ByteBuffer& buffer, std::size_t& offset) const;
    
    /**
     * @brief Décode un Data Item à longueur variable
     */
    std::unique_ptr<DataItem> decodeVariable(const ByteBuffer& buffer, std::size_t& offset) const;
    
    /**
     * @brief Décode un Data Item répétitif à longueur fixe
     */
    std::unique_ptr<DataItem> decodeRepetitiveFixed(const ByteBuffer& buffer, std::size_t& offset) const;
    
    /**
     * @brief Décode un Data Item répétitif à longueur variable
     */
    std::unique_ptr<DataItem> decodeRepetitiveVariable(const ByteBuffer& buffer, std::size_t& offset) const;
    
    /**
     * @brief Décode les champs d'un Data Item
     * @param reader BitReader positionné au début des champs
     * @param fields_to_decode Liste des champs à décoder
     * @return Map des champs décodés
     */
    std::unordered_map<FieldName, Field> decodeFields(
        BitReader& reader,
        const std::vector<FieldSpec>& fields_to_decode
    ) const;
    
    /**
     * @brief Calcule le nombre d'octets à lire pour une structure variable avec FX
     * @param buffer Buffer contenant les données
     * @param offset Position de départ
     * @return Nombre d'octets à lire
     */
    std::size_t calculateVariableLengthWithFX(const ByteBuffer& buffer, std::size_t offset) const;

public:
    /**
     * @brief Constructeur pour le parser XML
     */
    DataItemSpec(DataItemId id, std::string title)
        : id_(std::move(id)),
          title_(std::move(title)),
          structure_(ItemStructure::FixedLength),
          fixed_length_(0),
          has_fx_bits_(false) {}

    /**
     * @brief Constructeur par défaut
     */
    DataItemSpec()
        : structure_(ItemStructure::FixedLength),
          fixed_length_(0),
          has_fx_bits_(false) {}

    /**
     * @brief Décode un Data Item depuis un buffer
     * 
     * @param buffer Buffer contenant les données
     * @param offset Position courante (sera mise à jour)
     * @return DataItem décodé
     * @throws DecodingException si le décodage échoue
     */
    std::unique_ptr<DataItem> decode(const ByteBuffer& buffer, std::size_t& offset) const;

    // Getters
    const DataItemId& getId() const { return id_; }
    const std::string& getTitle() const { return title_; }
    ItemStructure getStructure() const { return structure_; }
    std::uint16_t getFixedLength() const { return fixed_length_; }
    const std::vector<FieldSpec>& getFields() const { return fields_; }
    bool hasFxBits() const { return has_fx_bits_; }

    // Setters pour le parser XML
    void setId(DataItemId id) { id_ = std::move(id); }
    void setTitle(std::string title) { title_ = std::move(title); }
    void setStructure(ItemStructure structure, std::uint16_t fixed_length = 0) {
        structure_ = structure;
        fixed_length_ = fixed_length;
    }
    void setFields(std::vector<FieldSpec> fields) { fields_ = std::move(fields); }
    void setHasFxBits(bool has_fx) { has_fx_bits_ = has_fx; }

    /**
     * @brief Vérifie si le Data Item a des champs définis
     */
    bool hasFields() const { return !fields_.empty(); }

    /**
     * @brief Obtient le nombre de champs
     */
    std::size_t getFieldCount() const { return fields_.size(); }

    /**
     * @brief Calcule le nombre total de bits des champs
     */
    std::size_t getTotalBitSize() const;
};

} // namespace asterix