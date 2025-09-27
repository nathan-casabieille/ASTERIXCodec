class ByteBuffer {
private:
    std::vector<std::uint8_t> data_;
    
public:
    explicit ByteBuffer(std::vector<std::uint8_t> data);
    explicit ByteBuffer(const std::string& hex_string);
    
    std::uint8_t readByte(std::size_t offset) const;
    std::uint16_t readUInt16BE(std::size_t offset) const;
    std::uint32_t readUInt24BE(std::size_t offset) const;
    std::uint32_t readUInt32BE(std::size_t offset) const;
    
    std::size_t size() const { return data_.size(); }
    const std::uint8_t* data() const { return data_.data(); }
};