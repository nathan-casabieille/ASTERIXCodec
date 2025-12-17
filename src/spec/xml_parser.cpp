#include "asterix/spec/xml_parser.hpp"
#include "asterix/core/asterix_category.hpp"
#include "asterix/spec/uap_spec.hpp"
#include "asterix/spec/data_item_spec.hpp"
#include "asterix/spec/field_spec.hpp"
#include "asterix/core/types.hpp"
#include "asterix/utils/exceptions.hpp"

#include <tinyxml2.h>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace asterix::spec {

AsterixCategory XmlParser::parseSpecification(const std::filesystem::path& xml_file) {
    // Vérifier que le fichier existe
    if (!std::filesystem::exists(xml_file)) {
        throwParsingError("File access", "Specification file does not exist: " + xml_file.string());
    }

    // Charger et parser le document XML
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(xml_file.string().c_str()) != tinyxml2::XML_SUCCESS) {
        throwParsingError("XML parsing", "Failed to load XML file: " + std::string(doc.ErrorStr()));
    }

    // Valider la structure XML de base
    validateXmlStructure(doc);

    // Obtenir l'élément racine
    const auto* root = doc.RootElement();
    if (!root || std::string(root->Name()) != "asterix_category") {
        throwParsingError("XML structure", "Root element must be 'asterix_category'");
    }

    // Parser les attributs de la catégorie
    std::string category_str = getRequiredAttribute(root, "number");
    std::string version = getRequiredAttribute(root, "version");
    CategoryNumber category_number = parseCategoryNumber(category_str);

    // Parser la section UAP
    const auto* uap_element = root->FirstChildElement("uap");
    if (!uap_element) {
        throwParsingError("XML structure", "Missing 'uap' section");
    }
    UapSpec uap_spec = parseUap(uap_element);

    // Parser la section data_items
    const auto* data_items_element = root->FirstChildElement("data_items");
    if (!data_items_element) {
        throwParsingError("XML structure", "Missing 'data_items' section");
    }

    std::unordered_map<DataItemId, DataItemSpec> data_items;
    
    for (const auto* item_element = data_items_element->FirstChildElement("item");
         item_element != nullptr;
         item_element = item_element->NextSiblingElement("item")) {
        
        DataItemSpec item_spec = parseDataItem(item_element);
        DataItemId item_id = getRequiredAttribute(item_element, "name");
        data_items[item_id] = std::move(item_spec);
    }

    // Vérifier que tous les items UAP ont une spécification correspondante
    for (const auto& uap_item : uap_spec.getAllItems()) {
        if (data_items.find(uap_item.item_id) == data_items.end()) {
            throwParsingError("Specification consistency", 
                "UAP references undefined data item: " + uap_item.item_id);
        }
    }

    return AsterixCategory(category_number, version, std::move(uap_spec), std::move(data_items));
}

UapSpec XmlParser::parseUap(const tinyxml2::XMLElement* uap_element) {
    validateUapStructure(uap_element);
    
    std::vector<UapSpec::UapItem> uap_items;
    
    for (const auto* item_element = uap_element->FirstChildElement("item");
         item_element != nullptr;
         item_element = item_element->NextSiblingElement("item")) {
        
        std::string bit_str = getRequiredAttribute(item_element, "bit");
        std::string name = getRequiredAttribute(item_element, "name");
        bool mandatory = getBoolAttribute(item_element, "mandatory", false);
        
        std::uint8_t bit_position = parseBitPosition(bit_str);
        
        // Vérifier que la position de bit est valide (1-7, bit 1 est réservé pour FX)
        if (bit_position < 2 || bit_position > 8) {
            throwParsingError("UAP parsing", 
                "Invalid bit position " + bit_str + " for item " + name + 
                ". Valid range is 2-8 (bit 1 reserved for FX)");
        }
                
        uap_items.push_back({bit_position, name, mandatory});
    }
    
    // Trier les items par position de bit pour une construction correcte de l'UAP
    std::sort(uap_items.begin(), uap_items.end(), 
              [](const auto& a, const auto& b) { return a.bit_position > b.bit_position; });
    
    return UapSpec(std::move(uap_items));
}

DataItemSpec XmlParser::parseDataItem(const tinyxml2::XMLElement* item_element) {
    validateDataItemStructure(item_element);
    
    DataItemId id = getRequiredAttribute(item_element, "name");
    std::string title = getRequiredAttribute(item_element, "title");
    
    DataItemSpec spec(id, title);
    
    // Parser la structure de l'item
    parseItemStructure(item_element, spec);
    
    return spec;
}

