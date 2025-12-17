#pragma once

#include "asterix/core/types.hpp"
#include "asterix/data/data_item.hpp"
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>

namespace asterix {

/**
 * @brief Représente un Data Record ASTERIX décodé
 * 
 * Un Data Record contient une série de Data Items décodés selon l'UAP.
 * C'est l'unité de base des données ASTERIX.
 */
class AsterixRecord {
private:
    std::unordered_map<DataItemId, DataItem> data_items_;
    
public:
    /**
     * @brief Constructeur
     * @param data_items Map des Data Items décodés
     */
    explicit AsterixRecord(std::unordered_map<DataItemId, DataItem> data_items)
        : data_items_(std::move(data_items)) {}
    
    /**
     * @brief Constructeur par défaut
     */
    AsterixRecord() = default;
    
    /**
     * @brief Obtient un Data Item par son identifiant
     */
    const DataItem& getDataItem(const DataItemId& id) const;
    
    /**
     * @brief Vérifie si un Data Item est présent
     */
    bool hasDataItem(const DataItemId& id) const;
    
    /**
     * @brief Obtient la liste des identifiants de Data Items présents
     */
    std::vector<DataItemId> getDataItemIds() const;
    
    /**
     * @brief Obtient tous les Data Items
     */
    const std::unordered_map<DataItemId, DataItem>& getAllDataItems() const {
        return data_items_;
    }
    
    /**
     * @brief Obtient le nombre de Data Items
     */
    std::size_t getDataItemCount() const { return data_items_.size(); }
    
    /**
     * @brief Vérifie si le record est vide
     */
    bool isEmpty() const { return data_items_.empty(); }
    
    /**
     * @brief Accès direct à un champ
     */
    const Field& getField(const DataItemId& item_id, const FieldName& field_name) const;
    
    /**
     * @brief Obtient la valeur d'un champ directement
     */
    FieldValue getFieldValue(const DataItemId& item_id, const FieldName& field_name) const;
    
    /**
     * @brief Vérifie si un champ existe
     */
    bool hasField(const DataItemId& item_id, const FieldName& field_name) const;
    
    /**
     * @brief Convertit le record en string
     */
    std::string toString() const;
    
    /**
     * @brief Obtient un résumé court
     */
    std::string getSummary() const;
};

/**
 * @brief Représente un message ASTERIX décodé (Data Block)
 * 
 * Un message ASTERIX contient :
 * - Une catégorie (1 octet)
 * - Une longueur totale (2 octets)
 * - Un ou plusieurs Data Records
 */
class AsterixMessage {
private:
    CategoryNumber category_;
    std::uint16_t message_length_;
    std::vector<AsterixRecord> records_;
    
public:
    /**
     * @brief Constructeur pour message multi-records
     * @param category Numéro de catégorie ASTERIX
     * @param message_length Longueur totale du message en octets
     * @param records Vecteur de Data Records décodés
     */
    AsterixMessage(CategoryNumber category, 
                   std::uint16_t message_length,
                   std::vector<AsterixRecord> records)
        : category_(category),
          message_length_(message_length),
          records_(std::move(records)) {}
    
    /**
     * @brief Constructeur pour message single-record (compatibilité)
     * @param category Numéro de catégorie ASTERIX
     * @param message_length Longueur totale du message en octets
     * @param data_items Map des Data Items décodés
     */
    AsterixMessage(CategoryNumber category, 
                   std::uint16_t message_length,
                   std::unordered_map<DataItemId, DataItem> data_items)
        : category_(category),
          message_length_(message_length) {
        records_.emplace_back(std::move(data_items));
    }
    
    /**
     * @brief Constructeur par défaut
     */
    AsterixMessage()
        : category_(0),
          message_length_(0) {}
    
    /**
     * @brief Obtient le numéro de catégorie
     */
    CategoryNumber getCategory() const { return category_; }
    
    /**
     * @brief Obtient la longueur du message
     */
    std::uint16_t getLength() const { return message_length_; }
    
    /**
     * @brief Obtient le nombre de Data Records
     */
    std::size_t getRecordCount() const { return records_.size(); }
    
    /**
     * @brief Vérifie si le message contient plusieurs records
     */
    bool isMultiRecord() const { return records_.size() > 1; }
    
    /**
     * @brief Obtient un Data Record par index
     * @param index Index du record (0-based)
     * @return Référence constante vers le record
     * @throws std::out_of_range si l'index est invalide
     */
    const AsterixRecord& getRecord(std::size_t index) const;
    
    /**
     * @brief Obtient tous les records
     */
    const std::vector<AsterixRecord>& getAllRecords() const { return records_; }
    
    /**
     * @brief Obtient le premier record (compatibilité avec l'API single-record)
     */
    const AsterixRecord& getFirstRecord() const;
    
    // Méthodes de compatibilité pour l'API single-record
    // (opèrent sur le premier record uniquement)
    
    /**
     * @brief Obtient un Data Item du premier record
     */
    const DataItem& getDataItem(const DataItemId& id) const {
        return getFirstRecord().getDataItem(id);
    }
    
    /**
     * @brief Vérifie si un Data Item est présent dans le premier record
     */
    bool hasDataItem(const DataItemId& id) const {
        if (records_.empty()) return false;
        return records_[0].hasDataItem(id);
    }
    
    /**
     * @brief Obtient la liste des identifiants de Data Items du premier record
     */
    std::vector<DataItemId> getDataItemIds() const {
        return getFirstRecord().getDataItemIds();
    }
    
    /**
     * @brief Obtient tous les Data Items du premier record
     */
    const std::unordered_map<DataItemId, DataItem>& getAllDataItems() const {
        return getFirstRecord().getAllDataItems();
    }
    
    /**
     * @brief Obtient le nombre de Data Items dans le premier record
     */
    std::size_t getDataItemCount() const {
        return getFirstRecord().getDataItemCount();
    }
    
    /**
     * @brief Vérifie si le message est vide
     */
    bool isEmpty() const { 
        return records_.empty() || records_[0].isEmpty(); 
    }
    
    /**
     * @brief Accès direct à un champ du premier record
     */
    const Field& getField(const DataItemId& item_id, const FieldName& field_name) const {
        return getFirstRecord().getField(item_id, field_name);
    }
    
    /**
     * @brief Obtient la valeur d'un champ du premier record
     */
    FieldValue getFieldValue(const DataItemId& item_id, const FieldName& field_name) const {
        return getFirstRecord().getFieldValue(item_id, field_name);
    }
    
    /**
     * @brief Vérifie si un champ existe dans le premier record
     */
    bool hasField(const DataItemId& item_id, const FieldName& field_name) const {
        if (records_.empty()) return false;
        return records_[0].hasField(item_id, field_name);
    }
    
    /**
     * @brief Convertit le message en string pour affichage/debug
     */
    std::string toString() const;
    
    /**
     * @brief Obtient un résumé court du message
     */
    std::string getSummary() const;
    
    /**
     * @brief Valide le message
     */
    bool validate() const;
};

} // namespace asterix