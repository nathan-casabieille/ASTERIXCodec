class UapSpec {
private:
    struct UapItem {
        std::uint8_t bit_position;
        DataItemId item_id;
        bool mandatory;
    };
    std::vector<UapItem> items_;
    
public:
    std::vector<DataItemId> decodeUap(const ByteBuffer& buffer, std::size_t& offset) const;
    bool isMandatory(const DataItemId& item_id) const;
    std::uint8_t getBitPosition(const DataItemId& item_id) const;
};