void XmlParser::parseItemStructure(const tinyxml2::XMLElement* item_element, DataItemSpec& spec) {
    // Chercher le type de structure
    const auto* fixed_length = item_element->FirstChildElement("fixed_length");
    const auto* variable_length = item_element->FirstChildElement("variable_length");
    const auto* repetitive_fixed = item_element->FirstChildElement("repetitive_fixed");
    const auto* repetitive_variable = item_element->FirstChildElement("repetitive_variable");
    
    int structure_count = (fixed_length ? 1 : 0) + (variable_length ? 1 : 0) + 
                         (repetitive_fixed ? 1 : 0) + (repetitive_variable ? 1 : 0);
    
    if (structure_count != 1) {
        throwParsingError("Data item structure", 
            "Item " + spec.getId() + " must have exactly one structure type");
    }
    
    if (fixed_length) {
        std::string bytes_str = getRequiredAttribute(fixed_length, "bytes");
        std::uint16_t length = parseByteSize(bytes_str);
        spec.setStructure(ItemStructure::FixedLength, length);
        
        // Parser les champs
        std::vector<FieldSpec> fields;
        for (const auto* field_element = fixed_length->FirstChildElement("field");
             field_element != nullptr;
             field_element = field_element->NextSiblingElement("field")) {
            fields.push_back(parseField(field_element));
        }
        spec.setFields(std::move(fields));
        
    } else if (variable_length) {
        spec.setStructure(ItemStructure::VariableLength);
        
        // Vérifier s'il y a des bits FX
        bool has_fx = getBoolAttribute(variable_length, "has_fx", false);
        spec.setHasFxBits(has_fx);
        
        // Parser les champs
        std::vector<FieldSpec> fields;
        for (const auto* field_element = variable_length->FirstChildElement("field");
             field_element != nullptr;
             field_element = field_element->NextSiblingElement("field")) {
            fields.push_back(parseField(field_element));
        }
        spec.setFields(std::move(fields));
        
    } else if (repetitive_fixed) {
        std::string bytes_str = getRequiredAttribute(repetitive_fixed, "bytes");
        std::uint16_t rep_length = parseByteSize(bytes_str);
        spec.setStructure(ItemStructure::RepetitiveFixed, rep_length);
        
        // Parser les champs
        std::vector<FieldSpec> fields;
        for (const auto* field_element = repetitive_fixed->FirstChildElement("field");
             field_element != nullptr;
             field_element = field_element->NextSiblingElement("field")) {
            fields.push_back(parseField(field_element));
        }
        spec.setFields(std::move(fields));
        
    } else if (repetitive_variable) {
        spec.setStructure(ItemStructure::RepetitiveVariable);
        
        // Parser les champs
        std::vector<FieldSpec> fields;
        for (const auto* field_element = repetitive_variable->FirstChildElement("field");
             field_element != nullptr;
             field_element = field_element->NextSiblingElement("field")) {
            fields.push_back(parseField(field_element));
        }
        spec.setFields(std::move(fields));
    }
}

FieldSpec XmlParser::parseField(const tinyxml2::XMLElement* field_element) {
    validateFieldStructure(field_element);
    
    FieldName name = getRequiredAttribute(field_element, "name");
    std::string type_str = getRequiredAttribute(field_element, "type");
    std::string bits_str = getRequiredAttribute(field_element, "bits");
    std::string unit = getOptionalAttribute(field_element, "unit", "none");
    
    std::uint8_t bit_size = parseBitSize(bits_str);
    
    // Déterminer le type de champ
    FieldType type;
    if (type_str == "unsigned") {
        type = FieldType::Unsigned;
    } else if (type_str == "signed") {
        type = FieldType::Signed;
    } else if (type_str == "boolean") {
        type = FieldType::Boolean;
        if (bit_size != 1) {
            throwParsingError("Field parsing", 
                "Boolean field " + name + " must have exactly 1 bit");
        }
    } else if (type_str == "enum") {
        type = FieldType::Enumeration;
    } else if (type_str == "string") {
        type = FieldType::String;
    } else if (type_str == "raw") {
        type = FieldType::Raw;
    } else {
        throwParsingError("Field parsing", 
            "Unknown field type: " + type_str + " for field " + name);
    }
    
    FieldSpec spec(name, type, bit_size, unit);
    
    // Parser les attributs optionnels
    std::string scale_str = getOptionalAttribute(field_element, "scale");
    if (!scale_str.empty()) {
        double scale_factor = parseScaleFactor(scale_str);
        spec.setScaleFactor(scale_factor);
    }
    
    std::string offset_str = getOptionalAttribute(field_element, "offset");
    if (!offset_str.empty()) {
        std::int64_t offset = parseOffset(offset_str);
        spec.setOffset(offset);
    }
    
    // Parser les valeurs d'énumération si applicable
    if (type == FieldType::Enumeration) {
        auto enum_values = parseEnumValues(field_element);
        spec.setEnumValues(std::move(enum_values));
    }
    
    return spec;
}

