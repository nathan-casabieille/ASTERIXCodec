// test_cat02.cpp – Smoke-tests for CAT02 decode and encode round-trip.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat02

#include "ASTERIXCodec/Codec.hpp"
#include "ASTERIXCodec/SpecLoader.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace asterix;

// ─── Utility ─────────────────────────────────────────────────────────────────

static void hexdump(const std::vector<uint8_t>& v, const std::string& label) {
    std::cout << label << " [" << v.size() << "B]: ";
    for (uint8_t b : v)
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b << ' ';
    std::cout << std::dec << '\n';
}

static int failures = 0;

#define CHECK(cond, msg)                                                  \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::cerr << "FAIL [" << __LINE__ << "] " << (msg) << '\n';  \
            ++failures;                                                    \
        } else {                                                           \
            std::cout << "OK   " << (msg) << '\n';                        \
        }                                                                  \
    } while(0)

// ─────────────────────────────────────────────────────────────────────────────
//  Test 1: XML spec loads without error
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT02 spec load ===\n";
    try {
        CategoryDef cat = loadSpec(spec_path);
        CHECK(cat.cat == 2,           "cat number = 2");
        CHECK(cat.items.count("010"), "item 010 present");
        CHECK(cat.items.count("000"), "item 000 present");
        CHECK(cat.items.count("020"), "item 020 present");
        CHECK(cat.items.count("030"), "item 030 present");
        CHECK(cat.items.count("041"), "item 041 present");
        CHECK(cat.items.count("050"), "item 050 present");
        CHECK(cat.items.count("060"), "item 060 present");
        CHECK(cat.items.count("070"), "item 070 present");
        CHECK(cat.items.count("080"), "item 080 present");
        CHECK(cat.items.count("090"), "item 090 present");
        CHECK(cat.items.count("100"), "item 100 present");
        CHECK(cat.items.count("SP"),  "item SP present");
        CHECK(cat.uap_variations.count("default"), "UAP variation 'default' exists");
        CHECK(!cat.uap_case.has_value(), "no UAP case discriminator (single variation)");

        // Item type assertions
        CHECK(cat.items.at("010").type == ItemType::Fixed,           "010 is Fixed");
        CHECK(cat.items.at("000").type == ItemType::Fixed,           "000 is Fixed");
        CHECK(cat.items.at("050").type == ItemType::Repetitive,      "050 is Repetitive(FX)");
        CHECK(cat.items.at("070").type == ItemType::RepetitiveGroup, "070 is RepetitiveGroup");
        CHECK(cat.items.at("070").rep_group_bits == 16,              "070 group = 16 bits");

        codec.registerCategory(std::move(cat));
    } catch (const std::exception& e) {
        std::cerr << "FAIL spec load: " << e.what() << '\n';
        ++failures;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode a hand-crafted North Marker message
//
//  CAT02 North Marker: I010 + I000 + I030
//  UAP order: 010=slot1, 000=slot2, 020=slot3, 030=slot4
//
//  FSPEC: slot1=1, slot2=1, slot3=0, slot4=1, rest=0, FX=0
//         bit7..bit1: 1 1 0 1 0 0 0 | FX=0 → 0b11010000 = 0xD0
//
//  I002/010: SAC=0x08, SIC=0x0A
//  I002/000: MT=1 (North marker) → 0x01
//  I002/030: TOD = 100s → raw = 100 * 128 = 12800 = 0x003200 (3 bytes big-endian)
//            → 0x00, 0x32, 0x00
//
//  Total: 3(header) + 1(fspec) + 2(I010) + 1(I000) + 3(I030) = 10 bytes
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeNorthMarker(const Codec& codec) {
    std::cout << "\n=== Test: Decode CAT02 North Marker message ===\n";

    std::vector<uint8_t> frame = {
        0x02,        // CAT=2
        0x00, 0x0A,  // LEN=10
        0xD0,        // FSPEC: I010(bit7)=1, I000(bit6)=1, I020(bit5)=0, I030(bit4)=1, FX=0
        0x08, 0x0A,  // I002/010: SAC=8, SIC=10
        0x01,        // I002/000: MT=1 (North marker)
        0x00, 0x32, 0x00, // I002/030: TOD raw=12800 (100.0 s)
    };

    hexdump(frame, "input");
    DecodedBlock block = codec.decode(frame);

    CHECK(block.valid,        "block.valid");
    CHECK(block.cat == 2,     "block.cat == 2");
    CHECK(block.length == 10, "block.length == 10");
    CHECK(block.records.size() == 1, "one record");

    if (block.records.empty()) return;
    const auto& rec = block.records[0];
    CHECK(rec.valid, "record.valid");
    CHECK(rec.uap_variation == "default", "UAP variation = default");
    CHECK(rec.items.count("010"), "I010 present");
    CHECK(rec.items.count("000"), "I000 present");
    CHECK(rec.items.count("030"), "I030 present");
    CHECK(!rec.items.count("020"), "I020 absent");

    if (rec.items.count("010")) {
        CHECK(rec.items.at("010").fields.at("SAC") == 8,  "SAC == 8");
        CHECK(rec.items.at("010").fields.at("SIC") == 10, "SIC == 10");
    }
    if (rec.items.count("000")) {
        CHECK(rec.items.at("000").fields.at("MT") == 1, "MT == 1 (North marker)");
    }
    if (rec.items.count("030")) {
        CHECK(rec.items.at("030").fields.at("TOD") == 12800, "TOD raw == 12800 (100.0 s)");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Encode → decode round-trip for a Sector Crossing message
//          with I010, I000, I020, I030, I041
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripSectorCrossing(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip sector crossing message ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    // I002/010: SAC=5, SIC=7
    {
        DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
        di.fields["SAC"] = 5; di.fields["SIC"] = 7;
        src.items["010"] = std::move(di);
    }
    // I002/000: MT=2 (Sector crossing)
    {
        DecodedItem di; di.item_id = "000"; di.type = ItemType::Fixed;
        di.fields["MT"] = 2;
        src.items["000"] = std::move(di);
    }
    // I002/020: SN raw=64 → 64 * 1.40625 = 90°
    {
        DecodedItem di; di.item_id = "020"; di.type = ItemType::Fixed;
        di.fields["SN"] = 64;
        src.items["020"] = std::move(di);
    }
    // I002/030: TOD raw=6400 → 6400/128 = 50.0 s
    {
        DecodedItem di; di.item_id = "030"; di.type = ItemType::Fixed;
        di.fields["TOD"] = 6400;
        src.items["030"] = std::move(di);
    }
    // I002/041: ARS raw=2560 → 2560/128 = 20.0 s (antenna rotation period)
    {
        DecodedItem di; di.item_id = "041"; di.type = ItemType::Fixed;
        di.fields["ARS"] = 2560;
        src.items["041"] = std::move(di);
    }

    auto encoded = codec.encode(2, {src});
    hexdump(encoded, "encoded");
    CHECK(encoded.size() >= 3, "encoded block non-empty");

    auto block = codec.decode(encoded);
    CHECK(block.valid, "round-trip block valid");
    CHECK(block.cat == 2, "round-trip cat == 2");
    CHECK(block.records.size() == 1, "one record after round-trip");

    if (block.records.empty()) return;
    const auto& rec = block.records[0];
    CHECK(rec.valid, "round-trip record valid");
    CHECK(rec.uap_variation == "default", "round-trip UAP = default");

    if (rec.items.count("010")) {
        CHECK(rec.items.at("010").fields.at("SAC") == 5, "RT SAC == 5");
        CHECK(rec.items.at("010").fields.at("SIC") == 7, "RT SIC == 7");
    } else { std::cerr << "FAIL I010 missing\n"; ++failures; }

    if (rec.items.count("000")) {
        CHECK(rec.items.at("000").fields.at("MT") == 2, "RT MT == 2 (sector crossing)");
    } else { std::cerr << "FAIL I000 missing\n"; ++failures; }

    if (rec.items.count("020")) {
        CHECK(rec.items.at("020").fields.at("SN") == 64, "RT SN == 64");
    } else { std::cerr << "FAIL I020 missing\n"; ++failures; }

    if (rec.items.count("030")) {
        CHECK(rec.items.at("030").fields.at("TOD") == 6400, "RT TOD == 6400");
    } else { std::cerr << "FAIL I030 missing\n"; ++failures; }

    if (rec.items.count("041")) {
        CHECK(rec.items.at("041").fields.at("ARS") == 2560, "RT ARS == 2560");
    } else { std::cerr << "FAIL I041 missing\n"; ++failures; }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Decode FX-based Repetitive item (I002/050 Station Configuration)
//
//  UAP: slot1=010, slot2=000, ..., slot6=050
//  FSPEC with I010, I000, I050 present:
//    slot1=1, slot2=1, slot3=0, slot4=0, slot5=0, slot6=1, slot7=0, FX=0
//    → bits 7..1: 1 1 0 0 0 1 0 | FX=0 → 0b11000100 = 0xC4
//
//  I002/010: SAC=1, SIC=2
//  I002/000: MT=1 (North marker)
//  I002/050: two FX reps: value=10 (FX=1) → byte=(10<<1)|1=0x15
//                         value=20 (FX=0) → byte=(20<<1)|0=0x28
//
//  Total: 3 + 1(fspec) + 2(I010) + 1(I000) + 2(I050) = 9 bytes
// ─────────────────────────────────────────────────────────────────────────────
static void testRepetitiveFx(const Codec& codec) {
    std::cout << "\n=== Test: Decode I002/050 FX-repetitive ===\n";

    std::vector<uint8_t> frame = {
        0x02,        // CAT=2
        0x00, 0x09,  // LEN=9
        0xC4,        // FSPEC: I010(bit7)=1, I000(bit6)=1, I050(bit2)=1, FX=0
        0x01, 0x02,  // I002/010: SAC=1, SIC=2
        0x01,        // I002/000: MT=1
        0x15,        // I002/050 rep1: value=10, FX=1  ((10<<1)|1=0x15)
        0x28,        // I002/050 rep2: value=20, FX=0  ((20<<1)|0=0x28)
    };

    hexdump(frame, "input");
    DecodedBlock block = codec.decode(frame);

    CHECK(block.valid, "block.valid");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "record.valid");
    CHECK(rec.items.count("050"), "I050 present");

    if (rec.items.count("050")) {
        const auto& reps = rec.items.at("050").repetitions;
        CHECK(reps.size() == 2,  "I050 has 2 repetitions");
        if (reps.size() >= 2) {
            CHECK(reps[0] == 10, "I050 rep[0] == 10");
            CHECK(reps[1] == 20, "I050 rep[1] == 20");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Decode count-prefixed RepetitiveGroup item (I002/070 Plot Count)
//
//  Group layout: A(1b) + IDENT(5b) + COUNTER(10b) = 16 bits = 2 bytes each.
//
//  UAP: slot1=010, slot2=000, ..., slot8=070
//  FSPEC: I010 (slot1), I000 (slot2), I070 (slot8) present
//    Byte 1: slot1=1, slot2=1, slots3-7=0, FX=1 → 0b11000001 = 0xC1
//    Byte 2: slot8=1, slots9-14=0, FX=0 → 0b10000000 = 0x80
//
//  I002/010: SAC=1, SIC=2 → 0x01, 0x02
//  I002/000: MT=1 → 0x01
//
//  I002/070: REP=2 (2 groups)
//    Group 1: A=0, IDENT=1, COUNTER=50
//      Binary: 0|00001|0000110010  → 0x04, 0x32
//    Group 2: A=1, IDENT=2, COUNTER=75
//      Binary: 1|00010|0001001011  → 0x88, 0x4B
//
//  Total: 3 + 2(fspec) + 2(I010) + 1(I000) + 5(I070) = 13 bytes
// ─────────────────────────────────────────────────────────────────────────────
static void testPlotCountValues(const Codec& codec) {
    std::cout << "\n=== Test: Decode I002/070 plot count (RepetitiveGroup) ===\n";

    std::vector<uint8_t> frame = {
        0x02,        // CAT=2
        0x00, 0x0D,  // LEN=13
        0xC1,        // FSPEC byte 1: I010(bit7)=1, I000(bit6)=1, FX(bit0)=1
        0x80,        // FSPEC byte 2: I070(bit7)=1, FX(bit0)=0
        0x01, 0x02,  // I002/010: SAC=1, SIC=2
        0x01,        // I002/000: MT=1 (North marker)
        0x02,        // I002/070: REP count = 2
        0x04, 0x32,  // Group 1: A=0, IDENT=1, COUNTER=50
        0x88, 0x4B,  // Group 2: A=1, IDENT=2, COUNTER=75
    };

    hexdump(frame, "input");
    DecodedBlock block = codec.decode(frame);

    CHECK(block.valid, "block.valid");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "record.valid");
    CHECK(rec.items.count("070"), "I070 present");

    if (rec.items.count("070")) {
        const auto& grps = rec.items.at("070").group_repetitions;
        CHECK(grps.size() == 2, "I070 has 2 groups");

        if (grps.size() >= 1) {
            CHECK(grps[0].at("A")       == 0,  "group[0].A == 0 (antenna 1)");
            CHECK(grps[0].at("IDENT")   == 1,  "group[0].IDENT == 1 (sole primary)");
            CHECK(grps[0].at("COUNTER") == 50, "group[0].COUNTER == 50");
        }
        if (grps.size() >= 2) {
            CHECK(grps[1].at("A")       == 1,  "group[1].A == 1 (antenna 2)");
            CHECK(grps[1].at("IDENT")   == 2,  "group[1].IDENT == 2 (sole SSR)");
            CHECK(grps[1].at("COUNTER") == 75, "group[1].COUNTER == 75");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip of I002/070 Plot Count Values (encode then decode)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripPlotCount(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip I002/070 plot count ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    {
        DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
        di.fields["SAC"] = 3; di.fields["SIC"] = 4;
        src.items["010"] = std::move(di);
    }
    {
        DecodedItem di; di.item_id = "000"; di.type = ItemType::Fixed;
        di.fields["MT"] = 1; // North marker
        src.items["000"] = std::move(di);
    }
    {
        DecodedItem di; di.item_id = "070"; di.type = ItemType::RepetitiveGroup;
        di.group_repetitions.push_back({{"A", 0}, {"IDENT", 1}, {"COUNTER", 100}});
        di.group_repetitions.push_back({{"A", 0}, {"IDENT", 2}, {"COUNTER", 42}});
        di.group_repetitions.push_back({{"A", 1}, {"IDENT", 3}, {"COUNTER", 7}});
        src.items["070"] = std::move(di);
    }

    auto encoded = codec.encode(2, {src});
    hexdump(encoded, "encoded");

    auto block = codec.decode(encoded);
    CHECK(block.valid, "RT block valid");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");
    CHECK(rec.items.count("070"), "RT I070 present");

    if (rec.items.count("070")) {
        const auto& grps = rec.items.at("070").group_repetitions;
        CHECK(grps.size() == 3, "RT I070 has 3 groups");
        if (grps.size() >= 3) {
            CHECK(grps[0].at("COUNTER") == 100, "RT group[0].COUNTER == 100");
            CHECK(grps[1].at("IDENT")   == 2,   "RT group[1].IDENT == 2");
            CHECK(grps[1].at("COUNTER") == 42,   "RT group[1].COUNTER == 42");
            CHECK(grps[2].at("A")       == 1,   "RT group[2].A == 1");
            CHECK(grps[2].at("COUNTER") == 7,   "RT group[2].COUNTER == 7");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Collimation Error (I002/090) with signed quantities and Dynamic
//          Window (I002/100) – round-trip
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCollimationAndWindow(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip I002/090 and I002/100 ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    {
        DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
        di.fields["SAC"] = 2; di.fields["SIC"] = 9;
        src.items["010"] = std::move(di);
    }
    {
        DecodedItem di; di.item_id = "000"; di.type = ItemType::Fixed;
        di.fields["MT"] = 8; // Activation of blind zone filtering
        src.items["000"] = std::move(di);
    }
    // I002/090: RE=5 (raw, 5/128 ≈ 0.039 NM), AE=-3 (raw signed, -3/128*360/256... see spec)
    // For signed 8-bit: -3 encodes as 0xFD (two's complement)
    {
        DecodedItem di; di.item_id = "090"; di.type = ItemType::Fixed;
        di.fields["RE"] = 5;
        di.fields["AE"] = static_cast<uint64_t>(static_cast<int8_t>(-3)); // 0xFD
        src.items["090"] = std::move(di);
    }
    // I002/100: RS=1280 (raw, 1280/128=10 NM), RE=2560 (20 NM), TS=8192, TE=16384
    {
        DecodedItem di; di.item_id = "100"; di.type = ItemType::Fixed;
        di.fields["RS"] = 1280;
        di.fields["RE"] = 2560;
        di.fields["TS"] = 8192;
        di.fields["TE"] = 16384;
        src.items["100"] = std::move(di);
    }

    auto encoded = codec.encode(2, {src});
    hexdump(encoded, "encoded");

    auto block = codec.decode(encoded);
    CHECK(block.valid, "RT block valid");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");

    if (rec.items.count("090")) {
        CHECK(rec.items.at("090").fields.at("RE") == 5,                          "RT RE == 5");
        CHECK(rec.items.at("090").fields.at("AE") == (uint64_t)(uint8_t)(-3),    "RT AE == -3 raw");
    } else { std::cerr << "FAIL I090 missing\n"; ++failures; }

    if (rec.items.count("100")) {
        CHECK(rec.items.at("100").fields.at("RS") == 1280,  "RT RS == 1280");
        CHECK(rec.items.at("100").fields.at("RE") == 2560,  "RT RE == 2560");
        CHECK(rec.items.at("100").fields.at("TS") == 8192,  "RT TS == 8192");
        CHECK(rec.items.at("100").fields.at("TE") == 16384, "RT TE == 16384");
    } else { std::cerr << "FAIL I100 missing\n"; ++failures; }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pretty-printer (mirrors test_cat01 style)
// ─────────────────────────────────────────────────────────────────────────────

static std::string fmtElem(const ElementDef& e, uint64_t raw) {
    std::ostringstream s;
    switch (e.encoding) {
    case Encoding::Table: {
        auto it = e.table.find(raw);
        s << raw << " [" << (it != e.table.end() ? it->second : "?") << "]";
        break;
    }
    case Encoding::UnsignedQuantity:
        s << std::fixed << std::setprecision(4) << raw * e.scale << " " << e.unit
          << "  (raw=" << raw << ")";
        break;
    case Encoding::SignedQuantity: {
        int64_t sv = (e.bits < 64 && ((raw >> (e.bits-1)) & 1u))
                     ? static_cast<int64_t>(raw | (~uint64_t{0} << e.bits))
                     : static_cast<int64_t>(raw);
        s << std::fixed << std::setprecision(4) << sv * e.scale << " " << e.unit
          << "  (raw=" << sv << ")";
        break;
    }
    default:
        s << raw << " (0x" << std::hex << raw << std::dec << ")";
    }
    return s.str();
}

static void printBlock02(const DecodedBlock& block, const CategoryDef& cat) {
    std::cout << "  CAT=" << (int)block.cat
              << "  LEN=" << block.length
              << "  records=" << block.records.size() << "\n";

    for (size_t ri = 0; ri < block.records.size(); ++ri) {
        const auto& rec = block.records[ri];
        std::cout << "\n  +-- Record [" << ri << "]  UAP=" << rec.uap_variation;
        if (!rec.valid) std::cout << "  *** ERROR: " << rec.error;
        std::cout << "\n";

        const auto& uap = cat.uap_variations.at(rec.uap_variation);
        for (const auto& id : uap) {
            if (id == "-" || id == "rfs") continue;
            auto it = rec.items.find(id);
            if (it == rec.items.end()) continue;

            const DataItemDef& def  = cat.items.at(id);
            const DecodedItem& item = it->second;
            std::cout << "  |    I002/" << id << " - " << def.name << "\n";

            auto printFields = [&](const std::vector<ElementDef>& elems) {
                for (const auto& e : elems) {
                    if (e.is_spare) continue;
                    auto fit = item.fields.find(e.name);
                    if (fit == item.fields.end()) continue;
                    std::cout << "  |        " << e.name
                              << " = " << fmtElem(e, fit->second) << "\n";
                }
            };

            switch (item.type) {
            case ItemType::Fixed:
                printFields(def.elements);
                break;
            case ItemType::Repetitive:
                for (size_t i = 0; i < item.repetitions.size(); ++i)
                    std::cout << "  |        [" << i << "] = "
                              << item.repetitions[i] << "\n";
                break;
            case ItemType::RepetitiveGroup:
                for (size_t i = 0; i < item.group_repetitions.size(); ++i) {
                    std::cout << "  |        [" << i << "]:";
                    for (const auto& e : def.rep_group_elements) {
                        if (e.is_spare) continue;
                        auto fit = item.group_repetitions[i].find(e.name);
                        if (fit == item.group_repetitions[i].end()) continue;
                        std::cout << "  " << e.name << "=" << fmtElem(e, fit->second);
                    }
                    std::cout << "\n";
                }
                break;
            case ItemType::SP:
                std::cout << "  |        [" << item.raw_bytes.size() << " bytes]\n";
                break;
            default: break;
            }
        }
        std::cout << "  +--\n";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Real CAT02 sector crossing frame
//
//  Frame: 02 00 0C  F4  08 11  02  18  22 05 E1  60
//
//  FSPEC = 0xF4 = 1111_0100 (FX=0):
//    slot1=I010(✓) slot2=I000(✓) slot3=I020(✓) slot4=I030(✓)
//    slot5=I041(✗) slot6=I050(✓) slot7=I060(✗)
//
//  I002/010: SAC=8, SIC=17
//  I002/000: MT=2 (Sector crossing message)
//  I002/020: SN=24 raw → 24 × 1.40625 = 33.75°
//  I002/030: TOD raw=0x2205E1=2229729 → 2229729/128 = 17419.758 s ≈ 04:50:19 UTC
//  I002/050: 0x60 → FX=0, value=(0x60>>1)=48 (1 repetition)
//
//  Total: 3(header) + 1(FSPEC) + 2(I010) + 1(I000) + 1(I020) + 3(I030) + 1(I050) = 12 ✓
// ─────────────────────────────────────────────────────────────────────────────
static void testRealMessage(const Codec& codec) {
    std::cout << "\n=== Test: Real CAT02 sector crossing frame ===\n";

    const std::vector<uint8_t> frame = {
        0x02, 0x00, 0x0C,       // CAT=2, LEN=12
        0xF4,                   // FSPEC: I010 I000 I020 I030 _ I050 _ / FX=0
        0x08, 0x11,             // I002/010: SAC=8, SIC=17
        0x02,                   // I002/000: MT=2 (sector crossing)
        0x18,                   // I002/020: SN=24 (33.75°)
        0x22, 0x05, 0xE1,       // I002/030: TOD raw=2229729 (≈04:50:19 UTC)
        0x60,                   // I002/050: value=48, FX=0 (1 repetition)
    };

    hexdump(frame, "input");
    DecodedBlock block = codec.decode(frame);

    CHECK(block.valid,        "real: block.valid");
    CHECK(block.cat == 2,     "real: cat == 2");
    CHECK(block.length == 12, "real: length == 12");
    CHECK(block.records.size() == 1, "real: 1 record");

    if (block.records.empty()) return;
    const auto& rec = block.records[0];
    CHECK(rec.valid,                      "real: record.valid");
    CHECK(rec.uap_variation == "default", "real: UAP = default");
    CHECK(!rec.items.count("041"),        "real: I041 absent");
    CHECK(!rec.items.count("060"),        "real: I060 absent");

    if (rec.items.count("010")) {
        CHECK(rec.items.at("010").fields.at("SAC") == 8,  "real: SAC == 8");
        CHECK(rec.items.at("010").fields.at("SIC") == 17, "real: SIC == 17");
    } else { std::cerr << "FAIL real: I010 missing\n"; ++failures; }

    if (rec.items.count("000")) {
        CHECK(rec.items.at("000").fields.at("MT") == 2, "real: MT == 2 (sector crossing)");
    } else { std::cerr << "FAIL real: I000 missing\n"; ++failures; }

    if (rec.items.count("020")) {
        CHECK(rec.items.at("020").fields.at("SN") == 24, "real: SN == 24 (33.75°)");
    } else { std::cerr << "FAIL real: I020 missing\n"; ++failures; }

    if (rec.items.count("030")) {
        CHECK(rec.items.at("030").fields.at("TOD") == 2229729, "real: TOD raw == 2229729");
    } else { std::cerr << "FAIL real: I030 missing\n"; ++failures; }

    if (rec.items.count("050")) {
        const auto& reps = rec.items.at("050").repetitions;
        CHECK(reps.size() == 1, "real: I050 has 1 rep");
        if (!reps.empty())
            CHECK(reps[0] == 48, "real: I050 rep[0] == 48");
    } else { std::cerr << "FAIL real: I050 missing\n"; ++failures; }

    printBlock02(block, codec.category(2));
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = (argc > 1)
        ? fs::path(argv[1])
        : fs::path(__FILE__).parent_path().parent_path() / "specs" / "CAT02.xml";

    std::cout << "Using spec: " << spec_path << '\n';

    Codec codec;
    testSpecLoad(codec, spec_path);

    if (failures == 0) {
        testDecodeNorthMarker(codec);
        testRoundTripSectorCrossing(codec);
        testRepetitiveFx(codec);
        testPlotCountValues(codec);
        testRoundTripPlotCount(codec);
        testRoundTripCollimationAndWindow(codec);
        testRealMessage(codec);
    }

    std::cout << "\n──────────────────────────────────\n";
    if (failures == 0) {
        std::cout << "ALL TESTS PASSED\n";
    } else {
        std::cout << failures << " TEST(S) FAILED\n";
    }
    return failures == 0 ? 0 : 1;
}
