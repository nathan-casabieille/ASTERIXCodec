class FieldSpec {
private:
    FieldName name_;
    FieldType type_;
    std::uint8_t bit_size_;
    std::string unit_;
    double scale_factor_{1.0};
    std::int64_t offset_{0};
    std::unordered_map<std::uint64_t, std::string> enum_values_;
    
public:
    FieldValue decode(const BitReader& reader) const;
    FieldType getType() const { return type_; }
    std::uint8_t getBitSize() const { return bit_size_; }
};