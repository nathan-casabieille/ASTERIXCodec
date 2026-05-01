// test_cat19.cpp – Tests for CAT19 Multilateration System Status Messages decode/encode.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat19

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
//  Test 1: XML spec loads correctly
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT19 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 19,         "cat number = 19");
    CHECK(cat.edition == "1.3",  "edition = 1.3");

    for (auto id : {"000","010","140","550","551","552","553","600","610","620","RE","SP"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 12, "12 items total");

    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    CHECK(cat.uap_variations.at("default").size() == 14, "UAP has 14 slots");

    const auto& uap = cat.uap_variations.at("default");
    // Byte 1: 010 000 140 550 551 552 553
    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "000", "UAP slot  2 = 000");
    CHECK(uap[2]  == "140", "UAP slot  3 = 140");
    CHECK(uap[3]  == "550", "UAP slot  4 = 550");
    CHECK(uap[4]  == "551", "UAP slot  5 = 551");
    CHECK(uap[5]  == "552", "UAP slot  6 = 552");
    CHECK(uap[6]  == "553", "UAP slot  7 = 553");
    // Byte 2: 600 610 620 - - RE SP
    CHECK(uap[7]  == "600", "UAP slot  8 = 600");
    CHECK(uap[8]  == "610", "UAP slot  9 = 610");
    CHECK(uap[9]  == "620", "UAP slot 10 = 620");
    CHECK(uap[10] == "-",   "UAP slot 11 = - (unused)");
    CHECK(uap[12] == "RE",  "UAP slot 13 = RE");
    CHECK(uap[13] == "SP",  "UAP slot 14 = SP");

    // Item types
    CHECK(cat.items.at("000").type == ItemType::Fixed,           "000 is Fixed");
    CHECK(cat.items.at("552").type == ItemType::RepetitiveGroup, "552 is RepetitiveGroup");
    CHECK(cat.items.at("553").type == ItemType::Extended,        "553 is Extended");
    CHECK(cat.items.at("RE").type  == ItemType::SP,              "RE is SP/Explicit");
    CHECK(cat.items.at("SP").type  == ItemType::SP,              "SP is SP/Explicit");

    // Fixed byte sizes
    CHECK(cat.items.at("000").fixed_bytes == 1, "000 = 1 byte");
    CHECK(cat.items.at("010").fixed_bytes == 2, "010 = 2 bytes");
    CHECK(cat.items.at("140").fixed_bytes == 3, "140 = 3 bytes");
    CHECK(cat.items.at("550").fixed_bytes == 1, "550 = 1 byte");
    CHECK(cat.items.at("551").fixed_bytes == 1, "551 = 1 byte");
    CHECK(cat.items.at("600").fixed_bytes == 8, "600 = 8 bytes");
    CHECK(cat.items.at("610").fixed_bytes == 2, "610 = 2 bytes");
    CHECK(cat.items.at("620").fixed_bytes == 1, "620 = 1 byte");

    // I552 RepetitiveGroup: RSI(8)+spare(1)+RS1090+TX1030+TX1090+RSS+RSO+spare(2) = 16 bits
    const auto& i552 = cat.items.at("552");
    CHECK(i552.rep_group_bits == 16,              "552 rep_group_bits = 16 (2 bytes)");
    // Spares are included in rep_group_elements: RSI, spare(1), RS1090, TX1030, TX1090, RSS, RSO, spare(2) = 8 entries
    CHECK(i552.rep_group_elements.size() == 8,    "552 has 8 group elements (6 named + 2 spares)");
    CHECK(i552.rep_group_elements[0].name == "RSI",    "552 element[0] = RSI");
    CHECK(i552.rep_group_elements[2].name == "RS1090", "552 element[2] = RS1090 (after spare)");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode basic status message
