#pragma once

#include "asterix/core/types.hpp"
#include "asterix/data/field.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace asterix {

/**
 * @brief Représente un Data Item ASTERIX décodé
 * 
 * Un Data Item contient :
 * - Un identifiant (ex: "I002/010")
 * - Un titre descriptif
 * - Un ensemble de champs décodés (pour les items simples)
 * - Un ensemble de répétitions (pour les items répétitifs)
 */
class DataItem {
private:
    DataItemId id_;
    std::string title_;
    std::unordered_map<FieldName, Field> fields_;
    std::vector<DataItem> repetitions_;  ///< Pour les items répétitifs
    
public:
    /**
     * @brief Constructeur pour un Data Item simple
     */
    DataItem(DataItemId id, std::string title, 
             std::unordered_map<FieldName, Field> fields)
        : id_(std::move(id)),
          title_(std::move(title)),
          fields_(std::move(fields)) {}
    
    /**
     * @brief Constructeur pour un Data Item répétitif
     */
    DataItem(DataItemId id, std::string title,
             std::unordered_map<FieldName, Field> fields,
             std::vector<DataItem> repetitions)
        : id_(std::move(id)),
          title_(std::move(title)),
          fields_(std::move(fields)),
          repetitions_(std::move(repetitions)) {}
    
    /**
     * @brief Constructeur par défaut
     */
    DataItem() = default;

    // Getters de base
    const DataItemId& getId() const { return id_; }
    const std::string& getTitle() const { return title_; }
    
    /**
     * @brief Obtient un champ par son nom
     * @param name Nom du champ
     * @return Référence constante vers le champ
     * @throws InvalidDataException si le champ n'existe pas
     */
    const Field& getField(const FieldName& name) const;
    
    /**
     * @brief Vérifie si un champ existe
     * @param name Nom du champ
     * @return true si le champ existe
     */
    bool hasField(const FieldName& name) const;
    
    /**
     * @brief Obtient la liste de tous les noms de champs
     * @return Vecteur des noms de champs
     */
    std::vector<FieldName> getFieldNames() const;
    
    /**
     * @brief Obtient tous les champs
     * @return Map des champs
     */
    const std::unordered_map<FieldName, Field>& getAllFields() const { return fields_; }
    
    /**
     * @brief Obtient les répétitions (pour les items répétitifs)
     * @return Vecteur des répétitions
     */
    const std::vector<DataItem>& getRepetitions() const { return repetitions_; }
    
    /**
     * @brief Vérifie si le Data Item est répétitif
     * @return true si le Data Item contient des répétitions
     */
    bool isRepetitive() const { return !repetitions_.empty(); }
    
    /**
     * @brief Obtient le nombre de répétitions
     * @return Nombre de répétitions (0 si non répétitif)
     */
    std::size_t getRepetitionCount() const { return repetitions_.size(); }
    
    /**
     * @brief Obtient une répétition spécifique
     * @param index Index de la répétition (0-based)
     * @return Référence constante vers la répétition
     * @throws std::out_of_range si l'index est invalide
     */
    const DataItem& getRepetition(std::size_t index) const;
    
    /**
     * @brief Obtient le nombre de champs
     * @return Nombre de champs
     */
    std::size_t getFieldCount() const { return fields_.size(); }
    
    /**
     * @brief Vérifie si le Data Item est vide (pas de champs ni de répétitions)
     * @return true si vide
     */
    bool isEmpty() const { return fields_.empty() && repetitions_.empty(); }
    
    /**
     * @brief Convertit le Data Item en string pour affichage/debug
     * @param indent Indentation pour le formatage (usage interne)
     * @return Représentation textuelle du Data Item
     */
    std::string toString(std::size_t indent = 0) const;
    
    /**
     * @brief Obtient la valeur d'un champ directement
     * 
     * Méthode de convenance pour éviter getField().getValue()
     * 
     * @param name Nom du champ
     * @return Valeur du champ
     * @throws InvalidDataException si le champ n'existe pas
     */
    const FieldValue& getFieldValue(const FieldName& name) const;
};

} // namespace asterix