#pragma once

#include "asterix/core/types.hpp"
#include "asterix/spec/uap_spec.hpp"
#include "asterix/spec/data_item_spec.hpp"
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace asterix {

class AsterixCategory {
private:
    CategoryNumber category_number_;
    std::string version_;
    UapSpec uap_spec_;
    std::unordered_map<DataItemId, DataItemSpec> data_items_;
    
public:
    /**
     * @brief Constructeur chargeant une spécification depuis un fichier XML
     * @param xml_file Chemin vers le fichier de spécification XML
     * @throws SpecificationException si le fichier est invalide
     */
    explicit AsterixCategory(const std::filesystem::path& xml_file);
    
    /**
     * @brief Constructeur utilisé par le parser XML
     * @param category_number Numéro de catégorie ASTERIX
     * @param version Version de la spécification
     * @param uap_spec Spécification UAP
     * @param data_items Map des spécifications de Data Items
     */
    AsterixCategory(CategoryNumber category_number, std::string version,
                   UapSpec uap_spec, std::unordered_map<DataItemId, DataItemSpec> data_items)
        : category_number_(category_number), version_(std::move(version)),
          uap_spec_(std::move(uap_spec)), data_items_(std::move(data_items)) {}
    
    /**
     * @brief Obtient le numéro de catégorie
     * @return Numéro de catégorie (0-255)
     */
    CategoryNumber getCategoryNumber() const { return category_number_; }
    
    /**
     * @brief Obtient la version de la spécification
     * @return Version (ex: "1.2", "2.1")
     */
    const std::string& getVersion() const { return version_; }
    
    /**
     * @brief Obtient la spécification UAP
     * @return Référence constante vers l'UAP
     */
    const UapSpec& getUapSpec() const { return uap_spec_; }
    
    /**
     * @brief Obtient la spécification d'un Data Item
     * @param id Identifiant du Data Item (ex: "I002/010")
     * @return Référence constante vers la spécification
     * @throws SpecificationException si le Data Item n'existe pas
     */
    const DataItemSpec& getDataItemSpec(const DataItemId& id) const;
    
    /**
     * @brief Vérifie si un Data Item existe dans cette catégorie
     * @param id Identifiant du Data Item
     * @return true si le Data Item existe
     */
    bool hasDataItem(const DataItemId& id) const;
    
    /**
     * @brief Obtient tous les Data Items de cette catégorie
     * @return Map des spécifications de Data Items
     */
    const std::unordered_map<DataItemId, DataItemSpec>& getAllDataItems() const { 
        return data_items_; 
    }
    
    /**
     * @brief Obtient le nombre de Data Items définis
     * @return Nombre de Data Items
     */
    std::size_t getDataItemCount() const { return data_items_.size(); }
    
    /**
     * @brief Valide la cohérence de la spécification
     * 
     * Vérifie que :
     * - Tous les items UAP ont une spécification correspondante
     * - Les spécifications sont complètes et cohérentes
     * 
     * @throws SpecificationException si la validation échoue
     */
    void validate() const;
};

} // namespace asterix