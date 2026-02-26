#pragma once
// BitStream.hpp – Big-endian (MSB-first) bit-level I/O for ASTERIX buffers.
//
// ASTERIX wire format rules:
//   • Bytes are transmitted in network byte order (big-endian).
//   • Within each byte, bit 8 (MSB) is the most significant data bit.
//   • Bit 1 (LSB) is used as the FX continuation flag in FSPEC, Extended,
//     and Repetitive items.
//   • Data Items are always byte-aligned relative to the Data Record start;
//     sub-elements within an item are bit-packed MSB-first.

#include <algorithm>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace asterix {

// ─────────────────────────────────────────────────────────────────────────────
//  BitReader
// ─────────────────────────────────────────────────────────────────────────────
// Reads bits sequentially from a read-only byte span.
// Position is tracked as a bit offset from the start of the buffer.
// Bit 0 of the position corresponds to the MSB of the first byte.
//
// Example – reading the two nibbles of 0xAB:
//   readU(4) → 0xA   readU(4) → 0xB
class BitReader {
public:
    explicit BitReader(std::span<const uint8_t> buf) noexcept
        : buf_(buf), pos_(0) {}

    // ── Position queries ─────────────────────────────────────────────────────

    [[nodiscard]] size_t bitsRead()      const noexcept { return pos_; }
    [[nodiscard]] size_t bytesRead()     const noexcept { return pos_ / 8; }
    [[nodiscard]] size_t bitsAvailable() const noexcept {
        return buf_.size() * 8 - pos_;
    }
    [[nodiscard]] bool byteAligned() const noexcept { return (pos_ % 8) == 0; }
    [[nodiscard]] bool canRead(size_t n) const noexcept {
        return bitsAvailable() >= n;
    }

    // ── Fundamental read operations ──────────────────────────────────────────

    // Read n bits as an unsigned 64-bit integer, MSB of the field first.
    // Preconditions: 1 ≤ n ≤ 64 and canRead(n).
    [[nodiscard]] uint64_t readU(size_t n) {
        boundsCheck(n);
        uint64_t result = 0;
        size_t   left   = n;
        while (left > 0) {
            const size_t byte_idx     = pos_ / 8;
            const size_t bit_in_byte  = pos_ % 8;           // 0 = MSB side
            const size_t avail        = 8 - bit_in_byte;
            const size_t chunk        = std::min(left, avail);

            // Mask the 'chunk' bits starting from bit_in_byte (MSB side).
            // e.g. bit_in_byte=0, chunk=3 → mask = 0b11100000 → shift right by 5
            const uint8_t shift  = static_cast<uint8_t>(avail - chunk);
            const uint8_t mask   = static_cast<uint8_t>(((1u << chunk) - 1u) << shift);
            const uint8_t bits   = (buf_[byte_idx] & mask) >> shift;

            result = (result << chunk) | bits;
            pos_  += chunk;
            left  -= chunk;
        }
        return result;
    }

    // Read n bits as a signed 64-bit integer (two's complement), MSB first.
    [[nodiscard]] int64_t readS(size_t n) {
        const uint64_t raw = readU(n);
        // Sign-extend if the MSB of the field is 1
        if (n > 0 && n < 64 && ((raw >> (n - 1)) & 1u)) {
            return static_cast<int64_t>(raw | (~uint64_t{0} << n));
        }
        return static_cast<int64_t>(raw);
    }

    // Read a single bit as bool.
    [[nodiscard]] bool readBit() { return readU(1) != 0; }

    // Peek at the current full byte without advancing (must be byte-aligned).
    [[nodiscard]] uint8_t peekByte() const {
        if (!byteAligned() || bytesRead() >= buf_.size())
            throw std::out_of_range("BitReader::peekByte – out of bounds or unaligned");
        return buf_[pos_ / 8];
    }

    // Skip n bits.
    void skip(size_t n) {
        boundsCheck(n);
        pos_ += n;
    }

    // Advance to the next byte boundary (no-op if already aligned).
    void alignToByte() {
        if (!byteAligned()) pos_ = (pos_ + 7) & ~size_t{7};
    }

