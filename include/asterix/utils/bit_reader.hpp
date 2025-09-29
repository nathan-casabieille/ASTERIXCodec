#pragma once

#include "asterix/utils/byte_buffer.hpp"
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace asterix {

/**
 * @brief Classe pour lire des données bit par bit depuis un ByteBuffer
 * 
 * BitReader permet de lire des champs ASTERIX qui ne sont pas alignés sur des octets.
 * Il maintient une position courante en octets et en bits, et gère automatiquement
 * les transitions entre octets.
 * 
 * Les bits sont numérotés du MSB (bit 8) au LSB (bit 1) dans chaque octet,
 * conformément à la convention ASTERIX.
 */
class BitReader {
private:
    const ByteBuffer& buffer_;    ///< Référence vers le buffer de données
    std::size_t byte_offset_;     ///< Position courante en octets
    std::uint8_t bit_offset_;     ///< Position courante en bits (0-7)
    
    /**
     * @brief Vérifie qu'il reste assez de bits à lire
     * @param num_bits Nombre de bits à vérifier
     * @throws DecodingException si pas assez de bits disponibles
     */
    void checkAvailable(std::uint8_t num_bits) const;
    
    /**
     * @brief Calcule le nombre total de bits restants
     * @return Nombre de bits disponibles depuis la position courante
     */
    std::size_t getRemainingBits() const;

public:
    /**
     * @brief Constructeur
     * @param buffer Buffer contenant les données à lire
     * @param offset Position de départ en octets
     */
    BitReader(const ByteBuffer& buffer, std::size_t offset);
    
    /**
     * @brief Lit un seul bit
     * @return Valeur du bit (true si 1, false si 0)
     * @throws DecodingException si fin du buffer atteinte
     */
    bool readBit();
    
    /**
     * @brief Lit plusieurs bits et retourne la valeur brute
     * @param count Nombre de bits à lire (1-64)
     * @return Valeur brute des bits lus
     * @throws DecodingException si count > 64 ou fin du buffer atteinte
     */
    std::uint64_t readBits(std::uint8_t count);
    
    /**
     * @brief Lit un entier non signé sur un nombre de bits donné
     * @param bits Nombre de bits à lire (1-64)
     * @return Valeur non signée
     * @throws DecodingException si bits > 64 ou fin du buffer atteinte
     */
    std::uint64_t readUnsigned(std::uint8_t bits);
    
    /**
     * @brief Lit un entier signé sur un nombre de bits donné (complément à 2)
     * @param bits Nombre de bits à lire (1-64)
     * @return Valeur signée
     * @throws DecodingException si bits > 64 ou fin du buffer atteinte
     */
    std::int64_t readSigned(std::uint8_t bits);
    
    /**
     * @brief Saute un nombre de bits sans les lire
     * @param count Nombre de bits à sauter
     * @throws DecodingException si fin du buffer atteinte
     */
    void skipBits(std::uint8_t count);
    
    /**
     * @brief Aligne la position de lecture sur le prochain octet
     * 
     * Si la position courante est au milieu d'un octet, avance jusqu'au
     * début de l'octet suivant. Si déjà aligné, ne fait rien.
     */
    void alignToByte();
    
    /**
     * @brief Obtient la position courante en octets
     * @return Position en octets
     */
    std::size_t getByteOffset() const { return byte_offset_; }
    
    /**
     * @brief Obtient la position courante en bits dans l'octet courant
     * @return Position en bits (0-7)
     */
    std::uint8_t getBitOffset() const { return bit_offset_; }
    
    /**
     * @brief Vérifie si la lecture est alignée sur un octet
     * @return true si bit_offset_ == 0
     */
    bool isAligned() const { return bit_offset_ == 0; }
    
    /**
     * @brief Obtient le nombre d'octets restants dans le buffer
     * @return Nombre d'octets restants
     */
    std::size_t getRemainingBytes() const;
    
    /**
     * @brief Vérifie s'il reste des données à lire
     * @return true si au moins un bit est disponible
     */
    bool hasData() const;
    
    /**
     * @brief Réinitialise la position de lecture
     * @param offset Nouvelle position en octets
     */
    void reset(std::size_t offset);
};

} // namespace asterix