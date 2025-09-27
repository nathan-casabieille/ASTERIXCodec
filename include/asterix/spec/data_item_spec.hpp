class DataItemSpec {
private:
    DataItemId id_;
    std::string title_;
    ItemStructure structure_;
    std::uint16_t fixed_length_{0};
    std::vector<FieldSpec> fields_;
    bool has_fx_bits_{false};
    
public:
    std::unique_ptr<DataItem> decode(const ByteBuffer& buffer, std::size_t& offset) const;
    ItemStructure getStructure() const { return structure_; }
    const std::vector<FieldSpec>& getFields() const { return fields_; }
};