    // ── Byte-aligned helpers ─────────────────────────────────────────────────

    // Read n complete bytes; requires current position to be byte-aligned.
    [[nodiscard]] std::vector<uint8_t> readBytes(size_t n) {
        requireAligned("readBytes");
        if (bytesRead() + n > buf_.size())
            throw std::out_of_range("BitReader::readBytes – out of bounds");
        const size_t start = pos_ / 8;
        pos_ += n * 8;
        return {buf_.begin() + static_cast<ptrdiff_t>(start),
                buf_.begin() + static_cast<ptrdiff_t>(start + n)};
    }

    // Return a sub-reader covering the next n_bytes; advances this reader.
    // Requires byte alignment.
    [[nodiscard]] BitReader subReader(size_t n_bytes) {
        requireAligned("subReader");
        if (bytesRead() + n_bytes > buf_.size())
            throw std::out_of_range("BitReader::subReader – out of bounds");
        const size_t start = pos_ / 8;
        pos_ += n_bytes * 8;
        return BitReader{buf_.subspan(start, n_bytes)};
    }

    // Return remaining buffer as a span (requires byte alignment).
    [[nodiscard]] std::span<const uint8_t> remaining() const {
        requireAligned("remaining");
        return buf_.subspan(pos_ / 8);
    }

private:
    std::span<const uint8_t> buf_;
    size_t pos_{0};

    void boundsCheck(size_t n) const {
        if (n == 0 || n > 64)
            throw std::invalid_argument("BitReader: bit count must be 1–64");
        if (!canRead(n))
            throw std::out_of_range("BitReader: read past end of buffer");
    }

    void requireAligned(const char* where) const {
        if (!byteAligned())
            throw std::logic_error(
                std::string("BitReader::") + where + " – not byte-aligned");
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  BitWriter
// ─────────────────────────────────────────────────────────────────────────────
// Appends bits MSB-first into an internal byte buffer that grows as needed.
class BitWriter {
public:
    BitWriter() = default;

    // Write n bits from value, MSB first.  Only the low n bits of value are used.
    void writeU(uint64_t value, size_t n) {
        if (n == 0 || n > 64)
            throw std::invalid_argument("BitWriter: bit count must be 1–64");
        if (n < 64) value &= (uint64_t{1} << n) - 1u; // mask to n bits

        size_t left = n;
        while (left > 0) {
            if (buf_.empty() || (pos_ % 8) == 0)
                buf_.push_back(0);

            const size_t bit_in_byte = pos_ % 8;
            const size_t avail       = 8 - bit_in_byte;
            const size_t chunk       = std::min(left, avail);

            // The top 'chunk' bits of the remaining 'left' bits of value
            const uint64_t field_shift = left - chunk;
            const uint8_t  bits = static_cast<uint8_t>(
                (value >> field_shift) & ((1u << chunk) - 1u));

            // Place them at the correct position in the current byte
            const uint8_t byte_shift = static_cast<uint8_t>(avail - chunk);
            buf_.back() |= static_cast<uint8_t>(bits << byte_shift);

            pos_  += chunk;
            left  -= chunk;
        }
    }

    // Write n bits of a two's-complement signed integer.
    void writeS(int64_t value, size_t n) {
        writeU(static_cast<uint64_t>(value), n);
    }

    // Write a single bit.
    void writeBit(bool b) { writeU(b ? 1u : 0u, 1); }

    // Append raw bytes (byte-aligned assumed by caller).
    void writeBytes(std::span<const uint8_t> data) {
        for (uint8_t b : data) writeU(b, 8);
    }

    // Write a whole byte.
    void writeByte(uint8_t b) { writeU(b, 8); }

    [[nodiscard]] const std::vector<uint8_t>& buffer()  const noexcept { return buf_; }
    [[nodiscard]] std::vector<uint8_t>        take()          noexcept { return std::move(buf_); }
    [[nodiscard]] size_t bitsWritten()         const noexcept { return pos_; }
    [[nodiscard]] bool   byteAligned()         const noexcept { return (pos_ % 8) == 0; }

private:
    std::vector<uint8_t> buf_;
    size_t pos_{0};
};

} // namespace asterix
