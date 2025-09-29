#include "asterix/data/data_item.hpp"
#include "asterix/utils/exceptions.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace asterix {

const Field& DataItem::getField(const FieldName& name) const {
    auto it = fields_.find(name);
    if (it == fields_.end()) {
        throw InvalidDataException("Field '" + name + "' not found in Data Item '" + id_ + "'");
    }
    return it->second;
}

bool DataItem::hasField(const FieldName& name) const {
    return fields_.find(name) != fields_.end();
}

std::vector<FieldName> DataItem::getFieldNames() const {
    std::vector<FieldName> names;
    names.reserve(fields_.size());
    
    for (const auto& pair : fields_) {
        names.push_back(pair.first);
    }
    
    // Trier les noms pour avoir un ordre déterministe
    std::sort(names.begin(), names.end());
    
    return names;
}

const DataItem& DataItem::getRepetition(std::size_t index) const {
    if (index >= repetitions_.size()) {
        throw std::out_of_range("Repetition index " + std::to_string(index) + 
            " out of range for Data Item '" + id_ + "' (has " + 
            std::to_string(repetitions_.size()) + " repetitions)");
    }
    return repetitions_[index];
}

const FieldValue& DataItem::getFieldValue(const FieldName& name) const {
    return getField(name).getValue();
}

std::string DataItem::toString(std::size_t indent) const {
    std::ostringstream oss;
    std::string indent_str(indent, ' ');
    
    oss << indent_str << "DataItem: " << id_;
    if (!title_.empty()) {
        oss << " (" << title_ << ")";
    }
    oss << "\n";
    
    // Afficher les champs
    if (!fields_.empty()) {
        // Obtenir les noms triés pour un affichage cohérent
        auto field_names = getFieldNames();
        
        for (const auto& name : field_names) {
            const auto& field = fields_.at(name);
            oss << indent_str << "  " << name << ": " 
                << field.getValue().toString();
            
            if (!field.getUnit().empty() && field.getUnit() != "none") {
                oss << " " << field.getUnit();
            }
            oss << "\n";
        }
    }
    
    // Afficher les répétitions
    if (!repetitions_.empty()) {
        oss << indent_str << "  Repetitions: " << repetitions_.size() << "\n";
        
        for (std::size_t i = 0; i < repetitions_.size(); ++i) {
            oss << indent_str << "  [" << i << "]:\n";
            
            // Afficher les champs de chaque répétition
            const auto& rep = repetitions_[i];
            auto rep_field_names = rep.getFieldNames();
            
            for (const auto& name : rep_field_names) {
                const auto& field = rep.getAllFields().at(name);
                oss << indent_str << "    " << name << ": " 
                    << field.getValue().toString();
                
                if (!field.getUnit().empty() && field.getUnit() != "none") {
                    oss << " " << field.getUnit();
                }
                oss << "\n";
            }
        }
    }
    
    // Indiquer si vide
    if (isEmpty()) {
        oss << indent_str << "  (empty)\n";
    }
    
    return oss.str();
}

} // namespace asterix