#include "asterix/core/asterix_message.hpp"
#include "asterix/utils/exceptions.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace asterix {

const DataItem& AsterixMessage::getDataItem(const DataItemId& id) const {
    auto it = data_items_.find(id);
    if (it == data_items_.end()) {
        throw InvalidDataException(
            "Data Item '" + id + "' not found in ASTERIX message (Category " + 
            std::to_string(category_) + ")"
        );
    }
    return it->second;
}

bool AsterixMessage::hasDataItem(const DataItemId& id) const {
    return data_items_.find(id) != data_items_.end();
}

std::vector<DataItemId> AsterixMessage::getDataItemIds() const {
    std::vector<DataItemId> ids;
    ids.reserve(data_items_.size());
    
    for (const auto& [id, _] : data_items_) {
        ids.push_back(id);
    }
    
    // Trier les IDs pour avoir un ordre déterministe
    std::sort(ids.begin(), ids.end());
    
    return ids;
}

const Field& AsterixMessage::getField(const DataItemId& item_id, 
                                      const FieldName& field_name) const {
    const DataItem& item = getDataItem(item_id);
    return item.getField(field_name);
}

FieldValue AsterixMessage::getFieldValue(const DataItemId& item_id, 
                                         const FieldName& field_name) const {
    const DataItem& item = getDataItem(item_id);
    return item.getFieldValue(field_name);
}

bool AsterixMessage::hasField(const DataItemId& item_id, 
                              const FieldName& field_name) const {
    if (!hasDataItem(item_id)) {
        return false;
    }
    return data_items_.at(item_id).hasField(field_name);
}

std::string AsterixMessage::toString() const {
    std::ostringstream oss;
    
    // En-tête du message
    oss << "=== ASTERIX Message ===\n";
    oss << "Category: " << static_cast<int>(category_) << "\n";
    oss << "Length: " << message_length_ << " bytes\n";
    oss << "Data Items: " << data_items_.size() << "\n";
    oss << "\n";
    
    // Afficher chaque Data Item
    if (!data_items_.empty()) {
        // Obtenir les IDs triés pour un affichage cohérent
        auto sorted_ids = getDataItemIds();
        
        for (const auto& id : sorted_ids) {
            const auto& item = data_items_.at(id);
            oss << item.toString(2);  // Indentation de 2 espaces
            oss << "\n";
        }
    } else {
        oss << "  (no data items)\n";
    }
    
    oss << "======================\n";
    
    return oss.str();
}

std::string AsterixMessage::getSummary() const {
    std::ostringstream oss;
    
    oss << "ASTERIX CAT" << std::setfill('0') << std::setw(3) 
        << static_cast<int>(category_);
    oss << " | Length: " << message_length_ << " bytes";
    oss << " | Items: " << data_items_.size();
    
    if (!data_items_.empty()) {
        oss << " [";
        auto ids = getDataItemIds();
        for (std::size_t i = 0; i < std::min(ids.size(), std::size_t(3)); ++i) {
            if (i > 0) oss << ", ";
            oss << ids[i];
        }
        if (ids.size() > 3) {
            oss << ", ..." << (ids.size() - 3) << " more";
        }
        oss << "]";
    }
    
    return oss.str();
}

bool AsterixMessage::validate() const {
    // Vérifier que la catégorie est valide (0-255, déjà garanti par le type)
    
    // Vérifier que la longueur est cohérente
    // La longueur minimale est 3 octets (CAT + LEN)
    if (message_length_ < 3) {
        return false;
    }
    
    // Vérifier que chaque Data Item est valide
    for (const auto& [id, item] : data_items_) {
        // Vérifier que le Data Item n'est pas vide (sauf pour les répétitifs avec 0 répétitions)
        if (item.isEmpty() && !item.isRepetitive()) {
            return false;
        }
        
        // Si c'est répétitif, vérifier que les répétitions sont cohérentes
        if (item.isRepetitive()) {
            for (std::size_t i = 0; i < item.getRepetitionCount(); ++i) {
                const auto& rep = item.getRepetition(i);
                if (rep.isEmpty()) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

} // namespace asterix