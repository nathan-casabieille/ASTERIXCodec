class FieldValue {
private:
    std::variant
        std::uint64_t,      // unsigned
        std::int64_t,       // signed  
        bool,               // boolean
        std::string,        // enum/string
        std::vector<std::uint8_t>  // raw
    > value_;
    FieldType type_;
    
public:
    template<typename T> T as() const;
    std::uint64_t asUInt() const;
    std::int64_t asInt() const;
    bool asBool() const;
    std::string asEnum() const;
    std::string asString() const;
    std::vector<std::uint8_t> asRaw() const;
    
    FieldType getType() const { return type_; }
    std::string toString() const;
};