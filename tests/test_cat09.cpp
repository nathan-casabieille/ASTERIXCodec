// test_cat09.cpp – Tests for CAT09 Composite Weather Reports decode/encode round-trip.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat09

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

// ─── Deep comparison helper ──────────────────────────────────────────────────
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
            std::cerr << "FAIL [RT] " << p << " repetitions count mismatch\n";
            ++failures;
        } else {
            for (size_t i = 0; i < src.repetitions.size(); ++i)
                CHECK(dst.repetitions[i] == src.repetitions[i],
                      p + ".rep[" + std::to_string(i) + "]");
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
    std::cout << "\n=== Test: CAT09 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 9,          "cat number = 9");
    CHECK(cat.edition == "2.1",  "edition = 2.1");

    for (auto id : {"000","010","020","030","060","070","080","090","100"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 9, "9 items total");

    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    CHECK(cat.uap_variations.at("default").size() == 14, "UAP has 14 slots");

    const auto& uap = cat.uap_variations.at("default");
    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "000", "UAP slot  2 = 000");
    CHECK(uap[2]  == "020", "UAP slot  3 = 020");
    CHECK(uap[3]  == "030", "UAP slot  4 = 030");
    CHECK(uap[4]  == "060", "UAP slot  5 = 060");
    CHECK(uap[5]  == "070", "UAP slot  6 = 070");
    CHECK(uap[6]  == "080", "UAP slot  7 = 080");
    CHECK(uap[7]  == "090", "UAP slot  8 = 090");
    CHECK(uap[8]  == "100", "UAP slot  9 = 100");
    CHECK(uap[9]  == "-",   "UAP slot 10 = - (unused)");
    CHECK(uap[13] == "-",   "UAP slot 14 = - (unused)");

    // Item types
    CHECK(cat.items.at("000").type == ItemType::Fixed,           "000 is Fixed");
    CHECK(cat.items.at("010").type == ItemType::Fixed,           "010 is Fixed");
    CHECK(cat.items.at("020").type == ItemType::Extended,        "020 is Extended");
    CHECK(cat.items.at("030").type == ItemType::RepetitiveGroup, "030 is RepetitiveGroup");
    CHECK(cat.items.at("060").type == ItemType::Extended,        "060 is Extended");
    CHECK(cat.items.at("070").type == ItemType::Fixed,           "070 is Fixed");
    CHECK(cat.items.at("080").type == ItemType::Fixed,           "080 is Fixed");
    CHECK(cat.items.at("090").type == ItemType::RepetitiveGroup, "090 is RepetitiveGroup");
    CHECK(cat.items.at("100").type == ItemType::Fixed,           "100 is Fixed");

    // Fixed byte sizes
    CHECK(cat.items.at("000").fixed_bytes == 1, "000 = 1 byte");
    CHECK(cat.items.at("010").fixed_bytes == 2, "010 = 2 bytes");
    CHECK(cat.items.at("070").fixed_bytes == 3, "070 = 3 bytes");
    CHECK(cat.items.at("080").fixed_bytes == 3, "080 = 3 bytes");
    CHECK(cat.items.at("100").fixed_bytes == 2, "100 = 2 bytes");

    // Extended octet counts
    CHECK(cat.items.at("020").octets.size() == 1, "020 has 1 octet");
    CHECK(cat.items.at("060").octets.size() == 1, "060 has 1 octet");

    // RepetitiveGroup details
    const auto& i030 = cat.items.at("030");
    CHECK(i030.rep_group_bits == 48, "030 rep_group_bits = 48 (6 bytes)");
    CHECK(i030.rep_group_elements.size() == 3, "030 has 3 group elements");
    CHECK(i030.rep_group_elements[0].name == "X", "030 element[0] = X");
    CHECK(i030.rep_group_elements[1].name == "Y", "030 element[1] = Y");
    CHECK(i030.rep_group_elements[2].name == "L", "030 element[2] = L");

    const auto& i090 = cat.items.at("090");
    // 8+8+3+1+1+3 = 24 bits; spare counts toward rep_group_bits
    CHECK(i090.rep_group_bits == 24, "090 rep_group_bits = 24 (3 bytes)");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode basic SOP message
//          I010(SAC=1,SIC=5) + I000(MSGTYP=254) + I020(ORG=1,I=4,S=0)
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicSOP(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT09 SOP message ===\n";

    // FSPEC byte 1: bit7=1(I010), bit6=1(I000), bit5=1(I020), rest=0, FX=0
    //              = 1110_0000 = 0xE0
    //
    // I010: SAC=0x01, SIC=0x05
    // I000: MSGTYP=254=0xFE
    // I020: ORG=1, I=4(100b), S=0(000b), FX=0
    //   data bits: [1][1][0][0][0][0][0] = 1100000, byte = 1100000_0 = 0xC0

    std::vector<uint8_t> frame = {
        0x09,             // CAT = 9
        0x00, 0x08,       // LEN = 8
        0xE0,             // FSPEC
        0x01, 0x05,       // I010 SAC=1, SIC=5
        0xFE,             // I000 MSGTYP=254 (SOP)
        0xC0              // I020 ORG=1,I=4,S=0  FX=0
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

    CHECK(rec.items.at("010").fields.at("SAC") == 1,   "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC") == 5,   "I010.SIC=5");
    CHECK(rec.items.at("000").fields.at("MSGTYP") == 254, "I000.MSGTYP=254 (SOP)");
    CHECK(rec.items.at("020").fields.at("ORG") == 1,   "I020.ORG=1");
    CHECK(rec.items.at("020").fields.at("I")   == 4,   "I020.I=4");
    CHECK(rec.items.at("020").fields.at("S")   == 0,   "I020.S=0");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip I020 Vector Qualifier (1-octet Extended)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripVectorQualifier(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I020 Vector Qualifier ===\n";

    // ORG=0, I=7(111b), S=5(101b)
    // data bits: [0][1][1][1][1][0][1] = 0111101b, FX=0 → byte=0111101_0=0x7A

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    DecodedItem i020;
    i020.fields["ORG"] = 0;
    i020.fields["I"]   = 7;
    i020.fields["S"]   = 5;
    enc_rec.items["020"] = i020;

    std::vector<uint8_t> encoded = codec.encode(9, {enc_rec});
    hexdump(encoded, "I020 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "RT-020");

    // Verify: only 1 octet emitted for I020 (FSPEC=1B + I020=1B = 2B item payload)
    // Full frame: CAT(1)+LEN(2)+FSPEC(1)+I020(1) = 5 bytes
    CHECK(encoded.size() == 5, "I020 encoded as 1 octet");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip I060 Synchronisation/Control Signal (1-octet Extended)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripSyncControl(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I060 Sync/Control Signal ===\n";

    // SN=42(101010b), spare=0 → data bits: [1][0][1][0][1][0][0] = 1010100b, FX=0
    // byte = 1010100_0 = 0xA8
    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    DecodedItem i060;
    i060.fields["SN"] = 42;
    enc_rec.items["060"] = i060;

    std::vector<uint8_t> encoded = codec.encode(9, {enc_rec});
    hexdump(encoded, "I060 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("060"), "I060 present");
    CHECK(blk.records[0].items.at("060").fields.at("SN") == 42, "I060.SN=42");

    // Test boundary: SN=0 and SN=63 (max 6-bit)
    for (uint64_t sn : {uint64_t{0}, uint64_t{63}}) {
        DecodedRecord r;
        r.uap_variation = "default";
        DecodedItem it;
        it.fields["SN"] = sn;
        r.items["060"] = it;
        auto enc = codec.encode(9, {r});
        auto blk2 = codec.decode(enc);
        CHECK(blk2.valid && !blk2.records.empty() &&
              blk2.records[0].items.at("060").fields.at("SN") == sn,
              "I060.SN=" + std::to_string(sn) + " round-trip");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip I030 Cartesian Vectors (3 entries, 6 bytes each)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCartesianVectors(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I030 Cartesian Vectors ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i030;
    // Raw 16-bit values (signed X,Y stored as raw unsigned bit patterns)
    i030.group_repetitions.push_back({{"X", 100},  {"Y", 200},  {"L", 300}});
    i030.group_repetitions.push_back({{"X", 500},  {"Y", 1000}, {"L", 2000}});
    // Negative X: -50 as uint16 two's complement = 65486
    const uint64_t x_neg = static_cast<uint64_t>(static_cast<uint16_t>(static_cast<int16_t>(-50)));
    i030.group_repetitions.push_back({{"X", x_neg}, {"Y", 75}, {"L", 150}});
    enc_rec.items["030"] = i030;

    std::vector<uint8_t> encoded = codec.encode(9, {enc_rec});
    hexdump(encoded, "I030 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("030"), "I030 present");
    const auto& groups = blk.records[0].items.at("030").group_repetitions;
    CHECK(groups.size() == 3, "I030: 3 groups");

    if (groups.size() == 3) {
        CHECK(groups[0].at("X") == 100,   "I030[0].X=100");
        CHECK(groups[0].at("Y") == 200,   "I030[0].Y=200");
        CHECK(groups[0].at("L") == 300,   "I030[0].L=300");
        CHECK(groups[1].at("X") == 500,   "I030[1].X=500");
        CHECK(groups[1].at("Y") == 1000,  "I030[1].Y=1000");
        CHECK(groups[1].at("L") == 2000,  "I030[1].L=2000");
        CHECK(groups[2].at("X") == x_neg, "I030[2].X=-50 raw");
        CHECK(groups[2].at("Y") == 75,    "I030[2].Y=75");
        CHECK(groups[2].at("L") == 150,   "I030[2].L=150");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip I080 Processing Status (signed F, raw R+Q)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripProcessingStatus(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I080 Processing Status ===\n";

    // F = -2 (5-bit signed raw = 30 = 11110b), R=3, Q=0x1234
    const uint64_t f_neg = static_cast<uint64_t>(static_cast<uint8_t>(static_cast<int8_t>(-2)) & 0x1F);

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    DecodedItem i080;
    i080.fields["F"] = f_neg;
    i080.fields["R"] = 3;
    i080.fields["Q"] = 0x1234;
    enc_rec.items["080"] = i080;

    std::vector<uint8_t> encoded = codec.encode(9, {enc_rec});
    hexdump(encoded, "I080 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("080"), "I080 present");
    CHECK(blk.records[0].items.at("080").fields.at("F") == f_neg,  "I080.F=-2 raw");
    CHECK(blk.records[0].items.at("080").fields.at("R") == 3,      "I080.R=3");
    CHECK(blk.records[0].items.at("080").fields.at("Q") == 0x1234, "I080.Q=0x1234");

    // Positive F = 15 (max 4-bit positive in 5-bit signed)
    DecodedRecord enc2;
    enc2.uap_variation = "default";
    DecodedItem i080b;
    i080b.fields["F"] = 15;
    i080b.fields["R"] = 0;
    i080b.fields["Q"] = 0x7FFF;
    enc2.items["080"] = i080b;

    auto enc2_bytes = codec.encode(9, {enc2});
    auto blk2 = codec.decode(enc2_bytes);
    CHECK(blk2.valid, "block2 valid");
    if (!blk2.records.empty()) {
        CHECK(blk2.records[0].items.at("080").fields.at("F") == 15,     "I080.F=15");
        CHECK(blk2.records[0].items.at("080").fields.at("Q") == 0x7FFF, "I080.Q=0x7FFF");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip I090 Radar Configuration and Status
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripRadarConfig(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I090 Radar Configuration ===\n";

    // Group: SAC(8)+SIC(8)+spare(3)+CP(1)+WO(1)+R(3)
    // spare bits are written as zero; on decode they are skipped.

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i090;
    // Entry 0: SAC=10, SIC=20, CP=1, WO=0, R=2
    i090.group_repetitions.push_back({{"SAC", 10}, {"SIC", 20}, {"CP", 1}, {"WO", 0}, {"R", 2}});
    // Entry 1: SAC=30, SIC=40, CP=0, WO=1, R=5
    i090.group_repetitions.push_back({{"SAC", 30}, {"SIC", 40}, {"CP", 0}, {"WO", 1}, {"R", 5}});
    enc_rec.items["090"] = i090;

    std::vector<uint8_t> encoded = codec.encode(9, {enc_rec});
    hexdump(encoded, "I090 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("090"), "I090 present");
    const auto& groups = blk.records[0].items.at("090").group_repetitions;
    CHECK(groups.size() == 2, "I090: 2 groups");

    if (groups.size() == 2) {
        CHECK(groups[0].at("SAC") == 10, "I090[0].SAC=10");
        CHECK(groups[0].at("SIC") == 20, "I090[0].SIC=20");
        CHECK(groups[0].at("CP")  == 1,  "I090[0].CP=1");
        CHECK(groups[0].at("WO")  == 0,  "I090[0].WO=0");
        CHECK(groups[0].at("R")   == 2,  "I090[0].R=2");
        CHECK(groups[1].at("SAC") == 30, "I090[1].SAC=30");
        CHECK(groups[1].at("SIC") == 40, "I090[1].SIC=40");
        CHECK(groups[1].at("CP")  == 0,  "I090[1].CP=0");
        CHECK(groups[1].at("WO")  == 1,  "I090[1].WO=1");
        CHECK(groups[1].at("R")   == 5,  "I090[1].R=5");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Round-trip I070 Time of Day
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripTimeOfDay(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I070 Time of Day ===\n";

    // TOD = 0x300000 = 3145728 raw, physical = 3145728 * 0.0078125 = 24576.0 s
    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    DecodedItem i070;
    i070.fields["TOD"] = 0x300000;
    enc_rec.items["070"] = i070;

    std::vector<uint8_t> encoded = codec.encode(9, {enc_rec});
    hexdump(encoded, "I070 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("070"), "I070 present");
    CHECK(blk.records[0].items.at("070").fields.at("TOD") == 0x300000u, "I070.TOD round-trip");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 9: Full round-trip – all items together
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip CAT09 ===\n";

    const uint64_t f_raw = static_cast<uint64_t>(static_cast<uint8_t>(static_cast<int8_t>(-1)) & 0x1F);

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    // I010
    { DecodedItem it; it.fields["SAC"]=5; it.fields["SIC"]=50; enc_rec.items["010"]=it; }
    // I000
    { DecodedItem it; it.fields["MSGTYP"]=2; enc_rec.items["000"]=it; }
    // I020 – ORG=1, I=2, S=3
    { DecodedItem it; it.fields["ORG"]=1; it.fields["I"]=2; it.fields["S"]=3; enc_rec.items["020"]=it; }
    // I030 – 2 vectors
    {
        DecodedItem it;
        it.group_repetitions.push_back({{"X", 256}, {"Y", 512}, {"L", 1024}});
        it.group_repetitions.push_back({{"X", 10},  {"Y", 20},  {"L", 30}});
        enc_rec.items["030"] = it;
    }
    // I060 – SN=15
    { DecodedItem it; it.fields["SN"]=15; enc_rec.items["060"]=it; }
    // I070 – TOD
    { DecodedItem it; it.fields["TOD"]=0x200000; enc_rec.items["070"]=it; }
    // I080 – Processing status
    { DecodedItem it; it.fields["F"]=f_raw; it.fields["R"]=1; it.fields["Q"]=0x7A7A; enc_rec.items["080"]=it; }
    // I090 – 1 radar
    {
        DecodedItem it;
        it.group_repetitions.push_back({{"SAC", 7}, {"SIC", 77}, {"CP", 1}, {"WO", 1}, {"R", 3}});
        enc_rec.items["090"] = it;
    }
    // I100 – vector count
    { DecodedItem it; it.fields["TOTAL"]=1024; enc_rec.items["100"]=it; }

    std::vector<uint8_t> encoded = codec.encode(9, {enc_rec});
    hexdump(encoded, "Full RT encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "full-RT");
    CHECK(blk.records[0].items.count("010"), "I010 (mandatory) present");
}

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = "specs/CAT09.xml";
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
    testRoundTripSyncControl(codec);
    testRoundTripCartesianVectors(codec);
    testRoundTripProcessingStatus(codec);
    testRoundTripRadarConfig(codec);
    testRoundTripTimeOfDay(codec);
    testFullRoundTrip(codec);

    std::cout << "\n============================\n";
    if (failures == 0)
        std::cout << "ALL TESTS PASSED\n";
    else
        std::cout << failures << " TEST(S) FAILED\n";
    std::cout << "============================\n";

    return failures ? 1 : 0;
}
