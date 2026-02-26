#pragma once
// Codec.hpp – Public ASTERIX encode / decode API.
//
// Usage example:
//   Codec codec;
//   codec.registerCategory(loadSpec("specs/CAT01.xml"));
//
//   // Decode a raw frame:
//   auto block = codec.decode({raw_bytes.data(), raw_bytes.size()});
//
//   // Access fields:
//   auto& rec = block.records[0];
//   uint64_t sac = rec.items.at("010").fields.at("SAC");

#include "Types.hpp"
#include <span>
#include <unordered_map>
#include <vector>

namespace asterix {

class Codec {
public:
    // Register a category definition (loaded from XML via loadSpec()).
    // Multiple categories can be registered; each is keyed by its cat number.
    void registerCategory(CategoryDef cat);

    // Return a registered category definition (throws if not found).
    const CategoryDef& category(uint8_t cat) const;

    // ── Decode ───────────────────────────────────────────────────────────────
    // Decode a single ASTERIX Data Block from the raw byte buffer.
    // The buffer must start at the first byte of the Data Block (CAT byte).
    // Returns a DecodedBlock; check .valid and .error for problems.
    [[nodiscard]] DecodedBlock decode(std::span<const uint8_t> buf) const;

    // ── Encode ───────────────────────────────────────────────────────────────
    // Encode a single ASTERIX Data Block from a list of pre-built records.
    // Each record must carry a uap_variation and a populated items map.
    [[nodiscard]] std::vector<uint8_t> encode(uint8_t cat,
                                              const std::vector<DecodedRecord>& records) const;

private:
    std::unordered_map<uint8_t, CategoryDef> cats_;

    // Internal per-record helpers
    [[nodiscard]] DecodedRecord decodeRecord(std::span<const uint8_t> buf,
                                             const CategoryDef& cat,
                                             size_t& bytes_consumed) const;

    [[nodiscard]] std::vector<uint8_t> encodeRecord(const DecodedRecord& rec,
                                                     const CategoryDef& cat) const;

    // Item-level decode (returns number of bytes consumed from item_buf)
    [[nodiscard]] DecodedItem decodeItem(const DataItemDef& def,
                                          std::span<const uint8_t> item_buf,
                                          size_t& consumed) const;

    [[nodiscard]] std::vector<uint8_t> encodeItem(const DataItemDef& def,
                                                   const DecodedItem& val) const;

    // UAP selection helpers
    [[nodiscard]] const std::vector<std::string>&
    selectUap(const CategoryDef& cat, const DecodedRecord& partial) const;

    [[nodiscard]] std::string
    resolveVariation(const CategoryDef& cat, const DecodedRecord& partial) const;
};

} // namespace asterix
