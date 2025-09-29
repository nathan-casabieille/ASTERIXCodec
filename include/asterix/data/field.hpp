#pragma once

#include "asterix/core/types.hpp"
#include "asterix/data/field_value.hpp"
#include <string>

namespace asterix {

/**
 * @brief Représente un champ décodé d'un Data Item ASTERIX
 * 
 * Un champ contient :
 * - Un nom (ex: "SAC", "SIC", "latitude")
 * - Une valeur décodée (FieldValue)
 * - Une unité de mesure optionnelle (ex: "degrees", "meters", "knots")
 */
class Field {
private:
    FieldName name_;
    FieldValue value_;
    std::string unit_;

public:
    /**
     * @brief Constructeur complet
     */
    Field(FieldName name, FieldValue value, std::string unit)
        : name_(std::move(name)),
          value_(std::move(value)),
          unit_(std::move(unit)) {}

    /**
     * @brief Constructeur sans unité
     */
    Field(FieldName name, FieldValue value)
        : name_(std::move(name)),
          value_(std::move(value)),
          unit_("none") {}

    /**
     * @brief Constructeur par défaut
     */
    Field() : unit_("none") {}

    /**
     * @brief Obtient le nom du champ
     * @return Nom du champ
     */
    const FieldName& getName() const { return name_; }

    /**
     * @brief Obtient la valeur du champ
     * @return Référence constante vers la valeur
     */
    const FieldValue& getValue() const { return value_; }

    /**
     * @brief Obtient l'unité de mesure
     * @return Unité (ex: "degrees", "meters", "none")
     */
    const std::string& getUnit() const { return unit_; }

    /**
     * @brief Vérifie si le champ a une unité définie
     * @return true si l'unité n'est pas "none" ou vide
     */
    bool hasUnit() const { 
        return !unit_.empty() && unit_ != "none"; 
    }

    /**
     * @brief Obtient le type de la valeur
     * @return Type du champ
     */
    FieldType getType() const { 
        return value_.getType(); 
    }

    /**
     * @brief Convertit le champ en string pour affichage
     * @param include_unit Si true, inclut l'unité dans la sortie
     * @return Représentation textuelle du champ
     */
    std::string toString(bool include_unit = true) const;

    /**
     * @brief Obtient une représentation détaillée du champ
     * @return String formaté avec nom, valeur et unité
     */
    std::string toDetailedString() const;

    /**
     * @brief Vérifie l'égalité avec un autre Field
     * 
     * Deux champs sont égaux s'ils ont le même nom, la même valeur et la même unité
     */
    bool operator==(const Field& other) const {
        return name_ == other.name_ && 
               value_ == other.value_ && 
               unit_ == other.unit_;
    }

    /**
     * @brief Vérifie l'inégalité avec un autre Field
     */
    bool operator!=(const Field& other) const {
        return !(*this == other);
    }

    // Méthodes de convenance pour accéder directement à la valeur typée
    // Évite d'avoir à faire field.getValue().asUInt()

    /**
     * @brief Obtient la valeur comme unsigned
     * @throws std::bad_variant_access si le type ne correspond pas
     */
    std::uint64_t asUInt() const { return value_.asUInt(); }

    /**
     * @brief Obtient la valeur comme signed
     * @throws std::bad_variant_access si le type ne correspond pas
     */
    std::int64_t asInt() const { return value_.asInt(); }

    /**
     * @brief Obtient la valeur comme boolean
     * @throws std::bad_variant_access si le type ne correspond pas
     */
    bool asBool() const { return value_.asBool(); }

    /**
     * @brief Obtient la valeur comme enum
     * @throws std::bad_variant_access si le type ne correspond pas
     */
    std::string asEnum() const { return value_.asEnum(); }

    /**
     * @brief Obtient la valeur comme string
     * @throws std::bad_variant_access si le type ne correspond pas
     */
    std::string asString() const { return value_.asString(); }

    /**
     * @brief Obtient la valeur comme raw data
     * @throws std::bad_variant_access si le type ne correspond pas
     */
    std::vector<std::uint8_t> asRaw() const { return value_.asRaw(); }
};

} // namespace asterix