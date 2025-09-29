#pragma once

#include <exception>
#include <string>
#include <stdexcept>

namespace asterix {

/**
 * @brief Classe de base pour toutes les exceptions ASTERIX
 * 
 * Toutes les exceptions spécifiques au décodage ASTERIX héritent de cette classe.
 * Elle fournit un message d'erreur descriptif et hérite de std::exception
 * pour une compatibilité avec le système d'exceptions standard C++.
 */
class AsterixException : public std::exception {
protected:
    std::string message_;  ///< Message d'erreur détaillé
    
public:
    /**
     * @brief Constructeur avec message
     * @param message Description de l'erreur
     */
    explicit AsterixException(std::string message) 
        : message_(std::move(message)) {}
    
    /**
     * @brief Obtient le message d'erreur
     * @return Message d'erreur C-string
     */
    const char* what() const noexcept override { 
        return message_.c_str(); 
    }
    
    /**
     * @brief Obtient le message d'erreur sous forme de std::string
     * @return Message d'erreur
     */
    const std::string& getMessage() const noexcept { 
        return message_; 
    }
    
    /**
     * @brief Destructeur virtuel
     */
    virtual ~AsterixException() = default;
};

// ============================================================================
// Exceptions spécifiques
// ============================================================================

/**
 * @brief Exception levée lors d'erreurs de décodage
 * 
 * Utilisée quand :
 * - Les données sont corrompues ou invalides
 * - Le format ne correspond pas à la spécification
 * - Il manque des données requises
 * - Une valeur est hors limites
 */
class DecodingException : public AsterixException {
public:
    explicit DecodingException(std::string message)
        : AsterixException("Decoding error: " + std::move(message)) {}
    
    /**
     * @brief Constructeur avec contexte additionnel
     * @param context Contexte de l'erreur (ex: "Data Item I002/010")
     * @param details Détails de l'erreur
     */
    DecodingException(const std::string& context, const std::string& details)
        : AsterixException("Decoding error in " + context + ": " + details) {}
};

/**
 * @brief Exception levée lors d'erreurs de spécification
 * 
 * Utilisée quand :
 * - Le fichier XML de spécification est invalide
 * - La spécification est incohérente
 * - Un élément requis manque dans la spécification
 * - Les types ou structures sont incorrects
 */
class SpecificationException : public AsterixException {
public:
    explicit SpecificationException(std::string message)
        : AsterixException("Specification error: " + std::move(message)) {}
    
    /**
     * @brief Constructeur avec nom de fichier
     * @param filename Fichier de spécification concerné
     * @param details Détails de l'erreur
     */
    SpecificationException(const std::string& filename, const std::string& details)
        : AsterixException("Specification error in '" + filename + "': " + details) {}
};

/**
 * @brief Exception levée lors d'accès à des données invalides
 * 
 * Utilisée quand :
 * - On essaie d'accéder à un Data Item inexistant
 * - On essaie d'accéder à un champ inexistant
 * - On essaie de convertir une valeur vers un type incompatible
 * - Un index est hors limites
 */
class InvalidDataException : public AsterixException {
public:
    explicit InvalidDataException(std::string message)
        : AsterixException("Invalid data: " + std::move(message)) {}
    
    /**
     * @brief Constructeur pour accès invalide
     * @param item_or_field Nom de l'item ou du champ
     * @param reason Raison de l'invalidité
     */
    InvalidDataException(const std::string& item_or_field, const std::string& reason)
        : AsterixException("Invalid data access to '" + item_or_field + "': " + reason) {}
};

/**
 * @brief Exception levée lors d'erreurs d'encodage
 * 
 * Utilisée quand :
 * - Les données à encoder sont invalides
 * - Le format d'encodage est incorrect
 * - Une contrainte d'encodage n'est pas respectée
 */
class EncodingException : public AsterixException {
public:
    explicit EncodingException(std::string message)
        : AsterixException("Encoding error: " + std::move(message)) {}
    
    /**
     * @brief Constructeur avec contexte
     * @param context Contexte de l'erreur
     * @param details Détails de l'erreur
     */
    EncodingException(const std::string& context, const std::string& details)
        : AsterixException("Encoding error in " + context + ": " + details) {}
};

/**
 * @brief Exception levée lors d'erreurs d'I/O
 * 
 * Utilisée quand :
 * - Un fichier ne peut pas être ouvert
 * - Une erreur de lecture/écriture survient
 * - Un chemin est invalide
 */
class IOException : public AsterixException {
public:
    explicit IOException(std::string message)
        : AsterixException("I/O error: " + std::move(message)) {}
    
    /**
     * @brief Constructeur avec nom de fichier
     * @param filename Fichier concerné
     * @param details Détails de l'erreur
     */
    IOException(const std::string& filename, const std::string& details)
        : AsterixException("I/O error with file '" + filename + "': " + details) {}
};

/**
 * @brief Exception levée lors d'erreurs de configuration
 * 
 * Utilisée quand :
 * - Un paramètre de configuration est invalide
 * - Une option requise n'est pas fournie
 * - Une configuration est incohérente
 */
class ConfigurationException : public AsterixException {
public:
    explicit ConfigurationException(std::string message)
        : AsterixException("Configuration error: " + std::move(message)) {}
};

// ============================================================================
// Macros utilitaires pour lever des exceptions avec contexte
// ============================================================================

/**
 * @def ASTERIX_THROW_DECODING
 * @brief Macro pour lever une DecodingException avec fichier et ligne
 */
#define ASTERIX_THROW_DECODING(msg) \
    throw asterix::DecodingException( \
        std::string(msg) + " [" + __FILE__ + ":" + std::to_string(__LINE__) + "]" \
    )

/**
 * @def ASTERIX_THROW_SPECIFICATION
 * @brief Macro pour lever une SpecificationException avec fichier et ligne
 */
#define ASTERIX_THROW_SPECIFICATION(msg) \
    throw asterix::SpecificationException( \
        std::string(msg) + " [" + __FILE__ + ":" + std::to_string(__LINE__) + "]" \
    )

/**
 * @def ASTERIX_THROW_INVALID_DATA
 * @brief Macro pour lever une InvalidDataException avec fichier et ligne
 */
#define ASTERIX_THROW_INVALID_DATA(msg) \
    throw asterix::InvalidDataException( \
        std::string(msg) + " [" + __FILE__ + ":" + std::to_string(__LINE__) + "]" \
    )

/**
 * @def ASTERIX_ASSERT
 * @brief Macro pour assertion avec exception
 */
#define ASTERIX_ASSERT(condition, msg) \
    if (!(condition)) { \
        throw asterix::AsterixException( \
            std::string("Assertion failed: ") + #condition + " - " + msg + \
            " [" + __FILE__ + ":" + std::to_string(__LINE__) + "]" \
        ); \
    }

} // namespace asterix