class BitReader {
private:
    const ByteBuffer& buffer_;
    std::size_t byte_offset_;
    std::uint8_t bit_offset_;
    
public:
    BitReader(const ByteBuffer& buffer, std::size_t offset);
    
    bool readBit();
    std::uint64_t readBits(std::uint8_t count);
    std::uint64_t readUnsigned(std::uint8_t bits);
    std::int64_t readSigned(std::uint8_t bits);
    
    void skipBits(std::uint8_t count);
    void alignToByte();
};