//          I010(SAC=1,SIC=2) + I000(MSGTYP=1 "Start of Update Cycle")
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicMessage(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT19 status message ===\n";

    // UAP byte 1: 010(b7) 000(b6) 140(b5) 550(b4) 551(b3) 552(b2) 553(b1) FX(b0)
    // I010+I000 → bits 7,6 set → 1100_0000 = 0xC0
    // Total: 3(header) + 1(FSPEC) + 2(I010) + 1(I000) = 7 bytes

    std::vector<uint8_t> frame = {
        0x13,             // CAT = 19
        0x00, 0x07,       // LEN = 7
        0xC0,             // FSPEC byte 1 (FX=0): I010+I000
        0x01, 0x02,       // I010 SAC=1, SIC=2
        0x01              // I000 MSGTYP=1 (Start of Update Cycle)
    };

    hexdump(frame, "Basic status msg input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid, "block is valid");
    CHECK(blk.records.size() == 1, "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"), "I010 present");
    CHECK(rec.items.count("000"), "I000 present");
    CHECK(!rec.items.count("140"), "I140 absent");

    CHECK(rec.items.at("010").fields.at("SAC")    == 1, "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC")    == 2, "I010.SIC=2");
    CHECK(rec.items.at("000").fields.at("MSGTYP") == 1, "I000.MSGTYP=1 (Start of Update Cycle)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip Fixed status items (I000, I010, I140, I550, I551)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripSystemStatus(Codec& codec) {
    std::cout << "\n=== Test: Round-trip system status Fixed items ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=5; it.fields["SIC"]=10; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=2; enc_rec.items["000"]=it; }  // Periodic Status
    { DecodedItem it; it.fields["TOD"]=0x600000; enc_rec.items["140"]=it; }

    // I550: NOGO=1 (Degraded), OVL=0, TSV=0, TTF=0
    { DecodedItem it; it.fields["NOGO"]=1; it.fields["OVL"]=0; it.fields["TSV"]=0; it.fields["TTF"]=0; enc_rec.items["550"]=it; }

    // I551: TP1 in exec+good, TP2 in standby+faulted, TP3 and TP4 not used (0)
    { DecodedItem it;
      it.fields["TP1A"]=1; it.fields["TP1B"]=1;
      it.fields["TP2A"]=0; it.fields["TP2B"]=0;
      it.fields["TP3A"]=0; it.fields["TP3B"]=0;
      it.fields["TP4A"]=0; it.fields["TP4B"]=0;
      enc_rec.items["551"]=it; }

    std::vector<uint8_t> encoded = codec.encode(19, {enc_rec});
    hexdump(encoded, "System status encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "RT-status");

    CHECK(blk.records[0].items.at("550").fields.at("NOGO") == 1, "I550.NOGO=1 (Degraded)");
    CHECK(blk.records[0].items.at("551").fields.at("TP1A") == 1, "I551.TP1A=1 (Exec)");
    CHECK(blk.records[0].items.at("551").fields.at("TP1B") == 1, "I551.TP1B=1 (Good)");
    CHECK(blk.records[0].items.at("551").fields.at("TP2A") == 0, "I551.TP2A=0 (Standby)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip I552 Remote Sensor Detailed Status (RepetitiveGroup)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripRemoteSensors(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I552 Remote Sensor Status ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i552;
    // RS#1: RSI=1, RS1090=1, TX1030=0, TX1090=1, RSS=1 (Good), RSO=1 (Online)
    i552.group_repetitions.push_back({
        {"RSI", 1}, {"RS1090", 1}, {"TX1030", 0}, {"TX1090", 1}, {"RSS", 1}, {"RSO", 1}
    });
    // RS#2: RSI=2, RS1090=1, TX1030=1, TX1090=0, RSS=0 (Faulted), RSO=0 (Offline)
    i552.group_repetitions.push_back({
        {"RSI", 2}, {"RS1090", 1}, {"TX1030", 1}, {"TX1090", 0}, {"RSS", 0}, {"RSO", 0}
    });
    // RS#3: RSI=255, all present, good, online
    i552.group_repetitions.push_back({
        {"RSI", 255}, {"RS1090", 1}, {"TX1030", 1}, {"TX1090", 1}, {"RSS", 1}, {"RSO", 1}
    });
    enc_rec.items["552"] = i552;

    std::vector<uint8_t> encoded = codec.encode(19, {enc_rec});
    hexdump(encoded, "I552 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("552"), "I552 present");
    const auto& groups = blk.records[0].items.at("552").group_repetitions;
    CHECK(groups.size() == 3, "I552: 3 entries");

    if (groups.size() == 3) {
        CHECK(groups[0].at("RSI")    ==   1, "I552[0].RSI=1");
        CHECK(groups[0].at("RS1090") ==   1, "I552[0].RS1090=1 (present)");
        CHECK(groups[0].at("TX1030") ==   0, "I552[0].TX1030=0 (absent)");
        CHECK(groups[0].at("RSS")    ==   1, "I552[0].RSS=1 (Good)");
        CHECK(groups[0].at("RSO")    ==   1, "I552[0].RSO=1 (Online)");
        CHECK(groups[1].at("RSI")    ==   2, "I552[1].RSI=2");
        CHECK(groups[1].at("RSS")    ==   0, "I552[1].RSS=0 (Faulted)");
        CHECK(groups[2].at("RSI")    == 255, "I552[2].RSI=255");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip I553 Reference Transponder Status (Extended)
//          5a: single octet (REFTR1+REFTR2 only, FX=0)
//          5b: two octets  (REFTR1..REFTR4, FX=1 then FX=0)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripRefTransponders(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I553 Ref Transponder Status (Extended) ===\n";

    // --- 5a: 1-octet case ---
    {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        DecodedItem i553;
        i553.fields["REFTR1"] = 3; // Good
        i553.fields["REFTR2"] = 2; // Faulted
        enc_rec.items["553"] = i553;

        std::vector<uint8_t> encoded = codec.encode(19, {enc_rec});
        hexdump(encoded, "I553 1-octet encoded");

        DecodedBlock blk = codec.decode(encoded);
        CHECK(blk.valid, "553-1oct: block valid");
        if (!blk.records.empty()) {
            const auto& f = blk.records[0].items.at("553").fields;
            CHECK(f.at("REFTR1") == 3, "I553.REFTR1=3 (Good)");
            CHECK(f.at("REFTR2") == 2, "I553.REFTR2=2 (Faulted)");
            CHECK(!f.count("REFTR3"), "I553.REFTR3 absent (1-octet)");
            CHECK(!f.count("REFTR4"), "I553.REFTR4 absent (1-octet)");
        }
    }

    // --- 5b: 2-octet case ---
    {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        DecodedItem i553;
        i553.fields["REFTR1"] = 3; // Good
        i553.fields["REFTR2"] = 3; // Good
        i553.fields["REFTR3"] = 1; // Warning
        i553.fields["REFTR4"] = 3; // Good
        enc_rec.items["553"] = i553;

        std::vector<uint8_t> encoded = codec.encode(19, {enc_rec});
        hexdump(encoded, "I553 2-octet encoded");

        DecodedBlock blk = codec.decode(encoded);
        CHECK(blk.valid, "553-2oct: block valid");
        if (!blk.records.empty()) {
            const auto& f = blk.records[0].items.at("553").fields;
            CHECK(f.at("REFTR1") == 3, "I553.REFTR1=3 (Good)");
            CHECK(f.at("REFTR2") == 3, "I553.REFTR2=3 (Good)");
            CHECK(f.at("REFTR3") == 1, "I553.REFTR3=1 (Warning)");
            CHECK(f.at("REFTR4") == 3, "I553.REFTR4=3 (Good)");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip I600 WGS-84 position (32-bit signed LAT/LON)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripPosition(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I600 WGS-84 position ===\n";

    // scale = 180/2^30; positive LAT ~43°, negative LON ~-5°
    // LAT raw = 0x30000000 (805306368)
    // LON raw for -5°: -5 / (180/2^30) = -5 * 2^30/180 ≈ -29826161
    //   32-bit TC: 2^32 - 29826161 = 4265141135 = 0xFE38E38F

    const uint64_t lat_raw = 0x30000000ULL;
    const uint64_t lon_raw = static_cast<uint64_t>(
        static_cast<uint32_t>(static_cast<int32_t>(-29826161)));

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i600;
    i600.fields["LAT"] = lat_raw;
    i600.fields["LON"] = lon_raw;
    enc_rec.items["600"] = i600;

    std::vector<uint8_t> encoded = codec.encode(19, {enc_rec});
    hexdump(encoded, "I600 position encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("600"), "I600 present");
    const auto& f = blk.records[0].items.at("600").fields;
    CHECK(f.at("LAT") == lat_raw, "I600.LAT round-trip (positive)");
    CHECK(f.at("LON") == lon_raw, "I600.LON round-trip (negative)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip I610 height (signed 16-bit) and I620 undulation (signed 8-bit)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripHeightUndulation(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I610 height + I620 WGS-84 undulation ===\n";

    // I610: +100 m → raw = 100/0.25 = 400; -50 m → raw = -50/0.25 = -200 → 16-bit TC
    // I620: +40 m → raw = 40; -10 m → raw = -10 → 8-bit TC

    const uint64_t hgt_pos  = 400;
    const uint64_t hgt_neg  = static_cast<uint64_t>(static_cast<uint16_t>(static_cast<int16_t>(-200)));
    const uint64_t und_pos  = 40;
    const uint64_t und_neg  = static_cast<uint64_t>(static_cast<uint8_t>(static_cast<int8_t>(-10)));

    // Positive height + positive undulation
    {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        { DecodedItem it; it.fields["HGT"]=hgt_pos; enc_rec.items["610"]=it; }
        { DecodedItem it; it.fields["UND"]=und_pos;  enc_rec.items["620"]=it; }

        std::vector<uint8_t> encoded = codec.encode(19, {enc_rec});
        hexdump(encoded, "I610+I620 positive encoded");

        DecodedBlock blk = codec.decode(encoded);
        CHECK(blk.valid, "block valid (positive)");
        if (!blk.records.empty()) {
            CHECK(blk.records[0].items.at("610").fields.at("HGT") == hgt_pos, "I610.HGT=400 raw (+100m)");
            CHECK(blk.records[0].items.at("620").fields.at("UND") == und_pos, "I620.UND=40 raw (+40m)");
        }
    }

    // Negative height + negative undulation
    {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        { DecodedItem it; it.fields["HGT"]=hgt_neg; enc_rec.items["610"]=it; }
        { DecodedItem it; it.fields["UND"]=und_neg;  enc_rec.items["620"]=it; }

        std::vector<uint8_t> encoded = codec.encode(19, {enc_rec});
        hexdump(encoded, "I610+I620 negative encoded");

        DecodedBlock blk = codec.decode(encoded);
        CHECK(blk.valid, "block valid (negative)");
        if (!blk.records.empty()) {
            CHECK(blk.records[0].items.at("610").fields.at("HGT") == hgt_neg, "I610.HGT=-200 raw (-50m)");
            CHECK(blk.records[0].items.at("620").fields.at("UND") == und_neg, "I620.UND=-10 raw (-10m)");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Full round-trip with all item types
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip CAT19 ===\n";

    const uint64_t lat_raw = 0x20000000ULL;
    const uint64_t lon_raw = static_cast<uint64_t>(static_cast<uint32_t>(static_cast<int32_t>(-10000000)));
    const uint64_t hgt_raw = 120;  // 30 m / 0.25
    const uint64_t und_raw = static_cast<uint64_t>(static_cast<uint8_t>(static_cast<int8_t>(-5)));

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; enc_rec.items["000"]=it; }
    { DecodedItem it; it.fields["TOD"]=0x400000; enc_rec.items["140"]=it; }
    { DecodedItem it; it.fields["NOGO"]=0; it.fields["OVL"]=0; it.fields["TSV"]=0; it.fields["TTF"]=0; enc_rec.items["550"]=it; }
    { DecodedItem it;
      it.fields["TP1A"]=1; it.fields["TP1B"]=1;
      it.fields["TP2A"]=1; it.fields["TP2B"]=1;
      it.fields["TP3A"]=0; it.fields["TP3B"]=0;
      it.fields["TP4A"]=0; it.fields["TP4B"]=0;
      enc_rec.items["551"]=it; }
    {
        DecodedItem it;
        it.group_repetitions.push_back({{"RSI",1},{"RS1090",1},{"TX1030",1},{"TX1090",0},{"RSS",1},{"RSO",1}});
        it.group_repetitions.push_back({{"RSI",2},{"RS1090",1},{"TX1030",0},{"TX1090",0},{"RSS",1},{"RSO",1}});
        enc_rec.items["552"] = it;
    }
    { DecodedItem it; it.fields["REFTR1"]=3; it.fields["REFTR2"]=3; enc_rec.items["553"]=it; }
    { DecodedItem it; it.fields["LAT"]=lat_raw; it.fields["LON"]=lon_raw; enc_rec.items["600"]=it; }
    { DecodedItem it; it.fields["HGT"]=hgt_raw; enc_rec.items["610"]=it; }
    { DecodedItem it; it.fields["UND"]=und_raw; enc_rec.items["620"]=it; }

    std::vector<uint8_t> encoded = codec.encode(19, {enc_rec});
    hexdump(encoded, "Full RT encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "full-RT");
    CHECK(blk.records[0].items.count("010"), "I010 (mandatory) present");
}

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = "specs/CAT19.xml";
    if (argc >= 2)
        spec_path = argv[1];

    if (!fs::exists(spec_path)) {
        std::cerr << "Spec not found: " << spec_path << '\n';
        return 1;
    }

    Codec codec;

    testSpecLoad(codec, spec_path);
    testDecodeBasicMessage(codec);
    testRoundTripSystemStatus(codec);
    testRoundTripRemoteSensors(codec);
    testRoundTripRefTransponders(codec);
    testRoundTripPosition(codec);
    testRoundTripHeightUndulation(codec);
    testFullRoundTrip(codec);

    std::cout << "\n============================\n";
    if (failures == 0)
        std::cout << "ALL TESTS PASSED\n";
    else
        std::cout << failures << " TEST(S) FAILED\n";
    std::cout << "============================\n";

    return failures ? 1 : 0;
}
