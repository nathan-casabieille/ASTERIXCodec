#pragma once
// Types.hpp – Core metadata and decoded-value types for the ASTERIX codec.
// All ASTERIX data flows through these structures.

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace asterix {

// ─── Encoding describes how raw bits are interpreted ──────────────────────────
enum class Encoding {
    Raw,              // Treat bits as an opaque unsigned integer
    Table,            // Map raw integer → human-readable string
    UnsignedQuantity, // Physical value = scale × raw_bits  [unit]
    SignedQuantity,   // Physical value = scale × twos_complement(raw_bits)  [unit]
    StringOctal,      // 12-bit squawk stored as octal digits (Mode-2/3A)
    Spare,            // Bits to skip; no decoded output
};

// ─── Item structural type ─────────────────────────────────────────────────────
enum class ItemType {
    Fixed,             // Fixed byte-length; one or more sub-elements
    Extended,          // Variable octets, each with a trailing FX bit
    Repetitive,        // N octets, each = 7-bit value + FX bit (list semantics)
    RepetitiveGroup,   // 1-byte count prefix, then count × structured group
    RepetitiveGroupFX, // FX-terminated structured groups; FX is the last bit of each group
    Explicit,          // First byte carries length; followed by that many bytes
    SP,                // Special Purpose Field (explicit, SP-marker in UAP)
    Compound,          // PSF-driven optional sub-items, each a Fixed group
};

// ─── Mandatory / Conditional / Optional presence rule ─────────────────────────
enum class Presence { Mandatory, Conditional, Optional };

// ─── A single leaf field inside a Data Item ───────────────────────────────────
struct ElementDef {
    std::string name;          // Field name, e.g. "SAC", "TYP". Empty for spare.
    uint16_t    bits{0};       // Bit width
    Encoding    encoding{Encoding::Raw};
    bool        is_spare{false};

    // Table encoding – raw value → description string
    std::map<uint64_t, std::string> table;

    // Quantity encoding – LSB scale and physical unit
    double      scale{1.0};
    std::string unit;

    // Optional range constraints (informational)
    double      min_val{0.0};
    double      max_val{0.0};
    bool        has_range{false};
};

// ─── One 7-bit data octet inside an Extended item ─────────────────────────────
// The 8th bit (LSB of the raw octet) is always the FX continuation bit and is
// NOT represented here; the codec inserts / checks it automatically.
struct OctetDef {
    std::vector<ElementDef> elements; // bits must sum to exactly 7
};

// ─── One sub-item inside a Compound Data Item ─────────────────────────────────
// Each sub-item occupies one PSF slot and (if not unused) is a Fixed group.
// "-" name means the slot is unused (reserved by the standard).
struct CompoundSubItemDef {
    std::string             name;          // "COM", "PSR", etc.; "-" = unused slot
    std::vector<ElementDef> elements;      // Fields inside this sub-item
    uint16_t                fixed_bytes{0};// Byte length of this sub-item (0 if unused)
};

// ─── Full definition of one Data Item ─────────────────────────────────────────
struct DataItemDef {
    std::string id;    // "010", "020", … "SP"
    std::string name;  // Human-readable title
    ItemType    type{ItemType::Fixed};
    Presence    presence{Presence::Optional};

    // Fixed / Group: flat list of elements (spares included for bit accounting)
    std::vector<ElementDef> elements;

    // Extended: per-octet element lists
    std::vector<OctetDef> octets;

    // Repetitive (FX-based): the single repeated 7-bit element template
    ElementDef rep_element;

    // RepetitiveGroup (count-prefixed): elements for each structured group
    std::vector<ElementDef> rep_group_elements;
    uint16_t rep_group_bits{0};  // total bits per group (sum of element bits)

    // Computed total byte length for Fixed items (filled by SpecLoader)
    uint16_t fixed_bytes{0};

    // Compound: ordered sub-item definitions (one per PSF slot)
    std::vector<CompoundSubItemDef> compound_sub_items;
};

// ─── UAP discriminator (e.g. I001/020 TYP selects plot vs. track) ─────────────
struct UapCase {
    std::string item_id; // "020"
    std::string field;   // "TYP"
    std::map<uint64_t, std::string> value_to_variation; // 0→"plot", 1→"track"
};

// ─── Full Category definition (built from one XML spec file) ──────────────────
struct CategoryDef {
    uint8_t     cat{0};
    std::string name;
    std::string edition;
    std::string date;

    // All items in this category, keyed by ID string
    std::map<std::string, DataItemDef> items;

    // Named UAP variations: variation → ordered item-ID list
    // Sentinel strings in the list:
    //   "-"   = unused FSPEC slot (no item)
    //   "rfs" = Random Field Sequencing (not decoded; reserved)
    std::map<std::string, std::vector<std::string>> uap_variations;

    // Name of the variation to use when no discriminator is present
    std::string default_variation;

    // Optional UAP discriminator (e.g. decode I020/TYP then pick variation)
    std::optional<UapCase> uap_case;
};

// ─── Decoded Data Item value (one per present item in a record) ───────────────
struct DecodedItem {
    std::string item_id;
    ItemType    type{ItemType::Fixed};

    // Named sub-fields (Fixed/Extended).  Spares are excluded.
    // Value is the raw unsigned integer extracted from the wire.
    std::map<std::string, uint64_t> fields;

    // Repetitive (FX-based) items: each entry is the 7-bit raw value.
    std::vector<uint64_t> repetitions;

    // RepetitiveGroup items: each entry holds one group's named field values.
    std::vector<std::map<std::string, uint64_t>> group_repetitions;

    // Explicit / SP: raw payload bytes (length byte itself is NOT included).
    std::vector<uint8_t> raw_bytes;

    // Compound: present sub-items keyed by sub-item name.
    // Each value is a map of { field_name → raw_uint64 } for that sub-item.
    std::map<std::string, std::map<std::string, uint64_t>> compound_sub_fields;
};

// ─── A fully decoded Data Record ──────────────────────────────────────────────
struct DecodedRecord {
    std::map<std::string, DecodedItem> items; // item_id → decoded value
    std::string uap_variation;                // "plot" or "track"
    bool        valid{true};
    std::string error;
};

// ─── A fully decoded Data Block (one per call to Codec::decode) ───────────────
struct DecodedBlock {
    uint8_t  cat{0};
    uint16_t length{0};                   // as read from the wire
    std::vector<DecodedRecord> records;
    bool        valid{true};
    std::string error;
};

} // namespace asterix
