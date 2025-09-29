#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>

// Forward declarations pour éviter les dépendances circulaires
namespace tinyxml2 {
    class XMLDocument;
    class XMLElement;
}

namespace asterix {
    class AsterixCategory;
    class UapSpec;
    class DataItemSpec; 
    class FieldSpec;
    using CategoryNumber = std::uint8_t;
    using DataItemId = std::string;
}

namespace asterix::spec {

/**
 * @brief Parser XML pour les spécifications ASTERIX Eurocontrol
 * 
 * Cette classe s'occupe de parser les fichiers XML de spécification ASTERIX
 * et de créer les objets correspondants (AsterixCategory, UapSpec, DataItemSpec, etc.)
 */
class XmlParser {
public:
    /**
     * @brief Parse un fichier XML de spécification ASTERIX
     * @param xml_file Chemin vers le fichier XML
     * @return AsterixCategory configurée selon le fichier XML
     * @throws SpecificationException si le fichier est invalide ou corrompu
     */
    static AsterixCategory parseSpecification(const std::filesystem::path& xml_file);

private:
    /**
     * @brief Parse la section UAP du XML
     * @param uap_element Élément XML <uap>
     * @return UapSpec configuré
     */
    static UapSpec parseUap(const tinyxml2::XMLElement* uap_element);

    /**
     * @brief Parse un Data Item du XML
     * @param item_element Élément XML <item> dans la section <data_items>
     * @return DataItemSpec configuré
     */
    static DataItemSpec parseDataItem(const tinyxml2::XMLElement* item_element);

    /**
     * @brief Parse un champ (field) d'un Data Item
     * @param field_element Élément XML <field>
     * @return FieldSpec configuré
     */
    static FieldSpec parseField(const tinyxml2::XMLElement* field_element);

    /**
     * @brief Parse la structure d'un Data Item (fixed_length, variable_length, etc.)
     * @param item_element Élément XML <item> dans la section <data_items>
     * @param spec DataItemSpec à configurer
     */
    static void parseItemStructure(const tinyxml2::XMLElement* item_element, DataItemSpec& spec);

    /**
     * @brief Parse les valeurs d'énumération d'un champ
     * @param field_element Élément XML <field> contenant des <enum_value>
     * @return Map des valeurs d'énumération (clé -> valeur)
     */
    static std::unordered_map<std::uint64_t, std::string> parseEnumValues(
        const tinyxml2::XMLElement* field_element
    );

    /**
     * @brief Parse les champs composés (compound fields)
     * @param compound_element Élément XML contenant la structure composée
     * @return Vecteur de FieldSpec pour les sous-champs
     */
    static std::vector<FieldSpec> parseCompoundFields(const tinyxml2::XMLElement* compound_element);

    /**
     * @brief Utilitaires de validation et conversion
     */
    static CategoryNumber parseCategoryNumber(const std::string& category_str);
    static std::uint8_t parseBitPosition(const std::string& bit_str);
    static std::uint16_t parseByteSize(const std::string& bytes_str);
    static std::uint8_t parseBitSize(const std::string& bits_str);
    static double parseScaleFactor(const std::string& scale_str);
    static std::int64_t parseOffset(const std::string& offset_str);
    
    /**
     * @brief Validation du XML
     */
    static void validateXmlStructure(const tinyxml2::XMLDocument& doc);
    static void validateUapStructure(const tinyxml2::XMLElement* uap_element);
    static void validateDataItemStructure(const tinyxml2::XMLElement* item_element);
    static void validateFieldStructure(const tinyxml2::XMLElement* field_element);

    /**
     * @brief Utilitaires d'accès aux attributs XML avec validation
     */
    static std::string getRequiredAttribute(const tinyxml2::XMLElement* element, 
                                          const char* attr_name);
    static std::string getOptionalAttribute(const tinyxml2::XMLElement* element, 
                                          const char* attr_name, 
                                          const std::string& default_value = "");
    static bool getBoolAttribute(const tinyxml2::XMLElement* element, 
                               const char* attr_name, 
                               bool default_value = false);

    /**
     * @brief Gestion d'erreurs
     */
    static void throwParsingError(const std::string& context, const std::string& details);
};

} // namespace asterix::spec