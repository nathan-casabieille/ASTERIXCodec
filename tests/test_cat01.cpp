// test_cat01.cpp – Smoke-tests for CAT01 decode and encode round-trip.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat01

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
    std::cout << "\n=== Test: Spec load ===\n";
    try {
        CategoryDef cat = loadSpec(spec_path);
        CHECK(cat.cat == 1,     "cat number = 1");
        CHECK(cat.items.count("010"), "item 010 present");
        CHECK(cat.items.count("020"), "item 020 present");
        CHECK(cat.items.count("040"), "item 040 present");
        CHECK(cat.items.count("SP"),  "item SP present");
        CHECK(cat.uap_variations.count("plot"),  "UAP variation 'plot' exists");
        CHECK(cat.uap_variations.count("track"), "UAP variation 'track' exists");
        CHECK(cat.uap_case.has_value(), "UAP case discriminator loaded");
        codec.registerCategory(std::move(cat));
    } catch (const std::exception& e) {
        std::cerr << "FAIL spec load: " << e.what() << '\n';
        ++failures;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode a hand-crafted CAT01 "plot" Data Block
//
//  Block:
//    CAT = 0x01
//    LEN = 0x00 0x0C  (12 bytes total)
//
//  Record (FSPEC + items):
//    FSPEC byte 1: 1100_0000 | 0 (FX=0, last)
//                  ↑ I010 present, I020 present, rest absent
//    Byte = 0xC0
//
//    I001/010 (2 bytes): SAC=0x05  SIC=0x12
//    I001/020 (1 byte ext): TYP=0, SIM=0, SSRPSR=1(sole primary), ANT=0, SPI=0, RAB=0, FX=0
//                            = [0 0 01 0 0 0 | FX=0] = 0000_1000 | 0 = 0x08
//
//  Total: 3 (header) + 1 (fspec) + 2 (I010) + 1 (I020) = 7 bytes
//  But LEN must equal actual length.  Let's compute carefully.
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodePlot(const Codec& codec) {
    std::cout << "\n=== Test: Decode CAT01 plot record ===\n";

    // I001/020 first octet:
    // TYP=0(1b) SIM=0(1b) SSRPSR=01(2b) ANT=0(1b) SPI=0(1b) RAB=0(1b) FX=0
    // = 0 0 0 1 0 0 0 | 0 → 0b00010000 = 0x10
    // Wait: bits ordered MSB first within byte:
    //   bit7=TYP(0), bit6=SIM(0), bits5-4=SSRPSR(01), bit3=ANT(0), bit2=SPI(0), bit1=RAB(0), bit0=FX(0)
    //   = 0 0 01 0 0 0 0 = 0x10

    std::vector<uint8_t> frame = {
        0x01,         // CAT
        0x00, 0x09,   // LEN = 9
        // FSPEC: I010(bit7)=1, I020(bit6)=1, rest=0, FX=0 → 0b11000000 = 0xC0
        0xC0,
        // I001/010: SAC=5, SIC=18
        0x05, 0x12,
        // I001/020: TYP=0(plot), SSRPSR=1, rest=0, FX=0
        0x10,
        // padding to reach LEN=9
        // 3 (header) + 1 (fspec) + 2 (I010) + 1 (I020) = 7  → LEN=7
    };
    // Recompute: 3+1+2+1 = 7
    frame[1] = 0x00;
    frame[2] = 0x07;
    frame.resize(7);

    hexdump(frame, "input");
    DecodedBlock block = codec.decode(frame);

    CHECK(block.valid,         "block.valid");
    CHECK(block.cat == 1,      "block.cat == 1");
    CHECK(block.length == 7,   "block.length == 7");
    CHECK(block.records.size() == 1, "one record");

    if (block.records.empty()) return;
    const auto& rec = block.records[0];
    CHECK(rec.valid, "record.valid");
    CHECK(rec.uap_variation == "plot", "UAP variation = plot");
    CHECK(rec.items.count("010"), "I010 present");
    CHECK(rec.items.count("020"), "I020 present");

    if (rec.items.count("010")) {
        CHECK(rec.items.at("010").fields.at("SAC") == 5,  "SAC == 5");
        CHECK(rec.items.at("010").fields.at("SIC") == 18, "SIC == 18");
    }
    if (rec.items.count("020")) {
        CHECK(rec.items.at("020").fields.at("TYP")    == 0, "TYP == 0 (plot)");
        CHECK(rec.items.at("020").fields.at("SSRPSR") == 1, "SSRPSR == 1 (primary)");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Encode → decode round-trip for a "track" record
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripTrack(const Codec& codec) {
    std::cout << "\n=== Test: Encode/decode round-trip (track) ===\n";

    // Build a DecodedRecord with a few items (track variation)
    DecodedRecord src;
    src.uap_variation = "track";

    // I001/010 – SAC=1, SIC=2
    {
        DecodedItem di;
        di.item_id = "010"; di.type = ItemType::Fixed;
        di.fields["SAC"] = 1; di.fields["SIC"] = 2;
        src.items["010"] = std::move(di);
    }
    // I001/020 – TYP=1 (track), SSRPSR=3 (combined)
    {
        DecodedItem di;
        di.item_id = "020"; di.type = ItemType::Extended;
        di.fields["TYP"]    = 1;
        di.fields["SIM"]    = 0;
        di.fields["SSRPSR"] = 3;
        di.fields["ANT"]    = 0;
        di.fields["SPI"]    = 0;
        di.fields["RAB"]    = 0;
        src.items["020"] = std::move(di);
    }
    // I001/161 – Track Plot Number = 42
    {
        DecodedItem di;
        di.item_id = "161"; di.type = ItemType::Fixed;
        di.fields["TRKNO"] = 42;
        src.items["161"] = std::move(di);
    }
    // I001/040 – RHO=100 NM (raw=12800), THETA=90° (raw=16384)
    {
        DecodedItem di;
        di.item_id = "040"; di.type = ItemType::Fixed;
        di.fields["RHO"]   = 12800;   // 12800 / 128 = 100 NM
        di.fields["THETA"] = 16384;   // 16384 * (360/65536) ≈ 90°
        src.items["040"] = std::move(di);
    }
    // I001/170 – CON=1, RAD=1, GHO=0, FX=0 (one octet)
    {
        DecodedItem di;
        di.item_id = "170"; di.type = ItemType::Extended;
        di.fields["CON"]  = 1;
        di.fields["RAD"]  = 1;
        di.fields["MAN"]  = 0;
        di.fields["DOU"]  = 0;
        di.fields["RDPC"] = 0;
        di.fields["GHO"]  = 0;
        src.items["170"] = std::move(di);
    }

    // Encode
    std::vector<DecodedRecord> recs = {src};
    std::vector<uint8_t> encoded = codec.encode(1, recs);
    hexdump(encoded, "encoded");
    CHECK(encoded.size() >= 3, "encoded block non-empty");

    // Decode back
    DecodedBlock block = codec.decode(encoded);
    CHECK(block.valid, "round-trip block valid");
    CHECK(block.records.size() == 1, "one record after round-trip");

    if (block.records.empty()) return;
    const auto& rec = block.records[0];
    CHECK(rec.valid, "round-trip record valid");
    CHECK(rec.uap_variation == "track", "round-trip UAP = track");

    if (rec.items.count("010")) {
        CHECK(rec.items.at("010").fields.at("SAC") == 1, "RT SAC == 1");
        CHECK(rec.items.at("010").fields.at("SIC") == 2, "RT SIC == 2");
    } else {
        std::cerr << "FAIL I010 missing after round-trip\n"; ++failures;
    }

    if (rec.items.count("020")) {
        CHECK(rec.items.at("020").fields.at("TYP")    == 1, "RT TYP == 1");
        CHECK(rec.items.at("020").fields.at("SSRPSR") == 3, "RT SSRPSR == 3");
    } else {
        std::cerr << "FAIL I020 missing after round-trip\n"; ++failures;
    }

    if (rec.items.count("161")) {
        CHECK(rec.items.at("161").fields.at("TRKNO") == 42, "RT TRKNO == 42");
    }

    if (rec.items.count("040")) {
        CHECK(rec.items.at("040").fields.at("RHO")   == 12800, "RT RHO == 12800");
        CHECK(rec.items.at("040").fields.at("THETA") == 16384, "RT THETA == 16384");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Repetitive item decode (I001/030 Warning/Error Conditions)
// ─────────────────────────────────────────────────────────────────────────────
static void testRepetitiveItem(const Codec& codec) {
    std::cout << "\n=== Test: Decode I001/030 repetitive item ===\n";

    // Build a record: I010 + I030
    // FSPEC (plot UAP):
    //   slot1=I010(bit7)=1, slot2=I020(bit6)=0, slot3=I040(bit5)=0, slot4=I070(bit4)=0,
    //   slot5=I090(bit3)=0, slot6=I130(bit2)=0, slot7=I141(bit1)=0, FX=1 (more fspec)
    //   → fspec[0] = 1000_0001 = 0x81
    // Second FSPEC byte (slots 8-14):
    //   slot8=I050(bit7)=0, slot9=I120(bit6)=0, slot10=I131(bit5)=0,
    //   slot11=I080(bit4)=0, slot12=I100(bit3)=0, slot13=I060(bit2)=0, slot14=I030(bit1)=1, FX=0
    //   → fspec[1] = 0000_0010 = 0x02
    //
    // I010: SAC=0x01, SIC=0x02 → 0x01 0x02
    //
    // I030 two repetitions: value 4 (FX=1), value 64 (FX=0)
    //   byte1 = (4 << 1) | 1 = 0x09
    //   byte2 = (64 << 1) | 0 = 0x80

    std::vector<uint8_t> frame;
    frame.push_back(0x01);   // CAT
    frame.push_back(0x00);   // LEN high (placeholder)
    frame.push_back(0x00);   // LEN low  (placeholder)
    frame.push_back(0x81);   // FSPEC byte 1 (I010=1, FX=1)
    frame.push_back(0x02);   // FSPEC byte 2 (I030=1, FX=0)
    frame.push_back(0x01);   // I010 SAC
    frame.push_back(0x02);   // I010 SIC
    frame.push_back(0x09);   // I030 rep1: value=4, FX=1
    frame.push_back(0x80);   // I030 rep2: value=64, FX=0

    uint16_t len = static_cast<uint16_t>(frame.size());
    frame[1] = static_cast<uint8_t>(len >> 8);
    frame[2] = static_cast<uint8_t>(len & 0xFF);

    hexdump(frame, "input");
    DecodedBlock block = codec.decode(frame);

    CHECK(block.valid, "block.valid");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "record.valid");
    CHECK(rec.items.count("030"), "I030 present");

    if (rec.items.count("030")) {
        const auto& reps = rec.items.at("030").repetitions;
        CHECK(reps.size() == 2,  "I030 has 2 repetitions");
        if (reps.size() >= 2) {
            CHECK(reps[0] == 4,  "I030 rep[0] == 4");
            CHECK(reps[1] == 64, "I030 rep[1] == 64");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Special Purpose Field round-trip
// ─────────────────────────────────────────────────────────────────────────────
static void testSPField(const Codec& codec) {
    std::cout << "\n=== Test: SP field round-trip ===\n";

    // Build record with I010 + SP (plot UAP)
    // SP is slot 20 in the plot UAP → FSPEC spans 3 octets
    // Slots 1-7: I010=1, rest=0, FX=1   → 0x81
    // Slots 8-14: all 0, FX=1            → 0x01
    // Slots 15-21: I150(slot15)=0, ..., SP(slot20)=1, rfs(slot21)=0, FX=0
    //   SP is at bit position 7-(20-15)= 7-5 = 2 → bit2 set
    //   fspec[2] = 0000_0100 = 0x04

    DecodedRecord src;
    src.uap_variation = "plot";
    {
        DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
        di.fields["SAC"] = 7; di.fields["SIC"] = 8;
        src.items["010"] = std::move(di);
    }
    {
        DecodedItem di; di.item_id = "SP"; di.type = ItemType::SP;
        di.raw_bytes = {0xDE, 0xAD, 0xBE, 0xEF};
        src.items["SP"] = std::move(di);
    }

    auto encoded = codec.encode(1, {src});
    hexdump(encoded, "SP encoded");
    CHECK(encoded.size() >= 3, "SP block non-empty");

    auto block = codec.decode(encoded);
    CHECK(block.valid, "SP block valid");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "SP record valid");
    CHECK(rec.items.count("SP"), "SP item present");
    if (rec.items.count("SP")) {
        const auto& rb = rec.items.at("SP").raw_bytes;
        CHECK(rb.size() == 4, "SP payload size == 4");
        if (rb.size() == 4) {
            CHECK(rb[0] == 0xDE && rb[1] == 0xAD &&
                  rb[2] == 0xBE && rb[3] == 0xEF,
                  "SP payload bytes correct");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Data Block with two Data Records (one plot + one track)
//
//  This validates the common operational case where a single ASTERIX frame
//  carries multiple target reports for the same category.
// ─────────────────────────────────────────────────────────────────────────────
static void testMultiRecord(const Codec& codec) {
    std::cout << "\n=== Test: Multi-record Data Block (plot + track) ===\n";

    // ── Record 1: plot  (I010 + I020 + I040) ─────────────────────────────────
    DecodedRecord rec1;
    rec1.uap_variation = "plot";
    {
        DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
        di.fields["SAC"] = 10; di.fields["SIC"] = 20;
        rec1.items["010"] = std::move(di);
    }
    {
        DecodedItem di; di.item_id = "020"; di.type = ItemType::Extended;
        di.fields["TYP"]    = 0;  // plot
        di.fields["SIM"]    = 0;
        di.fields["SSRPSR"] = 2;  // sole secondary
        di.fields["ANT"]    = 0;
        di.fields["SPI"]    = 1;  // SPI set
        di.fields["RAB"]    = 0;
        rec1.items["020"] = std::move(di);
    }
    {
        // RHO = 50 NM → raw = 50 * 128 = 6400
        // THETA = 45° → raw = 45 / (360/65536) ≈ 8192
        DecodedItem di; di.item_id = "040"; di.type = ItemType::Fixed;
        di.fields["RHO"]   = 6400;
        di.fields["THETA"] = 8192;
        rec1.items["040"] = std::move(di);
    }

    // ── Record 2: track  (I010 + I020 + I161 + I170) ─────────────────────────
    DecodedRecord rec2;
    rec2.uap_variation = "track";
    {
        DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
        di.fields["SAC"] = 11; di.fields["SIC"] = 22;
        rec2.items["010"] = std::move(di);
    }
    {
        DecodedItem di; di.item_id = "020"; di.type = ItemType::Extended;
        di.fields["TYP"]    = 1;  // track
        di.fields["SIM"]    = 0;
        di.fields["SSRPSR"] = 3;  // combined
        di.fields["ANT"]    = 1;  // antenna 2
        di.fields["SPI"]    = 0;
        di.fields["RAB"]    = 0;
        rec2.items["020"] = std::move(di);
    }
    {
        DecodedItem di; di.item_id = "161"; di.type = ItemType::Fixed;
        di.fields["TRKNO"] = 777;
        rec2.items["161"] = std::move(di);
    }
    {
        DecodedItem di; di.item_id = "170"; di.type = ItemType::Extended;
        di.fields["CON"]  = 0;  // confirmed
        di.fields["RAD"]  = 1;  // SSR/combined
        di.fields["MAN"]  = 0;
        di.fields["DOU"]  = 0;
        di.fields["RDPC"] = 0;
        di.fields["GHO"]  = 0;
        rec2.items["170"] = std::move(di);
    }

    // ── Encode both records into one block ────────────────────────────────────
    auto encoded = codec.encode(1, {rec1, rec2});
    hexdump(encoded, "multi-record encoded");
    CHECK(encoded.size() >= 3, "multi-record block non-empty");

    // ── Decode and verify ─────────────────────────────────────────────────────
    auto block = codec.decode(encoded);
    CHECK(block.valid,                      "multi-record block.valid");
    CHECK(block.cat == 1,                   "multi-record block.cat == 1");
    CHECK(block.records.size() == 2,        "two records decoded");

    if (block.records.size() < 2) { ++failures; return; }

    // Verify record 1 (plot)
    const auto& r1 = block.records[0];
    CHECK(r1.valid,                         "record[0] valid");
    CHECK(r1.uap_variation == "plot",       "record[0] UAP = plot");
    CHECK(r1.items.count("010"),            "record[0] I010 present");
    CHECK(r1.items.count("020"),            "record[0] I020 present");
    CHECK(r1.items.count("040"),            "record[0] I040 present");
    if (r1.items.count("010")) {
        CHECK(r1.items.at("010").fields.at("SAC") == 10, "record[0] SAC == 10");
        CHECK(r1.items.at("010").fields.at("SIC") == 20, "record[0] SIC == 20");
    }
    if (r1.items.count("020")) {
        CHECK(r1.items.at("020").fields.at("TYP")    == 0, "record[0] TYP == 0");
        CHECK(r1.items.at("020").fields.at("SSRPSR") == 2, "record[0] SSRPSR == 2");
        CHECK(r1.items.at("020").fields.at("SPI")    == 1, "record[0] SPI == 1");
    }
    if (r1.items.count("040")) {
        CHECK(r1.items.at("040").fields.at("RHO")   == 6400, "record[0] RHO == 6400");
        CHECK(r1.items.at("040").fields.at("THETA") == 8192, "record[0] THETA == 8192");
    }

    // Verify record 2 (track)
    const auto& r2 = block.records[1];
    CHECK(r2.valid,                         "record[1] valid");
    CHECK(r2.uap_variation == "track",      "record[1] UAP = track");
    CHECK(r2.items.count("010"),            "record[1] I010 present");
    CHECK(r2.items.count("020"),            "record[1] I020 present");
    CHECK(r2.items.count("161"),            "record[1] I161 present");
    CHECK(r2.items.count("170"),            "record[1] I170 present");
    if (r2.items.count("010")) {
        CHECK(r2.items.at("010").fields.at("SAC") == 11, "record[1] SAC == 11");
        CHECK(r2.items.at("010").fields.at("SIC") == 22, "record[1] SIC == 22");
    }
    if (r2.items.count("020")) {
        CHECK(r2.items.at("020").fields.at("TYP")    == 1, "record[1] TYP == 1");
        CHECK(r2.items.at("020").fields.at("SSRPSR") == 3, "record[1] SSRPSR == 3");
        CHECK(r2.items.at("020").fields.at("ANT")    == 1, "record[1] ANT == 1");
    }
    if (r2.items.count("161")) {
        CHECK(r2.items.at("161").fields.at("TRKNO") == 777, "record[1] TRKNO == 777");
    }
    if (r2.items.count("170")) {
        CHECK(r2.items.at("170").fields.at("RAD") == 1, "record[1] RAD == 1");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pretty-printer: renders a DecodedBlock with physical values and table lookups
// ─────────────────────────────────────────────────────────────────────────────

static std::string fmtOctal(uint64_t raw, int bits) {
    int digits = (bits + 2) / 3;
    std::ostringstream oss;
    oss << std::oct << std::setw(digits) << std::setfill('0') << raw;
    return oss.str();
}

static std::string fmtElement(const ElementDef& e, uint64_t raw) {
    switch (e.encoding) {
    case Encoding::Raw:
        return std::to_string(raw) + " (0x" + [&]{
            std::ostringstream s; s << std::hex << raw; return s.str();
        }() + ")";
    case Encoding::Table: {
        auto it = e.table.find(raw);
        std::string m = (it != e.table.end()) ? it->second : "?";
        return std::to_string(raw) + " [" + m + "]";
    }
    case Encoding::UnsignedQuantity: {
        std::ostringstream s;
        s << std::fixed << std::setprecision(4) << raw * e.scale << " " << e.unit
          << "  (raw=" << raw << ")";
        return s.str();
    }
    case Encoding::SignedQuantity: {
        int64_t sv;
        if (e.bits > 0 && e.bits < 64 && ((raw >> (e.bits - 1)) & 1u))
            sv = static_cast<int64_t>(raw | (~uint64_t{0} << e.bits));
        else
            sv = static_cast<int64_t>(raw);
        std::ostringstream s;
        s << std::fixed << std::setprecision(4) << sv * e.scale << " " << e.unit
          << "  (raw=" << sv << ")";
        return s.str();
    }
    case Encoding::StringOctal:
        return fmtOctal(raw, e.bits);
    default:
        return std::to_string(raw);
    }
}

// Search for an ElementDef by name in Fixed element list or Extended octets.
static const ElementDef* findElem(const DataItemDef& def, const std::string& name) {
    for (const auto& e : def.elements)  if (e.name == name) return &e;
    for (const auto& oct : def.octets)
        for (const auto& e : oct.elements) if (e.name == name) return &e;
    return nullptr;
}

static void printBlock(const DecodedBlock& block, const CategoryDef& cat) {
    std::cout << "  CAT=" << (int)block.cat
              << "  LEN=" << block.length
              << "  records=" << block.records.size() << "\n";

    for (size_t ri = 0; ri < block.records.size(); ++ri) {
        const auto& rec = block.records[ri];
        std::cout << "\n  +-- Record [" << ri << "]"
                  << "  UAP=" << rec.uap_variation;
        if (!rec.valid) std::cout << "  *** ERROR: " << rec.error;
        std::cout << "\n";

        // Iterate in UAP order for deterministic output
        const auto& uap = cat.uap_variations.at(rec.uap_variation);
        for (const auto& id : uap) {
            if (id == "-" || id == "rfs") continue;
            auto it = rec.items.find(id);
            if (it == rec.items.end()) continue;

            const DataItemDef& def = cat.items.at(id);
            const DecodedItem& item = it->second;
            std::cout << "  |    I001/" << id << " - " << def.name << "\n";

            auto printFields = [&](const std::vector<ElementDef>& elems) {
                for (const auto& e : elems) {
                    if (e.is_spare) continue;
                    auto fit = item.fields.find(e.name);
                    if (fit == item.fields.end()) continue;
                    std::cout << "  |        " << e.name
                              << " = " << fmtElement(e, fit->second) << "\n";
                }
            };

            switch (item.type) {
            case ItemType::Fixed:
                printFields(def.elements);
                break;
            case ItemType::Extended:
                for (const auto& oct : def.octets) printFields(oct.elements);
                break;
            case ItemType::Repetitive:
                for (size_t i = 0; i < item.repetitions.size(); ++i)
                    std::cout << "  |        [" << i << "] = "
                              << fmtElement(def.rep_element, item.repetitions[i]) << "\n";
                break;
            case ItemType::SP:
                std::cout << "  |        [" << item.raw_bytes.size() << " bytes]: ";
                for (uint8_t b : item.raw_bytes)
                    std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b << ' ';
                std::cout << std::dec << "\n";
                break;
            default: break;
            }
        }
        std::cout << "  +--\n";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Real-world CAT01 frame – 4 track records from the same radar
//
//  Frame (hex):
//    01 00 53 F7 84 08 11 A8 00 4A 46 D7 EA 2E 08 43 A2 F8 0F 82 05 C8 48
//    F7 84 08 11 A8 05 28 29 0F EB 01 08 86 51 8B 01 72 06 18 48
//    F7 84 08 11 A8 03 21 2A 26 E9 FE 08 90 51 38 01 6B 05 C8 48
//    F7 84 08 11 A8 05 07 19 80 EB 54 08 3E 0C 38 02 00 06 40 48
//
//  FSPEC = F7 84  →  items in track UAP:
//    slot1=I010(✓) slot2=I020(✓) slot3=I161(✓) slot4=I040(✓)
//    slot5=I042(✗) slot6=I200(✓) slot7=I070(✓) / slot8=I090(✓) slot13=I170(✓)
//  Items per record (18 data bytes):
//    I010(2) + I020(1,FX=0) + I161(2) + I040(4) + I200(4) + I070(2) + I090(2) + I170(1,FX=0) = 18 ✓
//
//  Pre-computed expected values (record 0):
//    I010: SAC=0x08 SIC=0x11=17
//    I020: TYP=1 (track) SSRPSR=2 (sole secondary) ANT=1 (antenna 2)
//    I161: TRKNO=74
//    I040: RHO raw=18135 (141.68 NM)  THETA raw=59950 (≈329.4°)
//    I200: GSP raw=2115 (≈464 kt)     HDG raw=41720 (≈229°)
//    I070: V=0 G=0 MODE3A=0xF82 (octal 7602)
//    I090: V=0 G=0 HGT raw=1480 (370 FL)
//    I170: CON=0 RAD=1 RDPC=1
// ─────────────────────────────────────────────────────────────────────────────
static void testRealMessage(const Codec& codec) {
    std::cout << "\n=== Test: Real CAT01 frame (4 track records) ===\n";

    // clang-format off
    const std::vector<uint8_t> frame = {
        // Data Block header
        0x01, 0x00, 0x53,
        // Record 0
        0xF7, 0x84,  0x08,0x11,  0xA8,  0x00,0x4A,
        0x46,0xD7, 0xEA,0x2E,  0x08,0x43, 0xA2,0xF8,
        0x0F,0x82,  0x05,0xC8,  0x48,
        // Record 1
        0xF7, 0x84,  0x08,0x11,  0xA8,  0x05,0x28,
        0x29,0x0F, 0xEB,0x01,  0x08,0x86, 0x51,0x8B,
        0x01,0x72,  0x06,0x18,  0x48,
        // Record 2
        0xF7, 0x84,  0x08,0x11,  0xA8,  0x03,0x21,
        0x2A,0x26, 0xE9,0xFE,  0x08,0x90, 0x51,0x38,
        0x01,0x6B,  0x05,0xC8,  0x48,
        // Record 3
        0xF7, 0x84,  0x08,0x11,  0xA8,  0x05,0x07,
        0x19,0x80, 0xEB,0x54,  0x08,0x3E, 0x0C,0x38,
        0x02,0x00,  0x06,0x40,  0x48
    };
    // clang-format on

    hexdump(frame, "input");
    DecodedBlock block = codec.decode(frame);

    // ── Block-level assertions ────────────────────────────────────────────────
    CHECK(block.valid,           "real: block.valid");
    CHECK(block.cat    == 1,     "real: cat == 1");
    CHECK(block.length == 83,    "real: length == 83");
    CHECK(block.records.size() == 4, "real: 4 records");

    if (block.records.size() != 4) { ++failures; return; }

    // Expected values per record (index → {TRKNO, RHO, THETA, GSP, HDG, MODE3A, HGT})
    struct Expected {
        uint16_t trkno;
        uint16_t rho;    // raw
        uint16_t theta;  // raw
        uint16_t gsp;    // raw
        uint16_t hdg;    // raw
        uint16_t mode3a; // raw (12-bit)
        uint16_t hgt;    // raw (14-bit)
    };
    const Expected ex[4] = {
        { 74,   18135, 59950, 2115, 41720, 0xF82, 1480 },
        { 1320, 10511, 60161, 2182, 20875, 0x172, 1560 },
        {  801, 10790, 59902, 2192, 20792, 0x16B, 1480 },
        { 1287,  6528, 60244, 2110,  3128, 0x200, 1600 },
    };

    for (size_t i = 0; i < 4; ++i) {
        const auto& rec = block.records[i];
        const auto& e   = ex[i];
        std::string p   = "real rec[" + std::to_string(i) + "] ";

        CHECK(rec.valid,                      p + "valid");
        CHECK(rec.uap_variation == "track",   p + "UAP=track");
        CHECK(!rec.items.count("042"),        p + "I042 absent");

        // I010
        if (rec.items.count("010")) {
            CHECK(rec.items.at("010").fields.at("SAC") == 8,  p + "SAC=8");
            CHECK(rec.items.at("010").fields.at("SIC") == 17, p + "SIC=17");
        } else { std::cerr << "FAIL " << p << "I010 missing\n"; ++failures; }

        // I020
        if (rec.items.count("020")) {
            CHECK(rec.items.at("020").fields.at("TYP")    == 1, p + "TYP=1");
            CHECK(rec.items.at("020").fields.at("SSRPSR") == 2, p + "SSRPSR=2");
            CHECK(rec.items.at("020").fields.at("ANT")    == 1, p + "ANT=1");
        } else { std::cerr << "FAIL " << p << "I020 missing\n"; ++failures; }

        // I161
        if (rec.items.count("161"))
            CHECK(rec.items.at("161").fields.at("TRKNO") == e.trkno, p + "TRKNO");
        else { std::cerr << "FAIL " << p << "I161 missing\n"; ++failures; }

        // I040
        if (rec.items.count("040")) {
            CHECK(rec.items.at("040").fields.at("RHO")   == e.rho,   p + "RHO");
            CHECK(rec.items.at("040").fields.at("THETA") == e.theta,  p + "THETA");
        } else { std::cerr << "FAIL " << p << "I040 missing\n"; ++failures; }

        // I200
        if (rec.items.count("200")) {
            CHECK(rec.items.at("200").fields.at("GSP") == e.gsp, p + "GSP");
            CHECK(rec.items.at("200").fields.at("HDG") == e.hdg, p + "HDG");
        } else { std::cerr << "FAIL " << p << "I200 missing\n"; ++failures; }

        // I070
        if (rec.items.count("070"))
            CHECK(rec.items.at("070").fields.at("MODE3A") == e.mode3a, p + "MODE3A");
        else { std::cerr << "FAIL " << p << "I070 missing\n"; ++failures; }

        // I090
        if (rec.items.count("090"))
            CHECK(rec.items.at("090").fields.at("HGT") == e.hgt, p + "HGT");
        else { std::cerr << "FAIL " << p << "I090 missing\n"; ++failures; }

        // I170 – first octet only (FX=0): CON=0, RAD=1, RDPC=1
        if (rec.items.count("170")) {
            CHECK(rec.items.at("170").fields.at("CON")  == 0, p + "CON=0");
            CHECK(rec.items.at("170").fields.at("RAD")  == 1, p + "RAD=1");
            CHECK(rec.items.at("170").fields.at("RDPC") == 1, p + "RDPC=1");
        } else { std::cerr << "FAIL " << p << "I170 missing\n"; ++failures; }
    }

    // Pretty-print decoded content
    printBlock(block, codec.category(1));
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    // Allow overriding the spec path via command-line (handy for out-of-tree builds)
    fs::path spec_path = (argc > 1)
        ? fs::path(argv[1])
        : fs::path(__FILE__).parent_path().parent_path() / "specs" / "CAT01.xml";

    std::cout << "Using spec: " << spec_path << '\n';

    Codec codec;
    testSpecLoad(codec, spec_path);

    if (failures == 0) {
        // Only run decode/encode tests if spec loaded successfully
        testDecodePlot(codec);
        testRoundTripTrack(codec);
        testRepetitiveItem(codec);
        testSPField(codec);
        testMultiRecord(codec);
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
