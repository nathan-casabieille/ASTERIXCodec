#include "asterix/core/asterix_category.hpp"
#include "asterix/spec/xml_parser.hpp"
#include "asterix/utils/exceptions.hpp"
#include <sstream>

namespace asterix {

AsterixCategory::AsterixCategory(const std::filesystem::path& xml_file) {
    // Utiliser le parser XML pour charger la spécification
    try {
        *this = spec::XmlParser::parseSpecification(xml_file);
    } catch (const SpecificationException&) {
        // Re-lancer les exceptions de spécification telles quelles
        throw;
    } catch (const std::exception& e) {
        // Encapsuler les autres exceptions
        throw SpecificationException(
            "Failed to load ASTERIX category from '" + xml_file.string() + "': " + e.what()
        );
    }
}

const DataItemSpec& AsterixCategory::getDataItemSpec(const DataItemId& id) const {
    auto it = data_items_.find(id);
    if (it == data_items_.end()) {
        throw SpecificationException(
            "Data Item '" + id + "' not found in ASTERIX Category " + 
            std::to_string(category_number_) + " (version " + version_ + ")"
        );
    }
    return it->second;
}

bool AsterixCategory::hasDataItem(const DataItemId& id) const {
    return data_items_.find(id) != data_items_.end();
}

void AsterixCategory::validate() const {
    std::vector<std::string> errors;
    
    // Vérifier que la catégorie a au moins un Data Item
    if (data_items_.empty()) {
        errors.push_back("Category has no Data Items defined");
    }
    
    // Vérifier que tous les items UAP ont une spécification correspondante
    const auto& uap_items = uap_spec_.getAllItems();
    for (const auto& uap_item : uap_items) {
        if (!hasDataItem(uap_item.item_id)) {
            errors.push_back(
                "UAP references undefined Data Item: " + uap_item.item_id
            );
        }
    }
    
    // Vérifier la cohérence de chaque Data Item
    for (const auto& [id, spec] : data_items_) {
        // Vérifier que le Data Item a au moins un champ (sauf pour les structures composées)
        if (!spec.hasFields()) {
            errors.push_back(
                "Data Item '" + id + "' has no fields defined"
            );
        }
        
        // Vérifier la cohérence entre structure et longueur fixe
        if (spec.getStructure() == ItemStructure::FixedLength ||
            spec.getStructure() == ItemStructure::RepetitiveFixed) {
            
            if (spec.getFixedLength() == 0) {
                errors.push_back(
                    "Data Item '" + id + "' has fixed structure but zero length"
                );
            }
            
            // Vérifier que la taille totale des champs correspond à la longueur fixe
            std::size_t total_bits = spec.getTotalBitSize();
            std::size_t expected_bits = spec.getFixedLength() * 8;
            
            if (total_bits != expected_bits) {
                std::ostringstream oss;
                oss << "Data Item '" << id << "' size mismatch: "
                    << "fields total " << total_bits << " bits, "
                    << "but fixed_length is " << spec.getFixedLength() 
                    << " bytes (" << expected_bits << " bits)";
                errors.push_back(oss.str());
            }
        }
        
        // Vérifier que les champs d'énumération ont des valeurs définies
        for (const auto& field : spec.getFields()) {
            if (field.getType() == FieldType::Enumeration && 
                !field.hasEnumValues()) {
                errors.push_back(
                    "Enumeration field '" + field.getName() + 
                    "' in Data Item '" + id + "' has no enum values defined"
                );
            }
            
            // Vérifier que la taille des champs booléens est exactement 1 bit
            if (field.getType() == FieldType::Boolean && 
                field.getBitSize() != 1) {
                errors.push_back(
                    "Boolean field '" + field.getName() + 
                    "' in Data Item '" + id + "' must have exactly 1 bit, got " +
                    std::to_string(field.getBitSize())
                );
            }
        }
    }
    
    // Vérifier qu'il n'y a pas d'items UAP orphelins (définis mais jamais référencés)
    // Note: tous les Data Items doivent être dans l'UAP
    std::unordered_set<DataItemId> uap_item_ids;
    for (const auto& uap_item : uap_items) {
        uap_item_ids.insert(uap_item.item_id);
    }
    
    for (const auto& [id, spec] : data_items_) {
        if (uap_item_ids.find(id) == uap_item_ids.end()) {
            // Avertissement plutôt qu'erreur : certains Data Items peuvent être 
            // définis pour référence future
            // errors.push_back("Data Item '" + id + "' is defined but not in UAP");
        }
    }
    
    // Si des erreurs ont été trouvées, lever une exception
    if (!errors.empty()) {
        std::ostringstream oss;
        oss << "ASTERIX Category " << static_cast<int>(category_number_) 
            << " (version " << version_ << ") validation failed:\n";
        
        for (std::size_t i = 0; i < errors.size(); ++i) {
            oss << "  " << (i + 1) << ". " << errors[i];
            if (i < errors.size() - 1) {
                oss << "\n";
            }
        }
        
        throw SpecificationException(oss.str());
    }
}

} // namespace asterix