std::unordered_map<std::uint64_t, std::string> XmlParser::parseEnumValues(
    const tinyxml2::XMLElement* field_element) {
    
    std::unordered_map<std::uint64_t, std::string> enum_values;
    
    for (const auto* enum_element = field_element->FirstChildElement("enum_value");
         enum_element != nullptr;
         enum_element = enum_element->NextSiblingElement("enum_value")) {
        
        std::string key_str = getRequiredAttribute(enum_element, "key");
        std::string value = getRequiredAttribute(enum_element, "value");
        
        try {
            std::uint64_t key = std::stoull(key_str, nullptr, 0); // Support hex with 0x prefix
            enum_values[key] = value;
        } catch (const std::exception& e) {
            throwParsingError("Enum parsing", 
                "Invalid enum key '" + key_str + "': " + e.what());
        }
    }
    
    return enum_values;
}

std::vector<FieldSpec> XmlParser::parseCompoundFields(const tinyxml2::XMLElement* compound_element) {
    std::vector<FieldSpec> fields;
    
    for (const auto* field_element = compound_element->FirstChildElement("field");
         field_element != nullptr;
         field_element = field_element->NextSiblingElement("field")) {
        fields.push_back(parseField(field_element));
    }
    
    return fields;
}

// Utilitaires de parsing et validation

CategoryNumber XmlParser::parseCategoryNumber(const std::string& category_str) {
    try {
        int category = std::stoi(category_str);
        if (category < 0 || category > 255) {
            throwParsingError("Category parsing", 
                "Category number must be between 0 and 255, got: " + category_str);
        }
        return static_cast<CategoryNumber>(category);
    } catch (const std::exception& e) {
        throwParsingError("Category parsing", 
            "Invalid category number '" + category_str + "': " + e.what());
    }
}

std::uint8_t XmlParser::parseBitPosition(const std::string& bit_str) {
    try {
        int bit = std::stoi(bit_str);
        if (bit < 1 || bit > 8) {
            throwParsingError("Bit position parsing", 
                "Bit position must be between 1 and 8, got: " + bit_str);
        }
        return static_cast<std::uint8_t>(bit);
    } catch (const std::exception& e) {
        throwParsingError("Bit position parsing", 
            "Invalid bit position '" + bit_str + "': " + e.what());
    }
}

std::uint16_t XmlParser::parseByteSize(const std::string& bytes_str) {
    try {
        int bytes = std::stoi(bytes_str);
        if (bytes < 1 || bytes > 65535) {
            throwParsingError("Byte size parsing", 
                "Byte size must be between 1 and 65535, got: " + bytes_str);
        }
        return static_cast<std::uint16_t>(bytes);
    } catch (const std::exception& e) {
        throwParsingError("Byte size parsing", 
            "Invalid byte size '" + bytes_str + "': " + e.what());
    }
}

std::uint8_t XmlParser::parseBitSize(const std::string& bits_str) {
    try {
        int bits = std::stoi(bits_str);
        if (bits < 1 || bits > 64) {
            throwParsingError("Bit size parsing", 
                "Bit size must be between 1 and 64, got: " + bits_str);
        }
        return static_cast<std::uint8_t>(bits);
    } catch (const std::exception& e) {
        throwParsingError("Bit size parsing", 
            "Invalid bit size '" + bits_str + "': " + e.what());
    }
}

double XmlParser::parseScaleFactor(const std::string& scale_str) {
    try {
        return std::stod(scale_str);
    } catch (const std::exception& e) {
        throwParsingError("Scale factor parsing", 
            "Invalid scale factor '" + scale_str + "': " + e.what());
    }
}

std::int64_t XmlParser::parseOffset(const std::string& offset_str) {
    try {
        return std::stoll(offset_str);
    } catch (const std::exception& e) {
        throwParsingError("Offset parsing", 
            "Invalid offset '" + offset_str + "': " + e.what());
    }
}

// Fonctions de validation

void XmlParser::validateXmlStructure(const tinyxml2::XMLDocument& doc) {
    if (!doc.RootElement()) {
        throwParsingError("XML validation", "No root element found");
    }
    
    const auto* root = doc.RootElement();
    if (std::string(root->Name()) != "asterix_category") {
        throwParsingError("XML validation", 
            "Root element must be 'asterix_category', found: " + std::string(root->Name()));
    }
    
    // Vérifier les attributs requis
    if (!root->Attribute("number")) {
        throwParsingError("XML validation", "Missing required attribute 'number' in asterix_category");
    }
    
    if (!root->Attribute("version")) {
        throwParsingError("XML validation", "Missing required attribute 'version' in asterix_category");
    }
}

