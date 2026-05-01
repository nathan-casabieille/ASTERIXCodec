// test_cat08.cpp – Tests for CAT08 Monoradar Derived Weather Information.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat08

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

// ─── Deep comparison helpers ─────────────────────────────────────────────────
static void checkItemsMatch(const std::map<std::string, DecodedItem>& got,
                             const std::map<std::string, DecodedItem>& expected,
                             const std::string& label) {
    for (const auto& [id, src] : expected) {
        auto it = got.find(id);
        if (it == got.end()) {
            std::cerr << "FAIL [RT] " << label << " I" << id << " missing\n";
            ++failures;
            continue;
        }
        const auto& dst = it->second;
        const std::string p = label + "/I" + id;

        for (const auto& [name, val] : src.fields) {
            auto fit = dst.fields.find(name);
            if (fit == dst.fields.end()) {
                std::cerr << "FAIL [RT] " << p << "." << name << " missing\n";
                ++failures;
            } else {
                CHECK(fit->second == val, p + "." + name);
            }
        }

        if (dst.repetitions.size() != src.repetitions.size()) {
            std::cerr << "FAIL [RT] " << p << " repetitions count mismatch ("
                      << src.repetitions.size() << " vs "
                      << dst.repetitions.size() << ")\n";
            ++failures;
        } else {
            for (size_t i = 0; i < src.repetitions.size(); ++i) {
                CHECK(dst.repetitions[i] == src.repetitions[i],
                      p + ".repetitions[" + std::to_string(i) + "]");
            }
        }

        if (dst.group_repetitions.size() != src.group_repetitions.size()) {
            std::cerr << "FAIL [RT] " << p << " group_repetitions count mismatch ("
                      << src.group_repetitions.size() << " vs "
                      << dst.group_repetitions.size() << ")\n";
            ++failures;
        } else {
            for (size_t gi = 0; gi < src.group_repetitions.size(); ++gi) {
                for (const auto& [gname, gval] : src.group_repetitions[gi]) {
                    auto git = dst.group_repetitions[gi].find(gname);
                    if (git == dst.group_repetitions[gi].end()) {
                        std::cerr << "FAIL [RT] " << p << "[" << gi << "]."
                                  << gname << " missing\n";
                        ++failures;
                    } else {
                        CHECK(git->second == gval,
                              p + "[" + std::to_string(gi) + "]." + gname);
                    }
                }
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 1: XML spec loads and item types / UAP are correct
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT08 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 8,          "cat number = 8");
    CHECK(cat.edition == "1.3",  "edition = 1.3");

    for (auto id : {"000","010","020","034","036","038","040","050","090","100","110","120","SP"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 13, "13 items total");

    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    CHECK(cat.uap_variations.at("default").size() == 14, "UAP has 14 slots");

    const auto& uap = cat.uap_variations.at("default");
    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "000", "UAP slot  2 = 000");
    CHECK(uap[2]  == "020", "UAP slot  3 = 020");
    CHECK(uap[3]  == "036", "UAP slot  4 = 036");
    CHECK(uap[4]  == "034", "UAP slot  5 = 034");
    CHECK(uap[5]  == "040", "UAP slot  6 = 040");
    CHECK(uap[6]  == "050", "UAP slot  7 = 050");
    CHECK(uap[7]  == "090", "UAP slot  8 = 090");
    CHECK(uap[8]  == "100", "UAP slot  9 = 100");
    CHECK(uap[9]  == "110", "UAP slot 10 = 110");
    CHECK(uap[10] == "120", "UAP slot 11 = 120");
    CHECK(uap[11] == "038", "UAP slot 12 = 038");
    CHECK(uap[12] == "SP",  "UAP slot 13 = SP");
    CHECK(uap[13] == "-",   "UAP slot 14 = - (RFS, unused)");

    // Item types
    CHECK(cat.items.at("000").type == ItemType::Fixed,           "000 is Fixed");
    CHECK(cat.items.at("010").type == ItemType::Fixed,           "010 is Fixed");
    CHECK(cat.items.at("020").type == ItemType::Extended,        "020 is Extended");
    CHECK(cat.items.at("034").type == ItemType::RepetitiveGroup, "034 is RepetitiveGroup");
    CHECK(cat.items.at("036").type == ItemType::RepetitiveGroup, "036 is RepetitiveGroup");
    CHECK(cat.items.at("038").type == ItemType::RepetitiveGroup, "038 is RepetitiveGroup");
    CHECK(cat.items.at("040").type == ItemType::Fixed,           "040 is Fixed");
    CHECK(cat.items.at("050").type == ItemType::RepetitiveGroup, "050 is RepetitiveGroup");
    CHECK(cat.items.at("090").type == ItemType::Fixed,           "090 is Fixed");
    CHECK(cat.items.at("100").type == ItemType::Fixed,           "100 is Fixed");
    CHECK(cat.items.at("110").type == ItemType::Repetitive,      "110 is Repetitive");
    CHECK(cat.items.at("120").type == ItemType::Fixed,           "120 is Fixed");
    CHECK(cat.items.at("SP").type  == ItemType::SP,              "SP is SP/Explicit");

    // Fixed byte sizes
    CHECK(cat.items.at("000").fixed_bytes == 1, "000 = 1 byte");
    CHECK(cat.items.at("010").fixed_bytes == 2, "010 = 2 bytes");
    CHECK(cat.items.at("040").fixed_bytes == 2, "040 = 2 bytes");
    CHECK(cat.items.at("090").fixed_bytes == 3, "090 = 3 bytes");
    CHECK(cat.items.at("100").fixed_bytes == 3, "100 = 3 bytes");
    CHECK(cat.items.at("120").fixed_bytes == 2, "120 = 2 bytes");

    // Extended octet count for I020
    CHECK(cat.items.at("020").octets.size() == 2, "020 has 2 octets");

    // RepetitiveGroup element counts and bit widths
    const auto& i034 = cat.items.at("034");
    CHECK(i034.rep_group_bits == 32, "034 rep_group_bits = 32 (4 bytes)");
    CHECK(i034.rep_group_elements.size() == 3, "034 has 3 group elements");

    const auto& i036 = cat.items.at("036");
    CHECK(i036.rep_group_bits == 24, "036 rep_group_bits = 24 (3 bytes)");
    CHECK(i036.rep_group_elements.size() == 3, "036 has 3 group elements");

    const auto& i038 = cat.items.at("038");
    CHECK(i038.rep_group_bits == 32, "038 rep_group_bits = 32 (4 bytes)");
    CHECK(i038.rep_group_elements.size() == 4, "038 has 4 group elements");

    const auto& i050 = cat.items.at("050");
    CHECK(i050.rep_group_bits == 16, "050 rep_group_bits = 16 (2 bytes)");
    CHECK(i050.rep_group_elements.size() == 2, "050 has 2 group elements");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode basic SOP message
//          I010(SAC=2,SIC=21) + I000(MSGTYP=254/SOP) + I020(ORG=1,I=3,S=2)
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicSOP(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT08 SOP message ===\n";

    // FSPEC byte 1: bit7=1(I010), bit6=1(I000), bit5=1(I020), bits4-1=0, bit0=0(FX=0)
    //              = 1110_0000 = 0xE0
    //
    // I010: SAC=0x02, SIC=0x15
    // I000: MSGTYP=254=0xFE
    // I020: ORG=1, I=3(011b), S=2(010b), FX=0
    //   Octet 1 data bits: [1][0][1][1][0][1][0] = 1011010
    //   byte = 1011010_0 = 0xB4

    std::vector<uint8_t> frame = {
        0x08,             // CAT = 8
        0x00, 0x08,       // LEN = 8
        0xE0,             // FSPEC
        0x02, 0x15,       // I010 SAC=2, SIC=21
        0xFE,             // I000 MSGTYP=254 (SOP)
        0xB4              // I020 ORG=1,I=3,S=2  FX=0
    };

    hexdump(frame, "SOP input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid, "block is valid");
    CHECK(blk.records.size() == 1, "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"), "I010 present");
    CHECK(rec.items.count("000"), "I000 present");
    CHECK(rec.items.count("020"), "I020 present");

    CHECK(rec.items.at("010").fields.at("SAC") == 2,   "I010.SAC=2");
    CHECK(rec.items.at("010").fields.at("SIC") == 21,  "I010.SIC=21");
    CHECK(rec.items.at("000").fields.at("MSGTYP") == 254, "I000.MSGTYP=254 (SOP)");
    CHECK(rec.items.at("020").fields.at("ORG") == 1,   "I020.ORG=1");
    CHECK(rec.items.at("020").fields.at("I") == 3,     "I020.I=3");
    CHECK(rec.items.at("020").fields.at("S") == 2,     "I020.S=2");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip I020 Vector Qualifier (both octets)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripVectorQualifier(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I020 Vector Qualifier (2 octets) ===\n";

    // Octet 1: ORG=0, I=5(101b), S=4(100b)  FX=1
    //   data bits: [0][1][0][1][1][0][0] = 0101100, byte = 0101100_1 = 0x59
    // Octet 2: spare(5)=0, TST=1, ER=1  FX=0
    //   data bits: [0][0][0][0][0][1][1] = 0000011, byte = 0000011_0 = 0x06

    std::vector<uint8_t> frame = {
        0x08,             // CAT=8
        0x00, 0x06,       // LEN=6
        0x20,             // FSPEC: slot3(I020)=1, others=0 → 0010_0000=0x20, FX=0
        0x59, 0x06        // I020: 2 octets
    };

    hexdump(frame, "VQ input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& f = blk.records[0].items.at("020").fields;
    CHECK(f.at("ORG") == 0, "I020.ORG=0");
    CHECK(f.at("I")   == 5, "I020.I=5");
    CHECK(f.at("S")   == 4, "I020.S=4");
    CHECK(f.at("TST") == 1, "I020.TST=1");
    CHECK(f.at("ER")  == 1, "I020.ER=1");

    // Encode and re-decode
    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    enc_rec.items["020"] = blk.records[0].items.at("020");

    std::vector<uint8_t> encoded = codec.encode(8, {enc_rec});
    hexdump(encoded, "VQ encoded");

    DecodedBlock blk2 = codec.decode(encoded);
    CHECK(blk2.valid, "re-decode valid");
    if (!blk2.records.empty())
        checkItemsMatch(blk2.records[0].items, enc_rec.items, "RT-020");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip I034 Polar Vectors (2 entries)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripPolarVectors(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I034 Polar Vectors ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i034;
    // Entry 0: STR=10, ENDR=50, AZ=0x2000
    i034.group_repetitions.push_back({{"STR", 10}, {"ENDR", 50}, {"AZ", 0x2000}});
    // Entry 1: STR=60, ENDR=80, AZ=0x4000
    i034.group_repetitions.push_back({{"STR", 60}, {"ENDR", 80}, {"AZ", 0x4000}});
    enc_rec.items["034"] = i034;

    std::vector<uint8_t> encoded = codec.encode(8, {enc_rec});
    hexdump(encoded, "I034 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("034"), "I034 present");
    const auto& groups = blk.records[0].items.at("034").group_repetitions;
    CHECK(groups.size() == 2, "I034: 2 groups");
    if (groups.size() == 2) {
        CHECK(groups[0].at("STR")  == 10,     "I034[0].STR=10");
        CHECK(groups[0].at("ENDR") == 50,     "I034[0].ENDR=50");
        CHECK(groups[0].at("AZ")   == 0x2000, "I034[0].AZ=0x2000");
        CHECK(groups[1].at("STR")  == 60,     "I034[1].STR=60");
        CHECK(groups[1].at("ENDR") == 80,     "I034[1].ENDR=80");
        CHECK(groups[1].at("AZ")   == 0x4000, "I034[1].AZ=0x4000");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip I036 Cartesian Vectors (3 entries)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCartesianVectors(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I036 Cartesian Vectors ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i036;
    i036.group_repetitions.push_back({{"X", 50},  {"Y", 30},  {"LENGTH", 40}});
    i036.group_repetitions.push_back({{"X", 100}, {"Y", 200}, {"LENGTH", 15}});
    i036.group_repetitions.push_back({{"X", 10},  {"Y", 20},  {"LENGTH", 5}});
    enc_rec.items["036"] = i036;

    std::vector<uint8_t> encoded = codec.encode(8, {enc_rec});
    hexdump(encoded, "I036 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "RT-036");

    const auto& groups = blk.records[0].items.at("036").group_repetitions;
    CHECK(groups.size() == 3, "I036: 3 groups");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip I038 Weather Vectors start/end (2 entries)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripWeatherVectors(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I038 Weather Vectors ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i038;
    i038.group_repetitions.push_back({{"X1", 10}, {"Y1", 20}, {"X2", 110}, {"Y2", 120}});
    i038.group_repetitions.push_back({{"X1", 30}, {"Y1", 40}, {"X2", 130}, {"Y2", 140}});
    enc_rec.items["038"] = i038;

    std::vector<uint8_t> encoded = codec.encode(8, {enc_rec});
    hexdump(encoded, "I038 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "RT-038");
    const auto& groups = blk.records[0].items.at("038").group_repetitions;
    CHECK(groups.size() == 2, "I038: 2 groups");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip I040 Contour Identifier + I050 Contour Points
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripContour(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I040+I050 Contour ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    // I040: ORG=0, I=5, spare, FSTLST=3, CSN=42
    DecodedItem i040;
    i040.fields["ORG"]    = 0;
    i040.fields["I"]      = 5;
    i040.fields["FSTLST"] = 3;
    i040.fields["CSN"]    = 42;
    enc_rec.items["040"] = i040;

    // I050: 3 contour points
    DecodedItem i050;
    i050.group_repetitions.push_back({{"X1", 15}, {"Y1", 25}});
    i050.group_repetitions.push_back({{"X1", 35}, {"Y1", 45}});
    i050.group_repetitions.push_back({{"X1", 55}, {"Y1", 65}});
    enc_rec.items["050"] = i050;

    std::vector<uint8_t> encoded = codec.encode(8, {enc_rec});
    hexdump(encoded, "Contour encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "RT-contour");

    const auto& pts = blk.records[0].items.at("050").group_repetitions;
    CHECK(pts.size() == 3, "I050: 3 points");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Round-trip I100 Processing Status (signed F, raw R+Q)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripProcessingStatus(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I100 Processing Status ===\n";

    // F = -3 (5-bit signed), two's complement raw = 29 (11101b)
    // R = 5 (raw), Q = 0x5A5A (15-bit raw = 0101_1010_0101_1010)
    const uint64_t f_raw = static_cast<uint64_t>(static_cast<uint8_t>(static_cast<int8_t>(-3)) & 0x1F);

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    DecodedItem i100;
    i100.fields["F"] = f_raw;
    i100.fields["R"] = 5;
    i100.fields["Q"] = 0x5A5A;
    enc_rec.items["100"] = i100;

    std::vector<uint8_t> encoded = codec.encode(8, {enc_rec});
    hexdump(encoded, "I100 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("100"), "I100 present");
    CHECK(blk.records[0].items.at("100").fields.at("F") == f_raw,  "I100.F=-3 raw round-trip");
    CHECK(blk.records[0].items.at("100").fields.at("R") == 5,      "I100.R=5");
    CHECK(blk.records[0].items.at("100").fields.at("Q") == 0x5A5A, "I100.Q=0x5A5A");

    // Also test positive F = 7
    DecodedRecord enc_rec2;
    enc_rec2.uap_variation = "default";
    DecodedItem i100b;
    i100b.fields["F"] = 7;
    i100b.fields["R"] = 0;
    i100b.fields["Q"] = 0x7FFF;
    enc_rec2.items["100"] = i100b;

    std::vector<uint8_t> encoded2 = codec.encode(8, {enc_rec2});
    DecodedBlock blk2 = codec.decode(encoded2);
    CHECK(blk2.valid, "block2 valid");
    if (!blk2.records.empty()) {
        CHECK(blk2.records[0].items.at("100").fields.at("F") == 7,      "I100.F=7 positive");
        CHECK(blk2.records[0].items.at("100").fields.at("Q") == 0x7FFF, "I100.Q=0x7FFF");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 9: Round-trip I110 Station Configuration (Repetitive FX, 3 entries)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripStationConfig(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I110 Station Configuration Status ===\n";

    // 3 entries: SCS=0x55(85), SCS=0x2A(42), SCS=0x7F(127)
    // Each entry: 7 data bits + FX bit
    // FX=1 for first two, FX=0 for last

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    DecodedItem i110;
    i110.repetitions = {85, 42, 127};
    enc_rec.items["110"] = i110;

    std::vector<uint8_t> encoded = codec.encode(8, {enc_rec});
    hexdump(encoded, "I110 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("110"), "I110 present");
    const auto& reps = blk.records[0].items.at("110").repetitions;
    CHECK(reps.size() == 3,   "I110: 3 repetitions");
    if (reps.size() == 3) {
        CHECK(reps[0] == 85,  "I110[0]=85");
        CHECK(reps[1] == 42,  "I110[1]=42");
        CHECK(reps[2] == 127, "I110[2]=127");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 10: Full round-trip – multiple items
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip CAT08 ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    // I010
    {
        DecodedItem it;
        it.fields["SAC"] = 3;
        it.fields["SIC"] = 99;
        enc_rec.items["010"] = it;
    }
    // I000 – Polar vector
    {
        DecodedItem it;
        it.fields["MSGTYP"] = 1;
        enc_rec.items["000"] = it;
    }
    // I020 – ORG=1, I=4, S=0 (octet 1 only)
    {
        DecodedItem it;
        it.fields["ORG"] = 1;
        it.fields["I"]   = 4;
        it.fields["S"]   = 0;
        enc_rec.items["020"] = it;
    }
    // I034 – 2 polar vectors
    {
        DecodedItem it;
        it.group_repetitions.push_back({{"STR", 20}, {"ENDR", 100}, {"AZ", 0x1800}});
        it.group_repetitions.push_back({{"STR", 110}, {"ENDR", 200}, {"AZ", 0x3000}});
        enc_rec.items["034"] = it;
    }
    // I090 – Time of day 0x400000 = 4194304 * 0.0078125 = 32768.0 s
    {
        DecodedItem it;
        it.fields["TOD"] = 0x400000;
        enc_rec.items["090"] = it;
    }
    // I100 – Processing status
    {
        DecodedItem it;
        it.fields["F"] = 2;   // +2
        it.fields["R"] = 1;
        it.fields["Q"] = 0x1234;
        enc_rec.items["100"] = it;
    }
    // I120 – Total item count
    {
        DecodedItem it;
        it.fields["TOTAL"] = 512;
        enc_rec.items["120"] = it;
    }

    std::vector<uint8_t> encoded = codec.encode(8, {enc_rec});
    hexdump(encoded, "Full RT encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "full-RT");
    CHECK(blk.records[0].items.count("010"), "I010 (mandatory) present");
}

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = "specs/CAT08.xml";
    if (argc >= 2)
        spec_path = argv[1];

    if (!fs::exists(spec_path)) {
        std::cerr << "Spec not found: " << spec_path << '\n';
        return 1;
    }

    Codec codec;

    testSpecLoad(codec, spec_path);
    testDecodeBasicSOP(codec);
    testRoundTripVectorQualifier(codec);
    testRoundTripPolarVectors(codec);
    testRoundTripCartesianVectors(codec);
    testRoundTripWeatherVectors(codec);
    testRoundTripContour(codec);
    testRoundTripProcessingStatus(codec);
    testRoundTripStationConfig(codec);
    testFullRoundTrip(codec);

    std::cout << "\n============================\n";
    if (failures == 0)
        std::cout << "ALL TESTS PASSED\n";
    else
        std::cout << failures << " TEST(S) FAILED\n";
    std::cout << "============================\n";

    return failures ? 1 : 0;
}
