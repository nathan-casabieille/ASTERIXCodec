#pragma once

#include <cstdint>
#include <string>

namespace asterix {

// ============================================================================
// Types fondamentaux ASTERIX
// ============================================================================

/**
 * @brief Numéro de catégorie ASTERIX (0-255)
 */
using CategoryNumber = std::uint8_t;

/**
 * @brief Identifiant d'un Data Item (ex: "I002/010", "I048/020")
 */
using DataItemId = std::string;

/**
 * @brief Nom d'un champ dans un Data Item (ex: "SAC", "SIC", "latitude")
 */
using FieldName = std::string;

// ============================================================================
// Énumérations
// ============================================================================

/**
 * @brief Type de données d'un champ ASTERIX
 */
enum class FieldType {
    Unsigned,      ///< Entier non signé
    Signed,        ///< Entier signé (complément à 2)
    Boolean,       ///< Booléen (1 bit)
    Enumeration,   ///< Énumération (valeur mappée à une chaîne)
    String,        ///< Chaîne de caractères ASCII
    Raw,           ///< Données brutes (octets)
    Compound,      ///< Structure composée (non utilisé directement)
    Repetitive     ///< Structure répétitive (non utilisé directement)
};

/**
 * @brief Structure d'un Data Item ASTERIX
 */
enum class ItemStructure {
    FixedLength,        ///< Longueur fixe (nombre d'octets défini)
    VariableLength,     ///< Longueur variable (avec octet de longueur ou bits FX)
    RepetitiveFixed,    ///< Répétitif à longueur fixe (REP + n * taille_fixe)
    RepetitiveVariable  ///< Répétitif à longueur variable (REP + n * (LEN + données))
};

// ============================================================================
// Fonctions utilitaires pour les énumérations
// ============================================================================

/**
 * @brief Convertit un FieldType en chaîne de caractères
 * @param type Type de champ
 * @return Représentation textuelle du type
 */
std::string fieldTypeToString(FieldType type);

/**
 * @brief Convertit une chaîne en FieldType
 * @param type_str Représentation textuelle du type
 * @return Type de champ correspondant
 * @throws std::invalid_argument si la chaîne ne correspond à aucun type
 */
FieldType stringToFieldType(const std::string& type_str);

/**
 * @brief Convertit un ItemStructure en chaîne de caractères
 * @param structure Structure d'item
 * @return Représentation textuelle de la structure
 */
std::string itemStructureToString(ItemStructure structure);

/**
 * @brief Convertit une chaîne en ItemStructure
 * @param structure_str Représentation textuelle de la structure
 * @return Structure d'item correspondante
 * @throws std::invalid_argument si la chaîne ne correspond à aucune structure
 */
ItemStructure stringToItemStructure(const std::string& structure_str);

/**
 * @brief Vérifie si un FieldType nécessite des valeurs d'énumération
 * @param type Type de champ
 * @return true si le type est Enumeration
 */
inline bool requiresEnumValues(FieldType type) {
    return type == FieldType::Enumeration;
}

/**
 * @brief Vérifie si un ItemStructure est répétitif
 * @param structure Structure d'item
 * @return true si la structure est répétitive
 */
inline bool isRepetitive(ItemStructure structure) {
    return structure == ItemStructure::RepetitiveFixed || 
           structure == ItemStructure::RepetitiveVariable;
}

/**
 * @brief Vérifie si un ItemStructure a une longueur fixe
 * @param structure Structure d'item
 * @return true si la structure a une longueur fixe
 */
inline bool isFixedLength(ItemStructure structure) {
    return structure == ItemStructure::FixedLength || 
           structure == ItemStructure::RepetitiveFixed;
}

/**
 * @brief Vérifie si un ItemStructure a une longueur variable
 * @param structure Structure d'item
 * @return true si la structure a une longueur variable
 */
inline bool isVariableLength(ItemStructure structure) {
    return structure == ItemStructure::VariableLength || 
           structure == ItemStructure::RepetitiveVariable;
}

// ============================================================================
// Constantes
// ============================================================================

namespace constants {

/**
 * @brief Taille de l'en-tête ASTERIX (CAT + LEN)
 */
constexpr std::size_t ASTERIX_HEADER_SIZE = 3;

/**
 * @brief Taille minimale d'un message ASTERIX
 */
constexpr std::size_t MIN_MESSAGE_SIZE = ASTERIX_HEADER_SIZE;

/**
 * @brief Taille maximale d'un message ASTERIX (64 KB)
 */
constexpr std::size_t MAX_MESSAGE_SIZE = 65535;

/**
 * @brief Position du bit FX dans un octet UAP ou variable
 */
constexpr std::uint8_t FX_BIT_POSITION = 1;

/**
 * @brief Masque pour le bit FX (bit 1, LSB)
 */
constexpr std::uint8_t FX_BIT_MASK = 0x01;

/**
 * @brief Nombre de bits de données dans un octet UAP (bits 2-8)
 */
constexpr std::uint8_t UAP_DATA_BITS_PER_BYTE = 7;

/**
 * @brief Position du bit MSB dans un octet
 */
constexpr std::uint8_t MSB_BIT_POSITION = 8;

/**
 * @brief Position du bit LSB dans un octet
 */
constexpr std::uint8_t LSB_BIT_POSITION = 1;

} // namespace constants

} // namespace asterix