void XmlParser::validateUapStructure(const tinyxml2::XMLElement* uap_element) {
    if (!uap_element->FirstChildElement("item")) {
        throwParsingError("UAP validation", "UAP section must contain at least one item");
    }
    
    // NOTE: Dans ASTERIX, les UAP multi-octets réutilisent les positions de bits 2-8
    // pour chaque octet. C'est normal et attendu. On ne vérifie donc PAS l'unicité 
    // des positions de bits, seulement l'unicité des noms d'items.
    
    std::set<std::string> used_names;
    
    for (const auto* item = uap_element->FirstChildElement("item");
         item != nullptr;
         item = item->NextSiblingElement("item")) {
        
        // Vérifier les attributs requis
        std::string bit_str = getRequiredAttribute(item, "bit");
        std::string name = getRequiredAttribute(item, "name");
        
        std::uint8_t bit_pos = parseBitPosition(bit_str);
        
        // Vérifier l'unicité des noms (les doublons de noms ne sont jamais permis)
        if (used_names.count(name)) {
            throwParsingError("UAP validation", 
                "Duplicate item name '" + name + "' in UAP");
        }
        used_names.insert(name);
        
        // Les positions de bits peuvent être dupliquées (multi-octet UAP)
        // donc on ne vérifie PAS l'unicité des positions de bits
    }
}

void XmlParser::validateDataItemStructure(const tinyxml2::XMLElement* item_element) {
    // Vérifier les attributs requis
    getRequiredAttribute(item_element, "name");
    getRequiredAttribute(item_element, "title");
    
    // Vérifier qu'il y a exactement un type de structure
    const auto* fixed_length = item_element->FirstChildElement("fixed_length");
    const auto* variable_length = item_element->FirstChildElement("variable_length");
    const auto* repetitive_fixed = item_element->FirstChildElement("repetitive_fixed");
    const auto* repetitive_variable = item_element->FirstChildElement("repetitive_variable");
    
    int structure_count = (fixed_length ? 1 : 0) + (variable_length ? 1 : 0) + 
                         (repetitive_fixed ? 1 : 0) + (repetitive_variable ? 1 : 0);
    
    if (structure_count == 0) {
        throwParsingError("Data item validation", 
            "Data item must have one structure type (fixed_length, variable_length, repetitive_fixed, or repetitive_variable)");
    }
    
    if (structure_count > 1) {
        throwParsingError("Data item validation", 
            "Data item can have only one structure type");
    }
}

void XmlParser::validateFieldStructure(const tinyxml2::XMLElement* field_element) {
    // Vérifier les attributs requis
    getRequiredAttribute(field_element, "name");
    getRequiredAttribute(field_element, "type");
    getRequiredAttribute(field_element, "bits");
}

// Utilitaires d'accès aux attributs

std::string XmlParser::getRequiredAttribute(const tinyxml2::XMLElement* element, 
                                          const char* attr_name) {
    const char* attr_value = element->Attribute(attr_name);
    if (!attr_value) {
        throwParsingError("Attribute access", 
            "Missing required attribute '" + std::string(attr_name) + 
            "' in element '" + element->Name() + "'");
    }
    return std::string(attr_value);
}

std::string XmlParser::getOptionalAttribute(const tinyxml2::XMLElement* element, 
                                          const char* attr_name, 
                                          const std::string& default_value) {
    const char* attr_value = element->Attribute(attr_name);
    return attr_value ? std::string(attr_value) : default_value;
}

bool XmlParser::getBoolAttribute(const tinyxml2::XMLElement* element, 
                               const char* attr_name, 
                               bool default_value) {
    const char* attr_value = element->Attribute(attr_name);
    if (!attr_value) {
        return default_value;
    }
    
    std::string value_str(attr_value);
    std::transform(value_str.begin(), value_str.end(), value_str.begin(), ::tolower);
    
    if (value_str == "true" || value_str == "1" || value_str == "yes") {
        return true;
    } else if (value_str == "false" || value_str == "0" || value_str == "no") {
        return false;
    } else {
        throwParsingError("Boolean attribute parsing", 
            "Invalid boolean value '" + std::string(attr_value) + 
            "' for attribute '" + attr_name + "'");
    }
}

void XmlParser::throwParsingError(const std::string& context, const std::string& details) {
    throw SpecificationException("[" + context + "] " + details);
}

} // namespace asterix::spec