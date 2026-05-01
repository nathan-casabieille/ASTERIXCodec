// test_cat17.cpp – Tests for CAT17 Mode S Surveillance Coordination Function Messages.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat17

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
//  Test 1: XML spec loads, item types and UAP are correct
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT17 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 17,         "cat number = 17");
    CHECK(cat.edition == "1.3",  "edition = 1.3");

    for (auto id : {"000","010","012","045","050","070","140","200",
                    "210","220","221","230","240","350","360","SP"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 16, "16 items total");

    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    CHECK(cat.uap_variations.at("default").size() == 21, "UAP has 21 slots");

    const auto& uap = cat.uap_variations.at("default");
    // Byte 1: 010 012 000 350 220 221 140
    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "012", "UAP slot  2 = 012");
    CHECK(uap[2]  == "000", "UAP slot  3 = 000");
    CHECK(uap[3]  == "350", "UAP slot  4 = 350");
    CHECK(uap[4]  == "220", "UAP slot  5 = 220");
    CHECK(uap[5]  == "221", "UAP slot  6 = 221");
    CHECK(uap[6]  == "140", "UAP slot  7 = 140");
    // Byte 2: 045 070 050 200 230 240 210
    CHECK(uap[7]  == "045", "UAP slot  8 = 045");
    CHECK(uap[8]  == "070", "UAP slot  9 = 070");
    CHECK(uap[9]  == "050", "UAP slot 10 = 050");
    CHECK(uap[10] == "200", "UAP slot 11 = 200");
    CHECK(uap[11] == "230", "UAP slot 12 = 230");
    CHECK(uap[12] == "240", "UAP slot 13 = 240");
    CHECK(uap[13] == "210", "UAP slot 14 = 210");
    // Byte 3: 360 - - - - - SP
    CHECK(uap[14] == "360", "UAP slot 15 = 360");
    CHECK(uap[15] == "-",   "UAP slot 16 = - (unused)");
    CHECK(uap[20] == "SP",  "UAP slot 21 = SP");

    // Item types
    CHECK(cat.items.at("000").type == ItemType::Fixed,           "000 is Fixed");
    CHECK(cat.items.at("010").type == ItemType::Fixed,           "010 is Fixed");
    CHECK(cat.items.at("210").type == ItemType::RepetitiveGroup, "210 is RepetitiveGroup");
    CHECK(cat.items.at("350").type == ItemType::RepetitiveGroup, "350 is RepetitiveGroup");
    CHECK(cat.items.at("SP").type  == ItemType::SP,              "SP is SP/Explicit");

    // Fixed byte sizes
    CHECK(cat.items.at("000").fixed_bytes == 1, "000 = 1 byte");
    CHECK(cat.items.at("010").fixed_bytes == 2, "010 = 2 bytes");
    CHECK(cat.items.at("012").fixed_bytes == 2, "012 = 2 bytes");
    CHECK(cat.items.at("045").fixed_bytes == 6, "045 = 6 bytes");
    CHECK(cat.items.at("050").fixed_bytes == 2, "050 = 2 bytes");
    CHECK(cat.items.at("070").fixed_bytes == 2, "070 = 2 bytes");
    CHECK(cat.items.at("140").fixed_bytes == 3, "140 = 3 bytes");
    CHECK(cat.items.at("200").fixed_bytes == 4, "200 = 4 bytes");
    CHECK(cat.items.at("220").fixed_bytes == 3, "220 = 3 bytes");
    CHECK(cat.items.at("221").fixed_bytes == 2, "221 = 2 bytes");
    CHECK(cat.items.at("230").fixed_bytes == 1, "230 = 1 byte");
    CHECK(cat.items.at("240").fixed_bytes == 1, "240 = 1 byte");
    CHECK(cat.items.at("360").fixed_bytes == 1, "360 = 1 byte");

    // I210 RepetitiveGroup: 24-bit ADR per entry
    const auto& i210 = cat.items.at("210");
    CHECK(i210.rep_group_bits == 24,             "210 rep_group_bits = 24 (3 bytes)");
    CHECK(i210.rep_group_elements.size() == 1,   "210 has 1 group element");
    CHECK(i210.rep_group_elements[0].name == "ADR", "210 element[0] = ADR");

    // I350 RepetitiveGroup: SAC(8)+SIC(8) = 16 bits per entry
    const auto& i350 = cat.items.at("350");
    CHECK(i350.rep_group_bits == 16,             "350 rep_group_bits = 16 (2 bytes)");
    CHECK(i350.rep_group_elements.size() == 2,   "350 has 2 group elements");
    CHECK(i350.rep_group_elements[0].name == "SAC", "350 element[0] = SAC");
    CHECK(i350.rep_group_elements[1].name == "SIC", "350 element[1] = SIC");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode basic SCN message
