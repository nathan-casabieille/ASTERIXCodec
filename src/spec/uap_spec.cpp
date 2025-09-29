#include "asterix/spec/uap_spec.hpp"
#include "asterix/utils/exceptions.hpp"
#include <algorithm>
#include <sstream>
#include <set>

namespace asterix {

UapSpec::UapSpec(std::vector<UapItem> items)
    : items_(std::move(items)) {
    buildIndexes();
}

void UapSpec::buildIndexes() {
    item_to_bit_.clear();
    item_mandatory_.clear();
    
    for (const auto& item : items_) {
        item_to_bit_[item.item_id] = item.bit_position;
        item_mandatory_[item.item_id] = item.mandatory;
    }
}

std::vector<DataItemId> UapSpec::decodeUap(const ByteBuffer& buffer, std::size_t& offset) const {
    std::vector<DataItemId> present_items;
    
    if (items_.empty()) {
        throw DecodingException("UAP specification is empty");
    }
    
    // Calculer le nombre maximum d'octets UAP possibles
    // Chaque octet peut contenir 7 items (bits 2-8), le bit 1 étant FX
    std::size_t max_items = items_.size();
    std::size_t max_uap_octets = (max_items + 6) / 7;  // Arrondi supérieur
    
    std::size_t uap_octet_index = 0;
    bool has_fx = true;
    
    // Index pour parcourir les items dans l'ordre de leur définition
    std::size_t item_index = 0;
    
    while (has_fx && uap_octet_index < max_uap_octets) {
        // Vérifier qu'il reste des octets dans le buffer
        if (offset >= buffer.size()) {
            throw DecodingException("Unexpected end of buffer while decoding UAP");
        }
        
        std::uint8_t uap_byte = buffer.readByte(offset);
        offset++;
        
        // Lire les bits 2 à 8 (bit 1 est FX)
        for (std::uint8_t bit_pos = 8; bit_pos >= 2 && item_index < items_.size(); --bit_pos) {
            const auto& item = items_[item_index];
            
            // Vérifier si ce bit correspond à la position attendue
            // Les items sont triés par bit_position décroissante
            if (item.bit_position == bit_pos) {
                // Vérifier si le bit est mis à 1
                std::uint8_t bit_mask = 1 << (bit_pos - 1);
                if (uap_byte & bit_mask) {
                    present_items.push_back(item.item_id);
                }
                item_index++;
            }
        }
        
        // Vérifier le bit FX (bit 1, LSB)
        has_fx = (uap_byte & 0x01) != 0;
        uap_octet_index++;
    }
    
    // Vérifier si on a décodé tous les octets UAP attendus
    if (has_fx && uap_octet_index >= max_uap_octets) {
        throw DecodingException("UAP has more octets than expected (FX bit still set)");
    }
    
    // Valider que tous les items obligatoires sont présents
    validateMandatoryItems(present_items);
    
    return present_items;
}

bool UapSpec::isMandatory(const DataItemId& item_id) const {
    auto it = item_mandatory_.find(item_id);
    if (it != item_mandatory_.end()) {
        return it->second;
    }
    return false;
}

std::uint8_t UapSpec::getBitPosition(const DataItemId& item_id) const {
    auto it = item_to_bit_.find(item_id);
    if (it != item_to_bit_.end()) {
        return it->second;
    }
    return 0;
}

bool UapSpec::hasItem(const DataItemId& item_id) const {
    return item_to_bit_.find(item_id) != item_to_bit_.end();
}

void UapSpec::validateMandatoryItems(const std::vector<DataItemId>& present_items) const {
    std::set<DataItemId> present_set(present_items.begin(), present_items.end());
    std::vector<DataItemId> missing_mandatory;
    
    for (const auto& item : items_) {
        if (item.mandatory && present_set.find(item.item_id) == present_set.end()) {
            missing_mandatory.push_back(item.item_id);
        }
    }
    
    if (!missing_mandatory.empty()) {
        std::ostringstream oss;
        oss << "Missing mandatory Data Items: ";
        for (std::size_t i = 0; i < missing_mandatory.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << missing_mandatory[i];
        }
        throw DecodingException(oss.str());
    }
}

} // namespace asterix