#pragma once

#include "asterix/core/types.hpp"
#include "asterix/data/data_item.hpp"
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>

namespace asterix {

/**
 * @brief Représente un message ASTERIX décodé
 * 
 * Un message ASTERIX contient :
 * - Une catégorie (1 octet)
 * - Une longueur totale (2 octets)
 * - Une série de Data Items décodés selon l'UAP
 */
class AsterixMessage {
private:
    CategoryNumber category_;
    std::uint16_t message_length_;
    std::unordered_map<DataItemId, DataItem> data_items_;
    
public:
    /**
     * @brief Constructeur complet
     * @param category Numéro de catégorie ASTERIX
     * @param message_length Longueur totale du message en octets
     * @param data_items Map des Data Items décodés
     */
    AsterixMessage(CategoryNumber category, 
                   std::uint16_t message_length,
                   std::unordered_map<DataItemId, DataItem> data_items)
        : category_(category),
          message_length_(message_length),
          data_items_(std::move(data_items)) {}
    
    /**
     * @brief Constructeur par défaut
     */
    AsterixMessage()
        : category_(0),
          message_length_(0) {}
    
    /**
     * @brief Obtient le numéro de catégorie
     * @return Numéro de catégorie (0-255)
     */
    CategoryNumber getCategory() const { return category_; }
    
    /**
     * @brief Obtient la longueur du message
     * @return Longueur totale en octets
     */
    std::uint16_t getLength() const { return message_length_; }
    
    /**
     * @brief Obtient un Data Item par son identifiant
     * @param id Identifiant du Data Item (ex: "I002/010")
     * @return Référence constante vers le Data Item
     * @throws InvalidDataException si le Data Item n'existe pas dans ce message
     */
    const DataItem& getDataItem(const DataItemId& id) const;
    
    /**
     * @brief Vérifie si un Data Item est présent dans le message
     * @param id Identifiant du Data Item
     * @return true si le Data Item est présent
     */
    bool hasDataItem(const DataItemId& id) const;
    
    /**
     * @brief Obtient la liste des identifiants de Data Items présents
     * @return Vecteur des identifiants de Data Items
     */
    std::vector<DataItemId> getDataItemIds() const;
    
    /**
     * @brief Obtient tous les Data Items du message
     * @return Map des Data Items
     */
    const std::unordered_map<DataItemId, DataItem>& getAllDataItems() const {
        return data_items_;
    }
    
    /**
     * @brief Obtient le nombre de Data Items dans le message
     * @return Nombre de Data Items
     */
    std::size_t getDataItemCount() const { return data_items_.size(); }
    
    /**
     * @brief Vérifie si le message est vide (pas de Data Items)
     * @return true si vide
     */
    bool isEmpty() const { return data_items_.empty(); }
    
    /**
     * @brief Accès direct à un champ d'un Data Item
     * 
     * Méthode de convenance pour accéder directement à un champ
     * sans avoir à faire getDataItem().getField()
     * 
     * @param item_id Identifiant du Data Item
     * @param field_name Nom du champ
     * @return Référence constante vers le champ
     * @throws InvalidDataException si le Data Item ou le champ n'existe pas
     */
    const Field& getField(const DataItemId& item_id, const FieldName& field_name) const;
    
    /**
     * @brief Obtient la valeur d'un champ directement
     * 
     * Méthode de convenance pour obtenir directement la valeur d'un champ
     * sans avoir à faire getDataItem().getField().getValue()
     * 
     * @param item_id Identifiant du Data Item
     * @param field_name Nom du champ
     * @return Valeur du champ
     * @throws InvalidDataException si le Data Item ou le champ n'existe pas
     */
    FieldValue getFieldValue(const DataItemId& item_id, const FieldName& field_name) const;
    
    /**
     * @brief Vérifie si un champ spécifique existe dans un Data Item
     * @param item_id Identifiant du Data Item
     * @param field_name Nom du champ
     * @return true si le Data Item existe et contient ce champ
     */
    bool hasField(const DataItemId& item_id, const FieldName& field_name) const;
    
    /**
     * @brief Convertit le message en string pour affichage/debug
     * @return Représentation textuelle du message
     */
    std::string toString() const;
    
    /**
     * @brief Obtient un résumé court du message
     * @return Résumé (catégorie, longueur, nombre de Data Items)
     */
    std::string getSummary() const;
    
    /**
     * @brief Valide le message
     * 
     * Vérifie que :
     * - La longueur du message est cohérente
     * - Tous les Data Items sont valides
     * 
     * @return true si le message est valide
     */
    bool validate() const;
};

} // namespace asterix