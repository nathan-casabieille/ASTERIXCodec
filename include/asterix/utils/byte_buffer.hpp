#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include <string>

namespace asterix {

/**
 * @brief Classe pour gérer un buffer d'octets avec lecture big-endian
 * 
 * ByteBuffer encapsule un vecteur d'octets et fournit des méthodes pour lire
 * des valeurs de différentes tailles en format big-endian (réseau),
 * conformément au standard ASTERIX.
 */
class ByteBuffer {
private:
    std::vector<std::uint8_t> data_;  ///< Données du buffer
    
    /**
     * @brief Vérifie qu'une lecture est possible à une position donnée
     * @param offset Position de lecture
     * @param length Nombre d'octets à lire
     * @throws DecodingException si la lecture dépasserait la fin du buffer
     */
    void checkBounds(std::size_t offset, std::size_t length) const;

public:
    /**
     * @brief Constructeur à partir d'un vecteur d'octets
     * @param data Données à encapsuler
     */
    explicit ByteBuffer(std::vector<std::uint8_t> data);
    
    /**
     * @brief Constructeur à partir d'une chaîne hexadécimale
     * @param hex_string Chaîne au format hexadécimal (ex: "0A1B2C" ou "0A 1B 2C")
     * @throws DecodingException si la chaîne n'est pas valide
     */
    explicit ByteBuffer(const std::string& hex_string);
    
    /**
     * @brief Constructeur à partir de pointeurs bruts
     * @param data Pointeur vers les données
     * @param size Taille des données en octets
     */
    ByteBuffer(const std::uint8_t* data, std::size_t size);
    
    /**
     * @brief Constructeur par défaut (buffer vide)
     */
    ByteBuffer() = default;
    
    /**
     * @brief Lit un octet à une position donnée
     * @param offset Position de lecture
     * @return Valeur de l'octet
     * @throws DecodingException si offset >= size()
     */
    std::uint8_t readByte(std::size_t offset) const;
    
    /**
     * @brief Lit un entier 16 bits en big-endian
     * @param offset Position de lecture
     * @return Valeur 16 bits
     * @throws DecodingException si lecture hors limites
     */
    std::uint16_t readUInt16BE(std::size_t offset) const;
    
    /**
     * @brief Lit un entier 24 bits en big-endian
     * @param offset Position de lecture
     * @return Valeur 24 bits (stockée dans uint32_t)
     * @throws DecodingException si lecture hors limites
     */
    std::uint32_t readUInt24BE(std::size_t offset) const;
    
    /**
     * @brief Lit un entier 32 bits en big-endian
     * @param offset Position de lecture
     * @return Valeur 32 bits
     * @throws DecodingException si lecture hors limites
     */
    std::uint32_t readUInt32BE(std::size_t offset) const;
    
    /**
     * @brief Lit un entier 64 bits en big-endian
     * @param offset Position de lecture
     * @return Valeur 64 bits
     * @throws DecodingException si lecture hors limites
     */
    std::uint64_t readUInt64BE(std::size_t offset) const;
    
    /**
     * @brief Lit plusieurs octets dans un vecteur
     * @param offset Position de lecture
     * @param length Nombre d'octets à lire
     * @return Vecteur contenant les octets lus
     * @throws DecodingException si lecture hors limites
     */
    std::vector<std::uint8_t> readBytes(std::size_t offset, std::size_t length) const;
    
    /**
     * @brief Obtient la taille du buffer
     * @return Nombre d'octets dans le buffer
     */
    std::size_t size() const { return data_.size(); }
    
    /**
     * @brief Obtient un pointeur constant vers les données
     * @return Pointeur vers les données brutes
     */
    const std::uint8_t* data() const { return data_.data(); }
    
    /**
     * @brief Vérifie si le buffer est vide
     * @return true si le buffer ne contient aucun octet
     */
    bool empty() const { return data_.empty(); }
    
    /**
     * @brief Accès direct à un octet (sans vérification)
     * @param index Index de l'octet
     * @return Référence constante vers l'octet
     */
    const std::uint8_t& operator[](std::size_t index) const {
        return data_[index];
    }
    
    /**
     * @brief Obtient le vecteur de données sous-jacent
     * @return Référence constante vers le vecteur
     */
    const std::vector<std::uint8_t>& getData() const { return data_; }
    
    /**
     * @brief Convertit le buffer en chaîne hexadécimale
     * @param with_spaces Si true, ajoute des espaces entre les octets
     * @return Représentation hexadécimale du buffer
     */
    std::string toHexString(bool with_spaces = true) const;
    
    /**
     * @brief Crée un sous-buffer
     * @param offset Position de départ
     * @param length Longueur du sous-buffer (0 = jusqu'à la fin)
     * @return Nouveau ByteBuffer contenant la sous-section
     * @throws DecodingException si les paramètres sont invalides
     */
    ByteBuffer slice(std::size_t offset, std::size_t length = 0) const;
    
    /**
     * @brief Réserve de l'espace dans le buffer
     * @param capacity Capacité à réserver
     */
    void reserve(std::size_t capacity) {
        data_.reserve(capacity);
    }
    
    /**
     * @brief Ajoute un octet à la fin du buffer
     * @param byte Octet à ajouter
     */
    void append(std::uint8_t byte) {
        data_.push_back(byte);
    }
    
    /**
     * @brief Ajoute plusieurs octets à la fin du buffer
     * @param bytes Octets à ajouter
     */
    void append(const std::vector<std::uint8_t>& bytes) {
        data_.insert(data_.end(), bytes.begin(), bytes.end());
    }
    
    /**
     * @brief Efface le contenu du buffer
     */
    void clear() {
        data_.clear();
    }
};

/**
 * @brief Convertit une chaîne hexadécimale en vecteur d'octets
 * @param hex_string Chaîne hexadécimale
 * @return Vecteur d'octets
 * @throws DecodingException si la chaîne est invalide
 */
std::vector<std::uint8_t> hexStringToBytes(const std::string& hex_string);

/**
 * @brief Convertit un vecteur d'octets en chaîne hexadécimale
 * @param bytes Vecteur d'octets
 * @param with_spaces Si true, ajoute des espaces entre les octets
 * @return Chaîne hexadécimale
 */
std::string bytesToHexString(const std::vector<std::uint8_t>& bytes, bool with_spaces = true);

} // namespace asterix