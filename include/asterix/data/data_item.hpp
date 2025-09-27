class DataItem {
private:
    DataItemId id_;
    std::string title_;
    std::unordered_map<FieldName, Field> fields_;
    std::vector<DataItem> repetitions_; // Pour les items répétitifs
    
public:
    const Field& getField(const FieldName& name) const;
    bool hasField(const FieldName& name) const;
    std::vector<FieldName> getFieldNames() const;
    const std::vector<DataItem>& getRepetitions() const;
};