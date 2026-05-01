// test_cat240.cpp – Tests for CAT240 Radar Video Transmission.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat240

#include "ASTERIXCodec/Codec.hpp"
#include "ASTERIXCodec/SpecLoader.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace asterix;

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
//  Test 1: XML spec loads correctly
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT240 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 240,        "cat number = 240");
    CHECK(cat.edition == "1.3",  "edition = 1.3");

    for (auto id : {"000","010","020","030","040","041","048","049",
                    "050","051","052","140","RE","SP"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 14, "14 items total");

    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    CHECK(cat.uap_variations.at("default").size() == 14, "UAP has 14 slots");

    const auto& uap = cat.uap_variations.at("default");
    // Byte 1: 010 000 020 030 040 041 048
    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "000", "UAP slot  2 = 000");
    CHECK(uap[2]  == "020", "UAP slot  3 = 020");
    CHECK(uap[3]  == "030", "UAP slot  4 = 030");
    CHECK(uap[4]  == "040", "UAP slot  5 = 040");
    CHECK(uap[5]  == "041", "UAP slot  6 = 041");
    CHECK(uap[6]  == "048", "UAP slot  7 = 048");
    // Byte 2: 049 050 051 052 140 RE SP
    CHECK(uap[7]  == "049", "UAP slot  8 = 049");
    CHECK(uap[8]  == "050", "UAP slot  9 = 050");
    CHECK(uap[9]  == "051", "UAP slot 10 = 051");
    CHECK(uap[10] == "052", "UAP slot 11 = 052");
    CHECK(uap[11] == "140", "UAP slot 12 = 140");
    CHECK(uap[12] == "RE",  "UAP slot 13 = RE");
    CHECK(uap[13] == "SP",  "UAP slot 14 = SP");

    // Item types
    for (auto id : {"000","010","020","040","041","048","049","140"}) {
        CHECK(cat.items.at(id).type == ItemType::Fixed,
              std::string(id) + " is Fixed");
    }
    for (auto id : {"030","050","051","052"}) {
        CHECK(cat.items.at(id).type == ItemType::RepetitiveGroup,
              std::string(id) + " is RepetitiveGroup");
    }
    CHECK(cat.items.at("RE").type == ItemType::SP, "RE is SP/Explicit");

    // Fixed byte sizes
    CHECK(cat.items.at("000").fixed_bytes == 1,  "000 = 1 byte");
    CHECK(cat.items.at("010").fixed_bytes == 2,  "010 = 2 bytes");
    CHECK(cat.items.at("020").fixed_bytes == 4,  "020 = 4 bytes");
    CHECK(cat.items.at("040").fixed_bytes == 12, "040 = 12 bytes");
    CHECK(cat.items.at("041").fixed_bytes == 12, "041 = 12 bytes");
    CHECK(cat.items.at("048").fixed_bytes == 2,  "048 = 2 bytes");
    CHECK(cat.items.at("049").fixed_bytes == 5,  "049 = 5 bytes");
    CHECK(cat.items.at("140").fixed_bytes == 3,  "140 = 3 bytes");

    // RepetitiveGroup specs
    {
        const auto& i030 = cat.items.at("030");
        CHECK(i030.rep_group_bits == 8,   "030 rep_group_bits = 8");
        CHECK(i030.rep_group_elements.size() == 1, "030 has 1 element");
        CHECK(i030.rep_group_elements[0].name == "CHAR", "030 element[0] = CHAR");
    }
    {
        const auto& i050 = cat.items.at("050");
        CHECK(i050.rep_group_bits == 32,  "050 rep_group_bits = 32");
        CHECK(i050.rep_group_elements.size() == 1, "050 has 1 element");
        CHECK(i050.rep_group_elements[0].name == "BLOCK", "050 element[0] = BLOCK");
    }
    {
        const auto& i051 = cat.items.at("051");
        CHECK(i051.rep_group_bits == 512, "051 rep_group_bits = 512 (8 × 64 bits)");
        CHECK(i051.rep_group_elements.size() == 8, "051 has 8 elements (B0-B7)");
        CHECK(i051.rep_group_elements[0].name == "B0", "051 element[0] = B0");
        CHECK(i051.rep_group_elements[7].name == "B7", "051 element[7] = B7");
    }
    {
        const auto& i052 = cat.items.at("052");
        CHECK(i052.rep_group_bits == 2048, "052 rep_group_bits = 2048 (32 × 64 bits)");
        CHECK(i052.rep_group_elements.size() == 32, "052 has 32 elements (B00-B31)");
        CHECK(i052.rep_group_elements[0].name  == "B00", "052 element[0] = B00");
        CHECK(i052.rep_group_elements[31].name == "B31", "052 element[31] = B31");
    }

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode basic Video Summary message from raw bytes
//          I010(SAC=1,SIC=2) + I000(MSGTYP=1 "Video Summary message")
//
//  UAP slot 0=010(bit7), slot 1=000(bit6) → FSPEC=0xC0
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicMessage(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT240 Video Summary message ===\n";

    // LEN = 3(header) + 1(FSPEC) + 2(I010) + 1(I000) = 7
    std::vector<uint8_t> frame = {
        0xF0,             // CAT = 240
        0x00, 0x07,       // LEN = 7
        0xC0,             // FSPEC byte 1 (FX=0): I010(b7)+I000(b6)
        0x01, 0x02,       // I010 SAC=1, SIC=2
        0x01              // I000 MSGTYP=1 (Video Summary)
    };

    hexdump(frame, "Video Summary message input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid, "block is valid");
    CHECK(blk.records.size() == 1, "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"), "I010 present");
    CHECK(rec.items.count("000"), "I000 present");
    CHECK(!rec.items.count("020"), "I020 absent");
    CHECK(!rec.items.count("050"), "I050 absent");

    CHECK(rec.items.at("010").fields.at("SAC")    == 1, "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC")    == 2, "I010.SIC=2");
    CHECK(rec.items.at("000").fields.at("MSGTYP") == 1, "I000.MSGTYP=1 (Video Summary)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip simple Fixed items (I010, I000, I020, I140)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripSimpleFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip simple Fixed items ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=3; it.fields["SIC"]=7; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=2; enc_rec.items["000"]=it; }  // Video message
    { DecodedItem it; it.fields["MSI"]=0x00001234; enc_rec.items["020"]=it; }
    { DecodedItem it; it.fields["TOD"]=0x600000; enc_rec.items["140"]=it; }

    std::vector<uint8_t> encoded = codec.encode(240, {enc_rec});
    hexdump(encoded, "Simple Fixed encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("SAC")    == 3,          "I010.SAC=3");
    CHECK(items.at("010").fields.at("SIC")    == 7,          "I010.SIC=7");
    CHECK(items.at("000").fields.at("MSGTYP") == 2,          "I000.MSGTYP=2 (Video message)");
    CHECK(items.at("020").fields.at("MSI")    == 0x00001234, "I020.MSI=0x1234");
    CHECK(items.at("140").fields.at("TOD")    == 0x600000,   "I140.TOD=0x600000");

    // MSI wraps at 32 bits
    DecodedRecord r2;
    r2.uap_variation = "default";
    { DecodedItem it; it.fields["MSI"]=0xFFFFFFFFu; r2.items["020"]=it; }
    auto enc2 = codec.encode(240, {r2});
    auto blk2 = codec.decode(enc2);
    if (!blk2.records.empty())
        CHECK(blk2.records[0].items.at("020").fields.at("MSI") == 0xFFFFFFFFu,
              "I020.MSI=0xFFFFFFFF (max)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip I030 Video Summary (ASCII free text as byte stream)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripVideoSummary(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I030 Video Summary ===\n";

    // "RADAR1" in ASCII
    const std::string text = "RADAR1";
    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    DecodedItem i030;
    for (char c : text)
        i030.group_repetitions.push_back({{"CHAR", static_cast<uint64_t>(c)}});
    enc_rec.items["030"] = i030;

    std::vector<uint8_t> encoded = codec.encode(240, {enc_rec});
    hexdump(encoded, "I030 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("030"), "I030 present");
    const auto& groups = blk.records[0].items.at("030").group_repetitions;
    CHECK(groups.size() == 6, "I030: 6 characters");

    if (groups.size() == 6) {
        const std::string decoded_text = "RADAR1";
        for (size_t i = 0; i < decoded_text.size(); ++i) {
            CHECK(groups[i].at("CHAR") == static_cast<uint64_t>(decoded_text[i]),
                  "I030 char[" + std::to_string(i) + "] = '" + decoded_text[i] + "'");
        }
    }

    // Empty string (0 characters)
    DecodedRecord r2;
    r2.uap_variation = "default";
    DecodedItem i2;  // no group_repetitions
    r2.items["030"] = i2;
    auto enc2 = codec.encode(240, {r2});
    auto blk2 = codec.decode(enc2);
    if (!blk2.records.empty())
        CHECK(blk2.records[0].items.at("030").group_repetitions.empty(),
              "I030: 0 characters (empty)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip I040 Video Header Nano and I041 Video Header Femto
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripVideoHeaders(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I040/I041 Video Headers ===\n";

    // I040: azimuth 90°→180°, STARTRG=0, CELLDUR=1000 ns
    // STARTAZ=90° → 90/0.0054931640625 = 16384 = 0x4000
    // ENDAZ=180°  → 180/0.0054931640625 = 32768 = 0x8000
    {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        DecodedItem i040;
        i040.fields["STARTAZ"] = 0x4000;  // 90°
        i040.fields["ENDAZ"]   = 0x8000;  // 180°
        i040.fields["STARTRG"] = 0;
        i040.fields["CELLDUR"] = 1000;    // 1000 ns = 1 µs
        enc_rec.items["040"] = i040;

        auto encoded = codec.encode(240, {enc_rec});
        hexdump(encoded, "I040 Nano encoded");
        auto blk = codec.decode(encoded);
        CHECK(blk.valid, "I040: block valid");
        if (!blk.records.empty()) {
            const auto& f = blk.records[0].items.at("040").fields;
            CHECK(f.at("STARTAZ") == 0x4000, "I040.STARTAZ=0x4000 (90°)");
            CHECK(f.at("ENDAZ")   == 0x8000, "I040.ENDAZ=0x8000 (180°)");
            CHECK(f.at("STARTRG") == 0,      "I040.STARTRG=0");
            CHECK(f.at("CELLDUR") == 1000,   "I040.CELLDUR=1000 ns");
        }
    }

    // I041: same azimuth, CELLDUR in femtoseconds
    {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        DecodedItem i041;
        i041.fields["STARTAZ"] = 0x2000;     // 45°
        i041.fields["ENDAZ"]   = 0x4000;     // 90°
        i041.fields["STARTRG"] = 512;
        i041.fields["CELLDUR"] = 1000000000; // 1 ns in femtoseconds
        enc_rec.items["041"] = i041;

        auto encoded = codec.encode(240, {enc_rec});
        hexdump(encoded, "I041 Femto encoded");
        auto blk = codec.decode(encoded);
        CHECK(blk.valid, "I041: block valid");
        if (!blk.records.empty()) {
            const auto& f = blk.records[0].items.at("041").fields;
            CHECK(f.at("STARTAZ") == 0x2000,     "I041.STARTAZ=0x2000 (45°)");
            CHECK(f.at("ENDAZ")   == 0x4000,     "I041.ENDAZ=0x4000 (90°)");
            CHECK(f.at("STARTRG") == 512,         "I041.STARTRG=512");
            CHECK(f.at("CELLDUR") == 1000000000, "I041.CELLDUR=1000000000 fs");
        }
    }

    // STARTAZ=0 (north) and ENDAZ full circle (max = 0xFFFF)
    {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        DecodedItem i040;
        i040.fields["STARTAZ"] = 0;
        i040.fields["ENDAZ"]   = 0xFFFF;
        i040.fields["STARTRG"] = 0xFFFFFFFFu;
        i040.fields["CELLDUR"] = 0xFFFFFFFFu;
        enc_rec.items["040"] = i040;

        auto encoded = codec.encode(240, {enc_rec});
        auto blk = codec.decode(encoded);
        if (!blk.records.empty()) {
            const auto& f = blk.records[0].items.at("040").fields;
            CHECK(f.at("STARTAZ") == 0,            "I040.STARTAZ=0 (north)");
            CHECK(f.at("ENDAZ")   == 0xFFFF,       "I040.ENDAZ=0xFFFF (max)");
            CHECK(f.at("STARTRG") == 0xFFFFFFFFu,  "I040.STARTRG=0xFFFFFFFF (max)");
            CHECK(f.at("CELLDUR") == 0xFFFFFFFFu,  "I040.CELLDUR=0xFFFFFFFF (max)");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip I048 Resolution + I049 Counters
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripResolutionCounters(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I048 Resolution and I049 Counters ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    // I048: no compression (C=0), High Resolution (RES=4)
    { DecodedItem it; it.fields["C"]=0; it.fields["RES"]=4; enc_rec.items["048"]=it; }
    // I049: 100 valid octets, 800 valid cells
    { DecodedItem it; it.fields["NBVB"]=100; it.fields["NBCELLS"]=800; enc_rec.items["049"]=it; }

    std::vector<uint8_t> encoded = codec.encode(240, {enc_rec});
    hexdump(encoded, "I048+I049 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("048").fields.at("C")       == 0,   "I048.C=0 (no compression)");
    CHECK(items.at("048").fields.at("RES")     == 4,   "I048.RES=4 (High Resolution)");
    CHECK(items.at("049").fields.at("NBVB")    == 100, "I049.NBVB=100");
    CHECK(items.at("049").fields.at("NBCELLS") == 800, "I049.NBCELLS=800");

    // Test compression flag and all RES values
    for (uint64_t res = 1; res <= 6; ++res) {
        DecodedRecord r2;
        r2.uap_variation = "default";
        { DecodedItem it; it.fields["C"]=1; it.fields["RES"]=res; r2.items["048"]=it; }
        auto enc2 = codec.encode(240, {r2});
        auto blk2 = codec.decode(enc2);
        if (!blk2.records.empty()) {
            const auto& f2 = blk2.records[0].items.at("048").fields;
            CHECK(f2.at("C")   == 1,   "I048.C=1 (compression)");
            CHECK(f2.at("RES") == res, "I048.RES=" + std::to_string(res));
        }
    }

    // I049 boundary: max NBVB=65535, max NBCELLS=16777215 (24-bit)
    DecodedRecord r3;
    r3.uap_variation = "default";
    { DecodedItem it; it.fields["NBVB"]=65535; it.fields["NBCELLS"]=16777215; r3.items["049"]=it; }
    auto enc3 = codec.encode(240, {r3});
    auto blk3 = codec.decode(enc3);
    if (!blk3.records.empty()) {
        const auto& f3 = blk3.records[0].items.at("049").fields;
        CHECK(f3.at("NBVB")    == 65535,    "I049.NBVB=65535 (max 16-bit)");
        CHECK(f3.at("NBCELLS") == 16777215, "I049.NBCELLS=16777215 (max 24-bit)");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip I050 Video Block Low Data Volume
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripVideoBlockLow(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I050 Video Block Low ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    DecodedItem i050;
    i050.group_repetitions.push_back({{"BLOCK", 0x00000000u}});
    i050.group_repetitions.push_back({{"BLOCK", 0xDEADBEEFu}});
    i050.group_repetitions.push_back({{"BLOCK", 0xFFFFFFFFu}});
    i050.group_repetitions.push_back({{"BLOCK", 0x12345678u}});
    enc_rec.items["050"] = i050;

    std::vector<uint8_t> encoded = codec.encode(240, {enc_rec});
    hexdump(encoded, "I050 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("050"), "I050 present");
    const auto& groups = blk.records[0].items.at("050").group_repetitions;
    CHECK(groups.size() == 4, "I050: 4 blocks");

    if (groups.size() == 4) {
        CHECK(groups[0].at("BLOCK") == 0x00000000u, "I050[0].BLOCK=0x00000000");
        CHECK(groups[1].at("BLOCK") == 0xDEADBEEFu, "I050[1].BLOCK=0xDEADBEEF");
        CHECK(groups[2].at("BLOCK") == 0xFFFFFFFFu, "I050[2].BLOCK=0xFFFFFFFF");
        CHECK(groups[3].at("BLOCK") == 0x12345678u, "I050[3].BLOCK=0x12345678");
    }

    // Single block
    DecodedRecord r2;
    r2.uap_variation = "default";
    DecodedItem i2;
    i2.group_repetitions.push_back({{"BLOCK", 0xABCD1234u}});
    r2.items["050"] = i2;
    auto enc2 = codec.encode(240, {r2});
    auto blk2 = codec.decode(enc2);
    if (!blk2.records.empty()) {
        const auto& g2 = blk2.records[0].items.at("050").group_repetitions;
        CHECK(g2.size() == 1, "I050: 1 block");
        if (!g2.empty())
            CHECK(g2[0].at("BLOCK") == 0xABCD1234u, "I050[0].BLOCK=0xABCD1234");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Round-trip I051 Video Block Medium (1 × 512-bit block)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripVideoBlockMedium(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I051 Video Block Medium (1 block = 8 × 64 bits) ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    DecodedItem i051;

    // 1 × 512-bit block: fill B0-B7 with sequential pattern
    std::map<std::string, uint64_t> block;
    for (int i = 0; i < 8; ++i)
        block["B" + std::to_string(i)] = static_cast<uint64_t>(0x0102030405060708ULL + i);
    i051.group_repetitions.push_back(block);
    enc_rec.items["051"] = i051;

    std::vector<uint8_t> encoded = codec.encode(240, {enc_rec});
    hexdump(encoded, "I051 encoded (1 block = 64 bytes)");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("051"), "I051 present");
    const auto& groups = blk.records[0].items.at("051").group_repetitions;
    CHECK(groups.size() == 1, "I051: 1 block");

    if (!groups.empty()) {
        for (int i = 0; i < 8; ++i) {
            const std::string key = "B" + std::to_string(i);
            const uint64_t expected = 0x0102030405060708ULL + i;
            CHECK(groups[0].at(key) == expected,
                  "I051[0].B" + std::to_string(i) + " matches");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 9: Full Video message round-trip with all video items
// ─────────────────────────────────────────────────────────────────────────────
static void testFullVideoMessage(Codec& codec) {
    std::cout << "\n=== Test: Full Video message round-trip CAT240 ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    // Header
    { DecodedItem it; it.fields["SAC"]=10; it.fields["SIC"]=20; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=2; enc_rec.items["000"]=it; }
    { DecodedItem it; it.fields["MSI"]=0x000000FF; enc_rec.items["020"]=it; }
    { DecodedItem it; it.fields["TOD"]=0x300000; enc_rec.items["140"]=it; }

    // Video header nano: 270°→360° radial, STARTRG=100, CELLDUR=500ns
    { DecodedItem it;
      it.fields["STARTAZ"] = 0xC000;  // 270°
      it.fields["ENDAZ"]   = 0x0000;  // 0° (=360°)
      it.fields["STARTRG"] = 100;
      it.fields["CELLDUR"] = 500;
      enc_rec.items["040"] = it; }

    // Resolution: no compression, Medium resolution (4 bits)
    { DecodedItem it; it.fields["C"]=0; it.fields["RES"]=3; enc_rec.items["048"]=it; }

    // Counters
    { DecodedItem it; it.fields["NBVB"]=50; it.fields["NBCELLS"]=400; enc_rec.items["049"]=it; }

    // 3 video blocks (low volume)
    { DecodedItem it;
      it.group_repetitions.push_back({{"BLOCK", 0xAAAAAAAAu}});
      it.group_repetitions.push_back({{"BLOCK", 0x55555555u}});
      it.group_repetitions.push_back({{"BLOCK", 0xFF00FF00u}});
      enc_rec.items["050"] = it; }

    std::vector<uint8_t> encoded = codec.encode(240, {enc_rec});
    hexdump(encoded, "Full Video message encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.count("010"), "I010 present");
    CHECK(items.count("000"), "I000 present");
    CHECK(items.count("020"), "I020 present");
    CHECK(items.count("040"), "I040 present");
    CHECK(items.count("048"), "I048 present");
    CHECK(items.count("049"), "I049 present");
    CHECK(items.count("050"), "I050 present");
    CHECK(items.count("140"), "I140 present");

    CHECK(items.at("010").fields.at("SAC")    == 10,          "I010.SAC=10");
    CHECK(items.at("010").fields.at("SIC")    == 20,          "I010.SIC=20");
    CHECK(items.at("000").fields.at("MSGTYP") == 2,           "I000.MSGTYP=2");
    CHECK(items.at("020").fields.at("MSI")    == 0xFF,        "I020.MSI=255");
    CHECK(items.at("040").fields.at("STARTAZ")== 0xC000,      "I040.STARTAZ=0xC000 (270°)");
    CHECK(items.at("040").fields.at("STARTRG")== 100,         "I040.STARTRG=100");
    CHECK(items.at("040").fields.at("CELLDUR")== 500,         "I040.CELLDUR=500ns");
    CHECK(items.at("048").fields.at("C")      == 0,           "I048.C=0");
    CHECK(items.at("048").fields.at("RES")    == 3,           "I048.RES=3 (Medium)");
    CHECK(items.at("049").fields.at("NBVB")   == 50,          "I049.NBVB=50");
    CHECK(items.at("049").fields.at("NBCELLS")== 400,         "I049.NBCELLS=400");
    CHECK(items.at("140").fields.at("TOD")    == 0x300000,    "I140.TOD=0x300000");

    const auto& blocks = items.at("050").group_repetitions;
    CHECK(blocks.size() == 3, "I050: 3 blocks");
    if (blocks.size() == 3) {
        CHECK(blocks[0].at("BLOCK") == 0xAAAAAAAAu, "I050[0].BLOCK=0xAAAAAAAA");
        CHECK(blocks[1].at("BLOCK") == 0x55555555u, "I050[1].BLOCK=0x55555555");
        CHECK(blocks[2].at("BLOCK") == 0xFF00FF00u, "I050[2].BLOCK=0xFF00FF00");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = "specs/CAT240.xml";
    if (argc >= 2)
        spec_path = argv[1];

    if (!fs::exists(spec_path)) {
        std::cerr << "Spec not found: " << spec_path << '\n';
        return 1;
    }

    Codec codec;

    testSpecLoad(codec, spec_path);
    testDecodeBasicMessage(codec);
    testRoundTripSimpleFixed(codec);
    testRoundTripVideoSummary(codec);
    testRoundTripVideoHeaders(codec);
    testRoundTripResolutionCounters(codec);
    testRoundTripVideoBlockLow(codec);
    testRoundTripVideoBlockMedium(codec);
    testFullVideoMessage(codec);

    std::cout << "\n============================\n";
    if (failures == 0)
        std::cout << "ALL TESTS PASSED\n";
    else
        std::cout << failures << " TEST(S) FAILED\n";
    std::cout << "============================\n";

    return failures ? 1 : 0;
}
