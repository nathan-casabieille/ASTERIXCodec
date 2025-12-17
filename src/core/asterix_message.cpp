#include "asterix/core/asterix_message.hpp"
#include "asterix/utils/exceptions.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace asterix {

// ============================================================================
// AsterixRecord - Implémentation
// ============================================================================

const DataItem& AsterixRecord::getDataItem(const DataItemId& id) const {
    auto it = data_items_.find(id);
    if (it == data_items_.end()) {
        throw InvalidDataException(
            "Data Item '" + id + "' not found in this record"
        );
    }
    return it->second;
}

bool AsterixRecord::hasDataItem(const DataItemId& id) const {
    return data_items_.find(id) != data_items_.end();
}

std::vector<DataItemId> AsterixRecord::getDataItemIds() const {
    std::vector<DataItemId> ids;
    ids.reserve(data_items_.size());
    
    for (const auto& [id, _] : data_items_) {
        ids.push_back(id);
    }
    
    std::sort(ids.begin(), ids.end());
    return ids;
}

const Field& AsterixRecord::getField(const DataItemId& item_id, 
                                     const FieldName& field_name) const {
    const DataItem& item = getDataItem(item_id);
    return item.getField(field_name);
}

FieldValue AsterixRecord::getFieldValue(const DataItemId& item_id, 
                                        const FieldName& field_name) const {
    const DataItem& item = getDataItem(item_id);
    return item.getFieldValue(field_name);
}

bool AsterixRecord::hasField(const DataItemId& item_id, 
                             const FieldName& field_name) const {
    if (!hasDataItem(item_id)) {
        return false;
    }
    return data_items_.at(item_id).hasField(field_name);
}

std::string AsterixRecord::toString() const {
    std::ostringstream oss;
    
    if (!data_items_.empty()) {
        auto sorted_ids = getDataItemIds();
        
        for (const auto& id : sorted_ids) {
            const auto& item = data_items_.at(id);
            oss << item.toString(2);  // Indentation de 2 espaces
            oss << "\n";
        }
    } else {
        oss << "  (empty record)\n";
    }
    
    return oss.str();
}

std::string AsterixRecord::getSummary() const {
    std::ostringstream oss;
    
    oss << "Items: " << data_items_.size();
    
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

// ============================================================================
// AsterixMessage - Implémentation
// ============================================================================

const AsterixRecord& AsterixMessage::getRecord(std::size_t index) const {
    if (index >= records_.size()) {
        throw std::out_of_range(
            "Record index " + std::to_string(index) + 
            " out of range (message has " + std::to_string(records_.size()) + 
            " records)"
        );
    }
    return records_[index];
}

const AsterixRecord& AsterixMessage::getFirstRecord() const {
    if (records_.empty()) {
        throw InvalidDataException("Message has no records");
    }
    return records_[0];
}

std::string AsterixMessage::toString() const {
    std::ostringstream oss;
    
    // En-tête du message
    oss << "=== ASTERIX Message ===\n";
    oss << "Category: " << static_cast<int>(category_) << "\n";
    oss << "Length: " << message_length_ << " bytes\n";
    oss << "Records: " << records_.size() << "\n";
    oss << "\n";
    
    // Afficher chaque Data Record
    for (std::size_t i = 0; i < records_.size(); ++i) {
        oss << "--- Data Record " << (i + 1) << " ---\n";
        oss << records_[i].toString();
        if (i < records_.size() - 1) {
            oss << "\n";
        }
    }
    
    oss << "======================\n";
    
    return oss.str();
}

std::string AsterixMessage::getSummary() const {
    std::ostringstream oss;
    
    oss << "ASTERIX CAT" << std::setfill('0') << std::setw(3) 
        << static_cast<int>(category_);
    oss << " | Length: " << message_length_ << " bytes";
    oss << " | Records: " << records_.size();
    
    if (records_.size() == 1 && !records_.empty()) {
        oss << " | " << records_[0].getSummary();
    } else if (records_.size() > 1) {
        // Compter le total d'items
        std::size_t total_items = 0;
        for (const auto& record : records_) {
            total_items += record.getDataItemCount();
        }
        oss << " | Total Items: " << total_items;
    }
    
    return oss.str();
}

bool AsterixMessage::validate() const {
    // Vérifier que la longueur est cohérente
    if (message_length_ < 3) {
        return false;
    }
    
    // Vérifier qu'il y a au moins un record
    if (records_.empty()) {
        return false;
    }
    
    // Vérifier que chaque record est valide
    for (const auto& record : records_) {
        // Vérifier que le record n'est pas vide
        if (record.isEmpty()) {
            return false;
        }
        
        // Vérifier que chaque Data Item est valide
        for (const auto& [id, item] : record.getAllDataItems()) {
            // Vérifier que le Data Item n'est pas vide (sauf répétitifs avec 0 répétitions)
            if (item.isEmpty() && !item.isRepetitive()) {
                return false;
            }
            
            // Si c'est répétitif, vérifier la cohérence
            if (item.isRepetitive()) {
                for (std::size_t i = 0; i < item.getRepetitionCount(); ++i) {
                    const auto& rep = item.getRepetition(i);
                    if (rep.isEmpty()) {
                        return false;
                    }
                }
            }
        }
    }
    
    return true;
}

} // namespace asterix