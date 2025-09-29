#pragma once

#include "asterix/core/types.hpp"
#include "asterix/utils/byte_buffer.hpp"
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace asterix {

/**
 * @brief Spécification du User Application Profile (UAP)
 * 
 * Le UAP définit quels Data Items sont présents dans un message ASTERIX.
 * Il utilise une structure en octets avec des bits FX (Field eXtension) :
 * - Chaque bit (sauf le bit 1/LSB) indique la présence d'un Data Item
 * - Le bit 1 (FX) indique s'il y a un octet UAP supplémentaire
 * - Les bits sont numérotés de 8 (MSB) à 1 (LSB)
 */
class UapSpec {
public:
    /**
     * @brief Structure représentant un item dans l'UAP
     */
    struct UapItem {
        std::uint8_t bit_position;  ///< Position du bit (2-8, le bit 1 est réservé pour FX)
        DataItemId item_id;         ///< Identifiant du Data Item (ex: "I002/010")
        bool mandatory;             ///< Indique si l'item est obligatoire
        
        UapItem(std::uint8_t pos, DataItemId id, bool mand)
            : bit_position(pos), item_id(std::move(id)), mandatory(mand) {}
    };

private:
    std::vector<UapItem> items_;
    std::unordered_map<DataItemId, std::uint8_t> item_to_bit_;  ///< Map item_id -> bit_position
    std::unordered_map<DataItemId, bool> item_mandatory_;        ///< Map item_id -> mandatory
    
    /**
     * @brief Construit les index internes pour un accès rapide
     */
    void buildIndexes();

public:
    /**
     * @brief Constructeur par défaut
     */
    UapSpec() = default;

    /**
     * @brief Constructeur avec liste d'items
     * @param items Liste des items UAP
     */
    explicit UapSpec(std::vector<UapItem> items);

    /**
     * @brief Décode l'UAP depuis un buffer
     * 
     * Lit les octets UAP jusqu'à trouver un FX bit à 0.
     * Retourne la liste ordonnée des Data Items présents dans le message.
     * 
     * @param buffer Buffer contenant les données
     * @param offset Position courante dans le buffer (sera mise à jour)
     * @return Liste des DataItemId présents dans l'ordre de décodage
     * @throws DecodingException si l'UAP est invalide
     */
    std::vector<DataItemId> decodeUap(const ByteBuffer& buffer, std::size_t& offset) const;

    /**
     * @brief Vérifie si un Data Item est obligatoire
     * @param item_id Identifiant du Data Item
     * @return true si l'item est obligatoire
     */
    bool isMandatory(const DataItemId& item_id) const;

    /**
     * @brief Obtient la position de bit d'un Data Item
     * @param item_id Identifiant du Data Item
     * @return Position du bit (2-8), ou 0 si l'item n'existe pas
     */
    std::uint8_t getBitPosition(const DataItemId& item_id) const;

    /**
     * @brief Obtient tous les items UAP
     * @return Liste des items UAP
     */
    const std::vector<UapItem>& getAllItems() const { return items_; }

    /**
     * @brief Vérifie si l'UAP contient un Data Item spécifique
     * @param item_id Identifiant du Data Item
     * @return true si l'item existe dans l'UAP
     */
    bool hasItem(const DataItemId& item_id) const;

    /**
     * @brief Obtient le nombre total d'items dans l'UAP
     * @return Nombre d'items
     */
    std::size_t getItemCount() const { return items_.size(); }

    /**
     * @brief Valide que tous les items obligatoires sont présents
     * @param present_items Liste des items présents dans le message
     * @throws DecodingException si des items obligatoires sont manquants
     */
    void validateMandatoryItems(const std::vector<DataItemId>& present_items) const;
};

} // namespace asterix