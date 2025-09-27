class XmlParser {
public:
    static AsterixCategory parseSpecification(const std::filesystem::path& xml_file);
    
private:
    static UapSpec parseUap(const tinyxml2::XMLElement* uap_element);
    static DataItemSpec parseDataItem(const tinyxml2::XMLElement* item_element);
    static FieldSpec parseField(const tinyxml2::XMLElement* field_element);
};