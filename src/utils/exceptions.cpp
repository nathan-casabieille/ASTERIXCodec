#include "asterix/utils/exceptions.hpp"

// Ce fichier est principalement vide car les exceptions sont implémentées
// dans le header avec des inline implementations.
// On peut ajouter des fonctions utilitaires ici si nécessaire.

namespace asterix {

// Placeholder pour éviter un fichier complètement vide
// et permettre une future expansion

namespace detail {

/**
 * @brief Fonction utilitaire pour formater les messages d'exception
 * @param prefix Préfixe du message
 * @param message Message principal
 * @param context Contexte optionnel
 * @return Message formaté
 */
std::string formatExceptionMessage(
    const std::string& prefix,
    const std::string& message,
    const std::string& context = "") {
    
    std::string result = prefix + ": " + message;
    
    if (!context.empty()) {
        result += " (" + context + ")";
    }
    
    return result;
}

} // namespace detail

} // namespace asterix