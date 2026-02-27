// Codec.cpp – ASTERIX decode/encode engine for registered categories.
//
// Wire-format reminder:
//   Data Block  = [CAT 1B][LEN 2B][Record…]
//   Data Record = [FSPEC bytes][Item bytes…]
//   FSPEC byte  = [I_n … I_n-6 | FX]
//                  MSB = first UAP slot; LSB = continuation flag
//
// All multi-byte integers on the wire are big-endian.

#include "ASTERIXCodec/Codec.hpp"
#include "ASTERIXCodec/BitStream.hpp"

#include <stdexcept>
#include <string>

namespace asterix {

// ─────────────────────────────────────────────────────────────────────────────
//  Category registry
// ─────────────────────────────────────────────────────────────────────────────

void Codec::registerCategory(CategoryDef cat) {
    uint8_t key = cat.cat;
    cats_[key]  = std::move(cat);
}

const CategoryDef& Codec::category(uint8_t cat) const {
    auto it = cats_.find(cat);
    if (it == cats_.end())
        throw std::runtime_error("Category " + std::to_string(cat) + " not registered");
    return it->second;
}

// ─────────────────────────────────────────────────────────────────────────────
//  UAP selection
// ─────────────────────────────────────────────────────────────────────────────

std::string Codec::resolveVariation(const CategoryDef& cat,
                                     const DecodedRecord& partial) const {
    if (!cat.uap_case.has_value())
        return cat.default_variation;

    const UapCase& uc = *cat.uap_case;
    auto rec_it = partial.items.find(uc.item_id);
    if (rec_it == partial.items.end())
        return cat.default_variation;

    auto field_it = rec_it->second.fields.find(uc.field);
    if (field_it == rec_it->second.fields.end())
        return cat.default_variation;

    auto var_it = uc.value_to_variation.find(field_it->second);
    if (var_it == uc.value_to_variation.end())
        return cat.default_variation;

    return var_it->second;
}

const std::vector<std::string>&
Codec::selectUap(const CategoryDef& cat, const DecodedRecord& partial) const {
    std::string var = resolveVariation(cat, partial);
    auto it = cat.uap_variations.find(var);
    if (it == cat.uap_variations.end()) {
        // fallback to the first registered variation
        it = cat.uap_variations.begin();
    }
    return it->second;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Item-level decode helpers
// ─────────────────────────────────────────────────────────────────────────────

// Decode the sub-elements of a Fixed item from its bit-reader.
static void decodeFixedElements(const std::vector<ElementDef>& elems,
                                 BitReader& br,
                                 DecodedItem& out) {
    for (const auto& e : elems) {
        if (e.is_spare) {
            br.skip(e.bits);
            continue;
        }
        uint64_t raw = br.readU(e.bits);
        out.fields[e.name] = raw;
    }
}

// Decode the sub-elements of one Extended octet from its bit-reader.
static void decodeOctetElements(const OctetDef& oct, BitReader& br, DecodedItem& out) {
    for (const auto& e : oct.elements) {
        if (e.is_spare) { br.skip(e.bits); continue; }
        out.fields[e.name] = br.readU(e.bits);
    }
    // FX bit: consume it but don't store it – the caller already checked it
    br.skip(1); // FX
}

DecodedItem Codec::decodeItem(const DataItemDef& def,
                               std::span<const uint8_t> item_buf,
                               size_t& consumed) const {
    DecodedItem out;
    out.item_id = def.id;
    out.type    = def.type;

    switch (def.type) {

    // ── Fixed ─────────────────────────────────────────────────────────────
    case ItemType::Fixed: {
        if (item_buf.size() < def.fixed_bytes)
            throw std::runtime_error("Item " + def.id + ": buffer too short for Fixed");
        BitReader br{item_buf.subspan(0, def.fixed_bytes)};
        decodeFixedElements(def.elements, br, out);
        consumed = def.fixed_bytes;
        break;
    }

    // ── Extended ──────────────────────────────────────────────────────────
    case ItemType::Extended: {
        // Read octets until FX=0.  Each raw octet = 7 data bits + 1 FX bit.
        size_t offset = 0;
        for (size_t oct_idx = 0; ; ++oct_idx) {
            if (offset >= item_buf.size())
                throw std::runtime_error("Item " + def.id + ": unexpected end of buffer in Extended");
            uint8_t raw_byte = item_buf[offset];
            bool    fx       = (raw_byte & 0x01u) != 0;
            ++offset;

            if (oct_idx < def.octets.size()) {
                // Wrap this single byte in a reader (8 bits)
                BitReader br{item_buf.subspan(offset - 1, 1)};
                decodeOctetElements(def.octets[oct_idx], br, out);
            } else {
                // Octet beyond spec definition – skip it but honour FX
                (void)raw_byte;
            }
            if (!fx) break;
        }
        consumed = offset;
        break;
    }

    // ── Repetitive FX ─────────────────────────────────────────────────────
    case ItemType::Repetitive: {
        size_t offset = 0;
        do {
            if (offset >= item_buf.size())
                throw std::runtime_error("Item " + def.id + ": buffer too short in Repetitive");
            uint8_t raw_byte = item_buf[offset++];
            bool    fx       = (raw_byte & 0x01u) != 0;
            uint64_t val     = (raw_byte >> 1) & 0x7Fu;   // top 7 bits
            out.repetitions.push_back(val);
            if (!fx) break;
        } while (true);
        consumed = offset;
        break;
    }

    // ── Repetitive count-prefixed (structured group) ───────────────────────
    case ItemType::RepetitiveGroup: {
        if (item_buf.empty())
            throw std::runtime_error("Item " + def.id + ": buffer too short for RepetitiveGroup");
        uint8_t rep_count  = item_buf[0];
        size_t  group_bytes = static_cast<size_t>(def.rep_group_bits) / 8;
        size_t  total_need  = 1 + static_cast<size_t>(rep_count) * group_bytes;
        if (item_buf.size() < total_need)
            throw std::runtime_error("Item " + def.id + ": buffer too short for RepetitiveGroup data");

        size_t offset = 1;
        for (uint8_t i = 0; i < rep_count; ++i) {
            std::map<std::string, uint64_t> grp;
            BitReader br{item_buf.subspan(offset, group_bytes)};
            for (const auto& e : def.rep_group_elements) {
                if (e.is_spare) { br.skip(e.bits); continue; }
                grp[e.name] = br.readU(e.bits);
            }
            out.group_repetitions.push_back(std::move(grp));
            offset += group_bytes;
        }
        consumed = total_need;
        break;
    }

    // ── Repetitive FX with structured group ───────────────────────────────
    // Each group is (rep_group_bits + 1) / 8 bytes wide.
    // The last bit of each group is the FX flag (1 = more groups follow).
    case ItemType::RepetitiveGroupFX: {
        size_t group_bytes = (static_cast<size_t>(def.rep_group_bits) + 1) / 8;
        size_t offset = 0;
        do {
            if (offset + group_bytes > item_buf.size())
                throw std::runtime_error("Item " + def.id +
                                         ": buffer too short in RepetitiveGroupFX");
            BitReader br{item_buf.subspan(offset, group_bytes)};
            std::map<std::string, uint64_t> grp;
            for (const auto& e : def.rep_group_elements) {
                if (e.is_spare) { br.skip(e.bits); continue; }
                grp[e.name] = br.readU(e.bits);
            }
            bool fx = br.readBit(); // FX is the last bit of the group
            out.group_repetitions.push_back(std::move(grp));
            offset += group_bytes;
            if (!fx) break;
        } while (true);
        consumed = offset;
        break;
    }

    // ── Explicit / SP ─────────────────────────────────────────────────────
    case ItemType::SP: {
        if (item_buf.empty())
            throw std::runtime_error("Item " + def.id + ": empty buffer for Explicit");
        uint8_t len = item_buf[0]; // first byte = payload length (including itself per ASTERIX spec)
        // len field includes itself: payload = len-1 bytes
        if (len < 1 || item_buf.size() < static_cast<size_t>(len))
            throw std::runtime_error("Item " + def.id + ": Explicit length out of range");
        out.raw_bytes.assign(item_buf.begin() + 1, item_buf.begin() + len);
        consumed = len;
        break;
    }

    // ── Compound ──────────────────────────────────────────────────────────
    // Wire format: PSF byte(s) [same FX-extension as outer FSPEC] followed by
    // the fixed-size payload of each sub-item whose PSF slot bit is set.
    // PSF bit mapping: bit 7 = sub-item 0, bit 6 = sub-item 1, … bit 1 = sub-item 6.
    // bit 0 of each PSF byte is the FX continuation flag.
    case ItemType::Compound: {
        // Read PSF byte(s)
        size_t offset = 0;
        std::vector<uint8_t> psf;
        do {
            if (offset >= item_buf.size())
                throw std::runtime_error("Item " + def.id + ": truncated Compound PSF");
            uint8_t b = item_buf[offset++];
            psf.push_back(b);
        } while (psf.back() & 0x01u); // FX=1 means more PSF bytes follow

        // Decode each sub-item whose PSF slot is set
        for (size_t slot = 0; slot < def.compound_sub_items.size(); ++slot) {
            const auto& si = def.compound_sub_items[slot];
            size_t psf_byte = slot / 7;
            size_t psf_bit  = 7 - (slot % 7); // bit 7 = slot 0, bit 1 = slot 6
            bool   present  = (psf_byte < psf.size()) &&
                              ((psf[psf_byte] >> psf_bit) & 0x01u);
            if (!present || si.name == "-") continue;

            if (offset + si.fixed_bytes > item_buf.size())
                throw std::runtime_error("Item " + def.id + "/" + si.name +
                                         ": buffer too short for Compound sub-item");
            std::map<std::string, uint64_t> sub_fields;
            BitReader br{item_buf.subspan(offset, si.fixed_bytes)};
            for (const auto& e : si.elements) {
                if (e.is_spare) { br.skip(e.bits); continue; }
                sub_fields[e.name] = br.readU(e.bits);
            }
            out.compound_sub_fields[si.name] = std::move(sub_fields);
            offset += si.fixed_bytes;
        }
        consumed = offset;
        break;
    }

    default:
        throw std::runtime_error("Item " + def.id + ": unsupported item type");
    }

    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Record-level decode
// ─────────────────────────────────────────────────────────────────────────────

// CAT01 has two UAPs sharing the same first two slots (I010, I020).
// Strategy:
//   1. Read FSPEC bytes.
//   2. Decode using default variation.
//   3. If I020 was decoded and a Case discriminator exists, re-run with the
//      correct variation (FSPEC is already known, only item order differs).
//
// Because I010 and I020 appear at FSPEC positions 1 and 2 in BOTH variations
// of CAT01, we need only one pass.  The resolved variation is stored in the
// record for the caller's use.

DecodedRecord Codec::decodeRecord(std::span<const uint8_t> buf,
                                   const CategoryDef& cat,
                                   size_t& bytes_consumed) const {
    DecodedRecord rec;
    size_t pos = 0;

    if (buf.empty()) {
        bytes_consumed = 0;
        rec.valid = false;
        rec.error = "decodeRecord called on empty buffer";
        return rec;
    }

    // ── Step 1: Read FSPEC ──────────────────────────────────────────────────
    std::vector<uint8_t> fspec;
    while (pos < buf.size()) {
        uint8_t b = buf[pos++];
        fspec.push_back(b);
        if ((b & 0x01u) == 0) break; // FX=0 → last FSPEC byte
    }
    if (fspec.empty())
        throw std::runtime_error("Record has empty FSPEC");

    // ── Step 2: Collect FSPEC presence bits (MSB→bit7 = UAP slot 1) ────────
    // UAP slot index (1-based) → bit position in FSPEC.
    // Each FSPEC byte contributes 7 slots (bits 7..1); bit 0 is FX.
    // Slot k (1-based) → fspec byte [(k-1)/7], bit (7 - ((k-1)%7)).

    auto isPresent = [&](size_t slot_1based) -> bool {
        if (slot_1based == 0) return false;
        size_t idx       = (slot_1based - 1) / 7;  // which fspec byte
        size_t bit_shift = 7 - ((slot_1based - 1) % 7); // bit within byte (7=MSB…1=next to FX)
        if (idx >= fspec.size()) return false;
        return ((fspec[idx] >> bit_shift) & 0x01u) != 0;
    };

    // ── Step 3: First pass – determine UAP variation ─────────────────────────
    // Use the default variation initially; after decoding the discriminator
    // item (e.g. I020) we can confirm / switch variation for FSPEC interpretation.
    //
    // For CAT01: slots 1 & 2 are I010 & I020 in BOTH variations, so a single
    // pass with default_variation is correct (no re-interpretation needed).

    const std::vector<std::string>* uap = &cat.uap_variations.at(cat.default_variation);

    // ── Step 4: Decode items in UAP order ────────────────────────────────────
    for (size_t slot = 1; slot <= uap->size(); ++slot) {
        const std::string& item_id = (*uap)[slot - 1];

        if (item_id == "-" || item_id == "rfs") continue;

        if (!isPresent(slot)) continue;

        auto def_it = cat.items.find(item_id);
        if (def_it == cat.items.end())
            throw std::runtime_error("FSPEC references unknown item: " + item_id);

        const DataItemDef& def = def_it->second;
        size_t item_consumed   = 0;
        DecodedItem di = decodeItem(def, buf.subspan(pos), item_consumed);
        pos += item_consumed;
        rec.items[item_id] = std::move(di);

        // After decoding the discriminator item, switch UAP if necessary.
        // (Re-checking the same FSPEC with the new UAP pointer is safe because
        // in CAT01 the first two slots are identical in both variations.)
        if (cat.uap_case.has_value() && item_id == cat.uap_case->item_id) {
            std::string var = resolveVariation(cat, rec);
            auto it = cat.uap_variations.find(var);
            if (it != cat.uap_variations.end()) {
                uap = &it->second;
                rec.uap_variation = var;
            }
        }
    }

    if (rec.uap_variation.empty())
        rec.uap_variation = cat.default_variation;

    // ── Step 5: Mandatory item validation ────────────────────────────────────
    for (const auto& [id, def] : cat.items) {
        if (def.presence == Presence::Mandatory &&
            rec.items.find(id) == rec.items.end()) {
            rec.valid = false;
            rec.error = "Mandatory item " + id + " not present";
        }
    }

    bytes_consumed = pos;
    return rec;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Public decode
// ─────────────────────────────────────────────────────────────────────────────

DecodedBlock Codec::decode(std::span<const uint8_t> buf) const {
    DecodedBlock block;

    if (buf.size() < 3) {
        block.valid = false;
        block.error = "Buffer too short for Data Block header (need ≥3 bytes)";
        return block;
    }

    block.cat    = buf[0];
    block.length = static_cast<uint16_t>((buf[1] << 8) | buf[2]);

    if (block.length < 3 || static_cast<size_t>(block.length) > buf.size()) {
        block.valid = false;
        block.error = "Data Block LEN field (" + std::to_string(block.length) + ") is invalid";
        return block;
    }

    auto cat_it = cats_.find(block.cat);
    if (cat_it == cats_.end()) {
        block.valid = false;
        block.error = "Category " + std::to_string(block.cat) + " not registered";
        return block;
    }
    const CategoryDef& cat = cat_it->second;

    // Payload: everything after the 3-byte header
    auto payload = buf.subspan(3, block.length - 3);
    size_t pos   = 0;

    while (pos < payload.size()) {
        size_t consumed = 0;
        try {
            DecodedRecord rec = decodeRecord(payload.subspan(pos), cat, consumed);
            block.records.push_back(std::move(rec));
        } catch (const std::exception& ex) {
            block.valid = false;
            block.error = std::string("Record decode error: ") + ex.what();
            break;
        }
        if (consumed == 0) {
            block.valid = false;
            block.error = "Infinite loop guard: no bytes consumed decoding record";
            break;
        }
        pos += consumed;
    }

    return block;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Item-level encode helpers
// ─────────────────────────────────────────────────────────────────────────────

static void encodeFixedElements(const std::vector<ElementDef>& elems,
                                 const DecodedItem& src,
                                 BitWriter& bw) {
    for (const auto& e : elems) {
        if (e.is_spare) {
            bw.writeU(0, e.bits);
            continue;
        }
        uint64_t val = 0;
        auto it = src.fields.find(e.name);
        if (it != src.fields.end()) val = it->second;
        bw.writeU(val, e.bits);
    }
}

std::vector<uint8_t> Codec::encodeItem(const DataItemDef& def,
                                         const DecodedItem& val) const {
    BitWriter bw;

    switch (def.type) {

    case ItemType::Fixed: {
        encodeFixedElements(def.elements, val, bw);
        break;
    }

    case ItemType::Extended: {
        // Write each octet that has non-zero data OR that is required.
        // Determine how many octets carry meaningful data.
        size_t last_useful = 0;
        for (size_t i = 0; i < def.octets.size(); ++i) {
            for (const auto& e : def.octets[i].elements) {
                if (!e.is_spare) {
                    auto it = val.fields.find(e.name);
                    if (it != val.fields.end() && it->second != 0)
                        last_useful = i + 1;
                }
            }
        }
        if (last_useful == 0) last_useful = 1; // always write at least 1 octet

        for (size_t i = 0; i < last_useful; ++i) {
            bool is_last = (i + 1 == last_useful);
            const OctetDef& oct = def.octets[i];
            for (const auto& e : oct.elements) {
                if (e.is_spare) { bw.writeU(0, e.bits); continue; }
                uint64_t v = 0;
                auto it = val.fields.find(e.name);
                if (it != val.fields.end()) v = it->second;
                bw.writeU(v, e.bits);
            }
            bw.writeBit(!is_last); // FX
        }
        break;
    }

    case ItemType::Repetitive: {
        const auto& reps = val.repetitions;
        for (size_t i = 0; i < reps.size(); ++i) {
            bool is_last = (i + 1 == reps.size());
            bw.writeU(reps[i] & 0x7Fu, 7); // 7 data bits
            bw.writeBit(!is_last);           // FX
        }
        if (reps.empty()) {
            bw.writeU(0, 7);
            bw.writeBit(false);
        }
        break;
    }

    case ItemType::RepetitiveGroup: {
        const auto& grps = val.group_repetitions;
        bw.writeByte(static_cast<uint8_t>(grps.size())); // 1-byte count prefix
        for (const auto& grp : grps) {
            for (const auto& e : def.rep_group_elements) {
                if (e.is_spare) { bw.writeU(0, e.bits); continue; }
                uint64_t v = 0;
                auto it = grp.find(e.name);
                if (it != grp.end()) v = it->second;
                bw.writeU(v, e.bits);
            }
        }
        break;
    }

    case ItemType::RepetitiveGroupFX: {
        const auto& grps = val.group_repetitions;
        if (grps.empty()) {
            // Emit one zero-filled group with FX=0
            for (const auto& e : def.rep_group_elements) {
                if (e.is_spare) { bw.writeU(0, e.bits); continue; }
                bw.writeU(0, e.bits);
            }
            bw.writeBit(false); // FX=0
        } else {
            for (size_t i = 0; i < grps.size(); ++i) {
                bool is_last = (i + 1 == grps.size());
                for (const auto& e : def.rep_group_elements) {
                    if (e.is_spare) { bw.writeU(0, e.bits); continue; }
                    uint64_t v = 0;
                    auto it = grps[i].find(e.name);
                    if (it != grps[i].end()) v = it->second;
                    bw.writeU(v, e.bits);
                }
                bw.writeBit(!is_last); // FX=1 → more groups, FX=0 → last
            }
        }
        break;
    }

    case ItemType::SP: {
        // length byte (includes itself) + payload
        uint8_t len = static_cast<uint8_t>(val.raw_bytes.size() + 1);
        bw.writeByte(len);
        bw.writeBytes(val.raw_bytes);
        break;
    }

    case ItemType::Compound: {
        const auto& subs = def.compound_sub_items;

        // Find the highest-indexed slot that has a present sub-item
        int last_slot = -1;
        for (size_t i = 0; i < subs.size(); ++i) {
            if (subs[i].name != "-" && val.compound_sub_fields.count(subs[i].name))
                last_slot = static_cast<int>(i);
        }

        // Always emit at least one PSF byte (even if nothing is present)
        size_t last_psf_byte = (last_slot >= 0) ? (static_cast<size_t>(last_slot) / 7) : 0;
        std::vector<uint8_t> psf_bytes(last_psf_byte + 1, 0);

        for (size_t i = 0; i < subs.size(); ++i) {
            if (subs[i].name == "-") continue;
            if (!val.compound_sub_fields.count(subs[i].name)) continue;
            size_t pb  = i / 7;
            size_t bit = 7 - (i % 7);
            psf_bytes[pb] |= static_cast<uint8_t>(1u << bit);
        }
        // FX bit for all PSF bytes except the last
        for (size_t i = 0; i + 1 < psf_bytes.size(); ++i)
            psf_bytes[i] |= 0x01u;

        bw.writeBytes(psf_bytes);

        // Write each present sub-item's fields in PSF slot order
        for (const auto& si : subs) {
            if (si.name == "-") continue;
            auto it = val.compound_sub_fields.find(si.name);
            if (it == val.compound_sub_fields.end()) continue;
            const auto& sub_fields = it->second;
            for (const auto& e : si.elements) {
                if (e.is_spare) { bw.writeU(0, e.bits); continue; }
                uint64_t v = 0;
                auto fit = sub_fields.find(e.name);
                if (fit != sub_fields.end()) v = fit->second;
                bw.writeU(v, e.bits);
            }
        }
        break;
    }

    default:
        throw std::runtime_error("encodeItem: unsupported item type for " + def.id);
    }

    return bw.take();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Record-level encode
// ─────────────────────────────────────────────────────────────────────────────

std::vector<uint8_t> Codec::encodeRecord(const DecodedRecord& rec,
                                           const CategoryDef& cat) const {
    // Select UAP
    std::string var = rec.uap_variation.empty() ? cat.default_variation : rec.uap_variation;
    auto uap_it = cat.uap_variations.find(var);
    if (uap_it == cat.uap_variations.end())
        throw std::runtime_error("encodeRecord: unknown UAP variation '" + var + "'");
    const auto& uap = uap_it->second;

    // ── Build FSPEC ──────────────────────────────────────────────────────────
    // Determine which UAP slots are present.
    std::vector<bool> present(uap.size(), false);
    for (size_t i = 0; i < uap.size(); ++i) {
        const std::string& id = uap[i];
        if (id == "-" || id == "rfs") continue;
        if (rec.items.count(id)) present[i] = true;
    }

    // Build FSPEC bytes: groups of 7 bits, each followed by FX bit.
    // Trim trailing empty octets (no need to send FSPEC bytes where all slots=0).
    size_t total_slots = uap.size();
    size_t n_fspec_octets = (total_slots + 6) / 7;
    // Find the last octet with at least one present item
    size_t last_fspec = 0;
    for (size_t i = 0; i < n_fspec_octets; ++i) {
        for (size_t s = i * 7; s < (i + 1) * 7 && s < total_slots; ++s) {
            if (present[s]) last_fspec = i;
        }
    }

    std::vector<uint8_t> fspec_bytes;
    for (size_t i = 0; i <= last_fspec; ++i) {
        uint8_t b = 0;
        for (size_t s = i * 7; s < (i + 1) * 7 && s < total_slots; ++s) {
            size_t bit_pos = 7 - (s % 7); // bit 7 = slot 1, bit 6 = slot 2, …
            if (present[s]) b |= (1u << bit_pos);
        }
        bool is_last = (i == last_fspec);
        if (!is_last) b |= 0x01u; // FX = 1 (more FSPEC bytes follow)
        fspec_bytes.push_back(b);
    }

    // ── Encode items in UAP order ────────────────────────────────────────────
    std::vector<uint8_t> payload;
    for (const auto& id : uap) {
        if (id == "-" || id == "rfs") continue;
        auto it = rec.items.find(id);
        if (it == rec.items.end()) continue;

        auto def_it = cat.items.find(id);
        if (def_it == cat.items.end())
            throw std::runtime_error("encodeRecord: item def not found for " + id);

        auto item_bytes = encodeItem(def_it->second, it->second);
        payload.insert(payload.end(), item_bytes.begin(), item_bytes.end());
    }

    // Concatenate FSPEC + items
    std::vector<uint8_t> record_bytes;
    record_bytes.insert(record_bytes.end(), fspec_bytes.begin(), fspec_bytes.end());
    record_bytes.insert(record_bytes.end(), payload.begin(), payload.end());
    return record_bytes;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Public encode
// ─────────────────────────────────────────────────────────────────────────────

std::vector<uint8_t> Codec::encode(uint8_t cat_num,
                                    const std::vector<DecodedRecord>& records) const {
    auto cat_it = cats_.find(cat_num);
    if (cat_it == cats_.end())
        throw std::runtime_error("encode: Category " + std::to_string(cat_num) + " not registered");
    const CategoryDef& cat = cat_it->second;

    // Encode all records
    std::vector<uint8_t> records_bytes;
    for (const auto& rec : records) {
        auto rb = encodeRecord(rec, cat);
        records_bytes.insert(records_bytes.end(), rb.begin(), rb.end());
    }

    // Compute total block length = 3 (header) + records
    uint16_t total_len = static_cast<uint16_t>(3 + records_bytes.size());

    std::vector<uint8_t> block;
    block.reserve(total_len);
    block.push_back(cat_num);
    block.push_back(static_cast<uint8_t>(total_len >> 8));
    block.push_back(static_cast<uint8_t>(total_len & 0xFFu));
    block.insert(block.end(), records_bytes.begin(), records_bytes.end());
    return block;
}

} // namespace asterix
