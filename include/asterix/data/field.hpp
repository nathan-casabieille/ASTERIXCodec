class Field {
private:
    FieldName name_;
    FieldValue value_;
    std::string unit_;
    
public:
    const FieldName& getName() const { return name_; }
    const FieldValue& getValue() const { return value_; }
    std::string getUnit() const { return unit_; }
};