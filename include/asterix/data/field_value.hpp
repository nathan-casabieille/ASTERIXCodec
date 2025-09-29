#pragma once

#include "asterix/core/types.hpp"
#include <variant>
#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace asterix {

/**
 * @brief Représente la valeur d'un champ ASTERIX décodé
 * 
 * Utilise std::variant pour stocker différents types de valeurs :
 * - std::uint64_t pour les entiers non signés
 * - std::int64_t pour les entiers signés
 * - bool pour les booléens
 * - std::string pour les énumérations et chaînes
 * - std::vector<std::uint8_t> pour les données brutes
 */
class FieldValue {
private:
    std::variant<
        std::uint64_t,              // unsigned
        std::int64_t,               // signed  
        bool,                       // boolean
        std::string,                // enum/string
        std::vector<std::uint8_t>   // raw
    > value_;
    FieldType type_;

public:
    /**
     * @brief Constructeur par défaut (valeur unsigned 0)
     */
    FieldValue() : value_(std::uint64_t{0}), type_(FieldType::Unsigned) {}

    /**
     * @brief Constructeur avec valeur unsigned
     */
    FieldValue(std::uint64_t value, FieldType type)
        : value_(value), type_(type) {}

    /**
     * @brief Constructeur avec valeur signed
     */
    FieldValue(std::int64_t value, FieldType type)
        : value_(value), type_(type) {}

    /**
     * @brief Constructeur avec valeur boolean
     */
    FieldValue(bool value, FieldType type)
        : value_(value), type_(type) {}

    /**
     * @brief Constructeur avec valeur string
     */
    FieldValue(std::string value, FieldType type)
        : value_(std::move(value)), type_(type) {}

    /**
     * @brief Constructeur avec valeur raw
     */
    FieldValue(std::vector<std::uint8_t> value, FieldType type)
        : value_(std::move(value)), type_(type) {}

    /**
     * @brief Obtient la valeur sous forme de type générique
     * @tparam T Type de la valeur attendue
     * @return Valeur convertie vers le type T
     * @throws std::bad_variant_access si le type ne correspond pas
     */
    template<typename T>
    T as() const {
        return std::get<T>(value_);
    }

    /**
     * @brief Obtient la valeur comme entier non signé
     * @return Valeur unsigned
     * @throws std::bad_variant_access si le type n'est pas unsigned
     */
    std::uint64_t asUInt() const;

    /**
     * @brief Obtient la valeur comme entier signé
     * @return Valeur signed
     * @throws std::bad_variant_access si le type n'est pas signed
     */
    std::int64_t asInt() const;

    /**
     * @brief Obtient la valeur comme booléen
     * @return Valeur boolean
     * @throws std::bad_variant_access si le type n'est pas boolean
     */
    bool asBool() const;

    /**
     * @brief Obtient la valeur comme énumération
     * @return Valeur enum (string)
     * @throws std::bad_variant_access si le type n'est pas enum
     */
    std::string asEnum() const;

    /**
     * @brief Obtient la valeur comme chaîne
     * @return Valeur string
     * @throws std::bad_variant_access si le type n'est pas string
     */
    std::string asString() const;

    /**
     * @brief Obtient la valeur comme données brutes
     * @return Valeur raw (vector<uint8_t>)
     * @throws std::bad_variant_access si le type n'est pas raw
     */
    std::vector<std::uint8_t> asRaw() const;

    /**
     * @brief Obtient le type de la valeur
     * @return Type du champ
     */
    FieldType getType() const { return type_; }

    /**
     * @brief Convertit la valeur en string pour affichage
     * @return Représentation textuelle de la valeur
     */
    std::string toString() const;

    /**
     * @brief Vérifie si la valeur est d'un type donné
     * @tparam T Type à vérifier
     * @return true si la valeur est du type T
     */
    template<typename T>
    bool holds() const {
        return std::holds_alternative<T>(value_);
    }

    /**
     * @brief Obtient la valeur du variant (accès bas niveau)
     * @return Référence constante vers le variant
     */
    const auto& getValue() const { return value_; }

    /**
     * @brief Vérifie l'égalité avec une autre FieldValue
     */
    bool operator==(const FieldValue& other) const {
        return type_ == other.type_ && value_ == other.value_;
    }

    /**
     * @brief Vérifie l'inégalité avec une autre FieldValue
     */
    bool operator!=(const FieldValue& other) const {
        return !(*this == other);
    }
};

} // namespace asterix