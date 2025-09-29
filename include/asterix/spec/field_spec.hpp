#pragma once

#include "asterix/core/types.hpp"
#include "asterix/data/field_value.hpp"
#include "asterix/utils/bit_reader.hpp"
#include <string>
#include <unordered_map>
#include <cstdint>
#include <vector>

namespace asterix {

/**
 * @brief Spécification d'un champ de données
 * 
 * Définit comment décoder un champ particulier dans un Data Item :
 * - Type de données (unsigned, signed, boolean, enum, string, raw)
 * - Taille en bits
 * - Unité de mesure
 * - Facteur d'échelle et offset pour les conversions
 * - Valeurs d'énumération si applicable
 */
class FieldSpec {
private:
    FieldName name_;
    FieldType type_;
    std::uint8_t bit_size_;
    std::string unit_;
    double scale_factor_;
    std::int64_t offset_;
    std::unordered_map<std::uint64_t, std::string> enum_values_;
    
    /**
     * @brief Applique le facteur d'échelle et l'offset à une valeur
     * @param raw_value Valeur brute lue
     * @return Valeur convertie
     */
    double applyScaling(std::int64_t raw_value) const;
    
    /**
     * @brief Obtient la valeur d'énumération correspondant à une clé
     * @param key Clé de l'énumération
     * @return Valeur de l'énumération ou représentation de la clé si inconnue
     */
    std::string getEnumValue(std::uint64_t key) const;

public:
    /**
     * @brief Constructeur complet
     */
    FieldSpec(FieldName name, FieldType type, std::uint8_t bit_size, std::string unit)
        : name_(std::move(name)), 
          type_(type), 
          bit_size_(bit_size), 
          unit_(std::move(unit)),
          scale_factor_(1.0),
          offset_(0) {}

    /**
     * @brief Constructeur par défaut
     */
    FieldSpec() 
        : type_(FieldType::Unsigned), 
          bit_size_(0), 
          scale_factor_(1.0), 
          offset_(0) {}

    /**
     * @brief Décode un champ depuis un BitReader
     * 
     * Lit les bits nécessaires et convertit la valeur selon le type de champ.
     * Applique le facteur d'échelle et l'offset si nécessaire.
     * 
     * @param reader BitReader positionné au début du champ
     * @return FieldValue contenant la valeur décodée
     * @throws DecodingException si le décodage échoue
     */
    FieldValue decode(BitReader& reader) const;

    // Getters
    const FieldName& getName() const { return name_; }
    FieldType getType() const { return type_; }
    std::uint8_t getBitSize() const { return bit_size_; }
    const std::string& getUnit() const { return unit_; }
    double getScaleFactor() const { return scale_factor_; }
    std::int64_t getOffset() const { return offset_; }
    const std::unordered_map<std::uint64_t, std::string>& getEnumValues() const { 
        return enum_values_; 
    }

    // Setters pour la construction par le parser XML
    void setName(FieldName name) { name_ = std::move(name); }
    void setType(FieldType type) { type_ = type; }
    void setBitSize(std::uint8_t size) { bit_size_ = size; }
    void setUnit(std::string unit) { unit_ = std::move(unit); }
    void setScaleFactor(double factor) { scale_factor_ = factor; }
    void setOffset(std::int64_t offset) { offset_ = offset; }
    void setEnumValues(std::unordered_map<std::uint64_t, std::string> values) { 
        enum_values_ = std::move(values); 
    }

    /**
     * @brief Vérifie si le champ a des valeurs d'énumération définies
     */
    bool hasEnumValues() const { return !enum_values_.empty(); }

    /**
     * @brief Vérifie si une clé d'énumération est valide
     */
    bool isValidEnumKey(std::uint64_t key) const {
        return enum_values_.find(key) != enum_values_.end();
    }

    /**
     * @brief Calcule la taille du champ en octets (arrondi supérieur)
     */
    std::size_t getSizeInBytes() const {
        return (bit_size_ + 7) / 8;
    }
};

} // namespace asterix