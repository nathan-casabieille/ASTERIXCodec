#pragma once

#include "asterix/core/types.hpp"
#include "asterix/core/asterix_category.hpp"
#include "asterix/core/asterix_message.hpp"
#include "asterix/utils/byte_buffer.hpp"
#include <vector>
#include <string>
#include <cstdint>

namespace asterix {

/**
 * @brief Décodeur de messages ASTERIX
 * 
 * Utilise une spécification AsterixCategory pour décoder des messages ASTERIX bruts.
 * Le décodeur suit le processus standard ASTERIX :
 * 1. Lire l'en-tête (CAT + LEN)
 * 2. Décoder les Data Records jusqu'à consommer toute la longueur
 * 3. Pour chaque record : décoder l'UAP puis les Data Items
 */
class AsterixDecoder {
private:
    const AsterixCategory& category_;
    
    /**
     * @brief Décode l'en-tête ASTERIX
     * @param buffer Buffer contenant les données
     * @param offset Position courante (sera mise à jour)
     * @param out_category Catégorie décodée (output)
     * @param out_length Longueur du message (output)
     * @throws DecodingException si l'en-tête est invalide
     */
    void decodeHeader(const ByteBuffer& buffer, std::size_t& offset,
                     CategoryNumber& out_category, std::uint16_t& out_length) const;
    
    /**
     * @brief Décode un Data Record
     * @param buffer Buffer contenant les données
     * @param offset Position courante (sera mise à jour)
     * @param end_offset Position maximale (fin du Data Block)
     * @return Record décodé
     * @throws DecodingException si le décodage échoue
     */
    AsterixRecord decodeRecord(const ByteBuffer& buffer, 
                               std::size_t& offset,
                               std::size_t end_offset) const;
    
    /**
     * @brief Décode l'UAP pour obtenir la liste des Data Items présents
     * @param buffer Buffer contenant les données
     * @param offset Position courante (sera mise à jour)
     * @return Liste ordonnée des identifiants de Data Items présents
     * @throws DecodingException si l'UAP est invalide
     */
    std::vector<DataItemId> decodeUap(const ByteBuffer& buffer, std::size_t& offset) const;
    
    /**
     * @brief Décode tous les Data Items présents
     * @param buffer Buffer contenant les données
     * @param offset Position courante (sera mise à jour)
     * @param present_items Liste des Data Items à décoder
     * @return Map des Data Items décodés
     * @throws DecodingException si le décodage échoue
     */
    std::unordered_map<DataItemId, DataItem> decodeDataItems(
        const ByteBuffer& buffer, 
        std::size_t& offset,
        const std::vector<DataItemId>& present_items
    ) const;
    
    /**
     * @brief Valide qu'un message appartient bien à cette catégorie
     * @param message_category Catégorie du message
     * @throws DecodingException si la catégorie ne correspond pas
     */
    void validateCategory(CategoryNumber message_category) const;
    
    /**
     * @brief Valide que la longueur du message est cohérente
     * @param declared_length Longueur déclarée dans l'en-tête
     * @param actual_length Longueur réelle des données
     * @throws DecodingException si incohérence détectée
     */
    void validateLength(std::uint16_t declared_length, std::size_t actual_length) const;

public:
    /**
     * @brief Constructeur
     * @param category Spécification de la catégorie ASTERIX à utiliser
     */
    explicit AsterixDecoder(const AsterixCategory& category);
    
    /**
     * @brief Décode un message ASTERIX depuis un ByteBuffer
     * @param buffer Buffer contenant les données brutes
     * @return Message ASTERIX décodé (peut contenir plusieurs records)
     * @throws DecodingException si le décodage échoue
     */
    AsterixMessage decode(const ByteBuffer& buffer) const;
    
    /**
     * @brief Décode un message ASTERIX depuis une chaîne hexadécimale
     * @param hex_data Données au format hexadécimal (ex: "22 00 0C ...")
     * @return Message ASTERIX décodé
     * @throws DecodingException si le décodage échoue
     */
    AsterixMessage decode(const std::string& hex_data) const;
    
    /**
     * @brief Décode un message ASTERIX depuis un vecteur d'octets
     * @param data Vecteur d'octets bruts
     * @return Message ASTERIX décodé
     * @throws DecodingException si le décodage échoue
     */
    AsterixMessage decode(const std::vector<std::uint8_t>& data) const;
    
    /**
     * @brief Obtient la catégorie associée à ce décodeur
     * @return Référence vers la spécification de catégorie
     */
    const AsterixCategory& getCategory() const { return category_; }
};

} // namespace asterix