//          I010(SAC=1,SIC=2) + I000(MSGTYP=10 "Track data")
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicMessage(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT17 SCN message ===\n";

    // UAP byte 1: 010(b7) 012(b6) 000(b5) 350(b4) 220(b3) 221(b2) 140(b1) FX(b0)
    // I010+I000 → bits 7,5 set → 1010_0000 = 0xA0
    // I010: SAC=0x01 SIC=0x02; I000: MSGTYP=10=0x0A
    // Total: 3(header) + 1(FSPEC) + 2(I010) + 1(I000) = 7 bytes

    std::vector<uint8_t> frame = {
        0x11,             // CAT = 17
        0x00, 0x07,       // LEN = 7
        0xA0,             // FSPEC byte 1 (FX=0): I010+I000
        0x01, 0x02,       // I010 SAC=1, SIC=2
        0x0A              // I000 MSGTYP=10 (Track data)
    };

    hexdump(frame, "Basic SCN msg input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid, "block is valid");
    CHECK(blk.records.size() == 1, "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"), "I010 present");
    CHECK(rec.items.count("000"), "I000 present");
    CHECK(!rec.items.count("012"), "I012 absent");

    CHECK(rec.items.at("010").fields.at("SAC")    == 1,  "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC")    == 2,  "I010.SIC=2");
    CHECK(rec.items.at("000").fields.at("MSGTYP") == 10, "I000.MSGTYP=10");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip simple Fixed items
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripFixedItems(Codec& codec) {
    std::cout << "\n=== Test: Round-trip simple Fixed items ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=3; it.fields["SIC"]=7; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=10; enc_rec.items["000"]=it; }       // Track data
    { DecodedItem it; it.fields["SAC"]=4; it.fields["SIC"]=8; enc_rec.items["012"]=it; }
    { DecodedItem it; it.fields["TOD"]=0x800000; enc_rec.items["140"]=it; }    // ~65536 s
    { DecodedItem it; it.fields["ADR"]=0xABCDEF; enc_rec.items["220"]=it; }
    { DecodedItem it; it.fields["DRN"]=0x1234; enc_rec.items["221"]=it; }
    { DecodedItem it; it.fields["CA"]=5; it.fields["SI"]=0; enc_rec.items["230"]=it; } // airborne
    { DecodedItem it; it.fields["CST"]=1; it.fields["FLT"]=1; enc_rec.items["240"]=it; }
    { DecodedItem it; it.fields["STATE"]=0xC3; enc_rec.items["360"]=it; }

    std::vector<uint8_t> encoded = codec.encode(17, {enc_rec});
    hexdump(encoded, "Fixed items encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "RT-fixed");

    CHECK(blk.records[0].items.at("220").fields.at("ADR")   == 0xABCDEFu, "I220.ADR=0xABCDEF");
    CHECK(blk.records[0].items.at("240").fields.at("CST")   == 1,          "I240.CST=1");
    CHECK(blk.records[0].items.at("240").fields.at("FLT")   == 1,          "I240.FLT=1");
    CHECK(blk.records[0].items.at("360").fields.at("STATE") == 0xC3u,      "I360.STATE=0xC3");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip I045 WGS-84 position (signed LAT/LON)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripWGS84(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I045 WGS-84 position ===\n";

    // Positive LAT (e.g. ~44°N): raw = 0x700000 (7340032)
    // Negative LON (e.g. ~5°W): raw for -5° = -5 / (180/2^25) ≈ -931040
    //   24-bit two's complement of -931040: 2^24 - 931040 = 16777216 - 931040 = 15846176 = 0xF20DE0

    const uint64_t lat_raw = 0x700000;
    const uint64_t lon_raw = static_cast<uint64_t>(static_cast<uint32_t>(static_cast<int32_t>(-931040)) & 0xFFFFFF);

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i045;
    i045.fields["LAT"] = lat_raw;
    i045.fields["LON"] = lon_raw;
    enc_rec.items["045"] = i045;

    std::vector<uint8_t> encoded = codec.encode(17, {enc_rec});
    hexdump(encoded, "I045 WGS-84 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("045"), "I045 present");
    const auto& f = blk.records[0].items.at("045").fields;
    CHECK(f.at("LAT") == lat_raw, "I045.LAT round-trip");
    CHECK(f.at("LON") == lon_raw, "I045.LON round-trip (negative)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip I050 flight level and I070 Mode 3/A
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripFlightLevel(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I050 flight level + I070 Mode 3/A ===\n";

    // I050: FL100 = 100 / 0.25 = 400 raw; V=0, G=0
    // I070: V=0, G=0, L=0, spare=0, MODE3A=0o1234 = 0b001010011100 = 0x29C = 668

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["V"]=0; it.fields["G"]=0; it.fields["ALT"]=400; enc_rec.items["050"]=it; }
    { DecodedItem it; it.fields["V"]=0; it.fields["G"]=0; it.fields["L"]=0; it.fields["MODE3A"]=0x29C; enc_rec.items["070"]=it; }

    std::vector<uint8_t> encoded = codec.encode(17, {enc_rec});
    hexdump(encoded, "FL+Mode3A encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "RT-FL");

    CHECK(blk.records[0].items.at("050").fields.at("ALT")    == 400,   "I050.ALT=400 raw (FL100)");
    CHECK(blk.records[0].items.at("070").fields.at("MODE3A") == 0x29Cu,"I070.MODE3A=0o1234");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip I200 track velocity (GSP + HDG)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripTrackVelocity(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I200 track velocity ===\n";

    // GSP = 250 kt = 250/3600 NM/s → raw = (250/3600) / (1/16384) = 250*16384/3600 ≈ 1138
    // HDG = 270° → raw = 270 / (360/65536) = 270*65536/360 = 49152 = 0xC000

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i200;
    i200.fields["GSP"] = 1138;
    i200.fields["HDG"] = 49152;
    enc_rec.items["200"] = i200;

    std::vector<uint8_t> encoded = codec.encode(17, {enc_rec});
    hexdump(encoded, "Track velocity encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("200"), "I200 present");
    const auto& f = blk.records[0].items.at("200").fields;
    CHECK(f.at("GSP") == 1138,  "I200.GSP=1138 raw");
    CHECK(f.at("HDG") == 49152, "I200.HDG=49152 raw (270°)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip I210 Mode S Address List (3 addresses)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripModeSAddressList(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I210 Mode S Address List ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i210;
    i210.group_repetitions.push_back({{"ADR", 0x4840D6}});
    i210.group_repetitions.push_back({{"ADR", 0x3C4AB0}});
    i210.group_repetitions.push_back({{"ADR", 0x000000}});
    enc_rec.items["210"] = i210;

    std::vector<uint8_t> encoded = codec.encode(17, {enc_rec});
    hexdump(encoded, "I210 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("210"), "I210 present");
    const auto& groups = blk.records[0].items.at("210").group_repetitions;
    CHECK(groups.size() == 3, "I210: 3 groups");

    if (groups.size() == 3) {
        CHECK(groups[0].at("ADR") == 0x4840D6u, "I210[0].ADR=0x4840D6");
        CHECK(groups[1].at("ADR") == 0x3C4AB0u, "I210[1].ADR=0x3C4AB0");
        CHECK(groups[2].at("ADR") == 0x000000u, "I210[2].ADR=0x000000");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Round-trip I350 Cluster Station/Node List (SAC/SIC pairs)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripClusterList(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I350 Cluster Station/Node List ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i350;
    i350.group_repetitions.push_back({{"SAC", 1}, {"SIC", 10}});
    i350.group_repetitions.push_back({{"SAC", 2}, {"SIC", 20}});
    i350.group_repetitions.push_back({{"SAC", 3}, {"SIC", 30}});
    enc_rec.items["350"] = i350;

    std::vector<uint8_t> encoded = codec.encode(17, {enc_rec});
    hexdump(encoded, "I350 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("350"), "I350 present");
    const auto& groups = blk.records[0].items.at("350").group_repetitions;
    CHECK(groups.size() == 3, "I350: 3 groups");

    if (groups.size() == 3) {
        CHECK(groups[0].at("SAC") ==  1, "I350[0].SAC=1");
        CHECK(groups[0].at("SIC") == 10, "I350[0].SIC=10");
        CHECK(groups[1].at("SAC") ==  2, "I350[1].SAC=2");
        CHECK(groups[1].at("SIC") == 20, "I350[1].SIC=20");
        CHECK(groups[2].at("SAC") ==  3, "I350[2].SAC=3");
        CHECK(groups[2].at("SIC") == 30, "I350[2].SIC=30");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 9: Full round-trip with all major item types
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip CAT17 ===\n";

    const uint64_t lat_raw = 0x500000;
    const uint64_t lon_raw = static_cast<uint64_t>(
        static_cast<uint32_t>(static_cast<int32_t>(-500000)) & 0xFFFFFF);

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["SAC"]=9; it.fields["SIC"]=5; enc_rec.items["012"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=10; enc_rec.items["000"]=it; }
    { DecodedItem it; it.fields["TOD"]=0x400000; enc_rec.items["140"]=it; }
    { DecodedItem it; it.fields["LAT"]=lat_raw; it.fields["LON"]=lon_raw; enc_rec.items["045"]=it; }
    { DecodedItem it; it.fields["V"]=0; it.fields["G"]=0; it.fields["ALT"]=800; enc_rec.items["050"]=it; } // FL200
    { DecodedItem it; it.fields["V"]=0; it.fields["G"]=0; it.fields["L"]=0; it.fields["MODE3A"]=0x7FF; enc_rec.items["070"]=it; }
    { DecodedItem it; it.fields["GSP"]=2000; it.fields["HDG"]=32768; enc_rec.items["200"]=it; } // HDG=180°
    { DecodedItem it; it.fields["CA"]=5; it.fields["SI"]=0; enc_rec.items["230"]=it; }
    { DecodedItem it; it.fields["CST"]=0; it.fields["FLT"]=0; enc_rec.items["240"]=it; }
    { DecodedItem it; it.fields["ADR"]=0x4840D6; enc_rec.items["220"]=it; }
    {
        DecodedItem it;
        it.group_repetitions.push_back({{"ADR", 0x4840D6}});
        it.group_repetitions.push_back({{"ADR", 0xABCDEF}});
        enc_rec.items["210"] = it;
    }
    {
        DecodedItem it;
        it.group_repetitions.push_back({{"SAC", 1}, {"SIC", 10}});
        it.group_repetitions.push_back({{"SAC", 2}, {"SIC", 20}});
        enc_rec.items["350"] = it;
    }
    { DecodedItem it; it.fields["STATE"]=0x01; enc_rec.items["360"]=it; }

    std::vector<uint8_t> encoded = codec.encode(17, {enc_rec});
    hexdump(encoded, "Full RT encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "full-RT");
    CHECK(blk.records[0].items.count("010"), "I010 (mandatory) present");
}

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = "specs/CAT17.xml";
    if (argc >= 2)
        spec_path = argv[1];

    if (!fs::exists(spec_path)) {
        std::cerr << "Spec not found: " << spec_path << '\n';
        return 1;
    }

    Codec codec;

    testSpecLoad(codec, spec_path);
    testDecodeBasicMessage(codec);
    testRoundTripFixedItems(codec);
    testRoundTripWGS84(codec);
    testRoundTripFlightLevel(codec);
    testRoundTripTrackVelocity(codec);
    testRoundTripModeSAddressList(codec);
    testRoundTripClusterList(codec);
    testFullRoundTrip(codec);

    std::cout << "\n============================\n";
    if (failures == 0)
        std::cout << "ALL TESTS PASSED\n";
    else
        std::cout << failures << " TEST(S) FAILED\n";
    std::cout << "============================\n";

    return failures ? 1 : 0;
}
