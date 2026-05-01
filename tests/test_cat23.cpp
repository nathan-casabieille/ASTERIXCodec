// test_cat23.cpp – Tests for CAT23 CNS/ATM Ground Station and Service Status Reports.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat23

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
    std::cout << "\n=== Test: CAT23 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 23,         "cat number = 23");
    CHECK(cat.edition == "1.3",  "edition = 1.3");

    for (auto id : {"000","010","015","070","100","101","110","120","200","RE","SP"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 11, "11 items total");

    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    CHECK(cat.uap_variations.at("default").size() == 14, "UAP has 14 slots");

    const auto& uap = cat.uap_variations.at("default");
    // Byte 1: 010 000 015 070 100 101 200
    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "000", "UAP slot  2 = 000");
    CHECK(uap[2]  == "015", "UAP slot  3 = 015");
    CHECK(uap[3]  == "070", "UAP slot  4 = 070");
    CHECK(uap[4]  == "100", "UAP slot  5 = 100");
    CHECK(uap[5]  == "101", "UAP slot  6 = 101");
    CHECK(uap[6]  == "200", "UAP slot  7 = 200");
    // Byte 2: 110 120 - - - RE SP
    CHECK(uap[7]  == "110", "UAP slot  8 = 110");
    CHECK(uap[8]  == "120", "UAP slot  9 = 120");
    CHECK(uap[9]  == "-",   "UAP slot 10 = - (unused)");
    CHECK(uap[12] == "RE",  "UAP slot 13 = RE");
    CHECK(uap[13] == "SP",  "UAP slot 14 = SP");

    // Item types
    CHECK(cat.items.at("000").type == ItemType::Fixed,           "000 is Fixed");
    CHECK(cat.items.at("100").type == ItemType::Extended,        "100 is Extended");
    CHECK(cat.items.at("101").type == ItemType::Fixed,           "101 is Fixed (2B model)");
    CHECK(cat.items.at("110").type == ItemType::Extended,        "110 is Extended");
    CHECK(cat.items.at("120").type == ItemType::RepetitiveGroup, "120 is RepetitiveGroup");
    CHECK(cat.items.at("RE").type  == ItemType::SP,              "RE is SP/Explicit");

    // Fixed byte sizes
    CHECK(cat.items.at("000").fixed_bytes == 1, "000 = 1 byte");
    CHECK(cat.items.at("010").fixed_bytes == 2, "010 = 2 bytes");
    CHECK(cat.items.at("015").fixed_bytes == 1, "015 = 1 byte");
    CHECK(cat.items.at("070").fixed_bytes == 3, "070 = 3 bytes");
    CHECK(cat.items.at("101").fixed_bytes == 2, "101 = 2 bytes");
    CHECK(cat.items.at("200").fixed_bytes == 1, "200 = 1 byte");

    // I120 RepetitiveGroup: TYPE(8)+REF(1)+spare(7)+CV(32) = 48 bits = 6B per entry
    // Spares included in rep_group_elements: TYPE, REF, spare(7), CV = 4 entries
    const auto& i120 = cat.items.at("120");
    CHECK(i120.rep_group_bits == 48,             "120 rep_group_bits = 48 (6 bytes)");
    CHECK(i120.rep_group_elements.size() == 4,   "120 has 4 group elements (TYPE+REF+spare+CV)");
    CHECK(i120.rep_group_elements[0].name == "TYPE", "120 element[0] = TYPE");
    CHECK(i120.rep_group_elements[3].name == "CV",   "120 element[3] = CV");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode basic ground station status report
//          I010(SAC=1,SIC=2) + I000(RTYP=1 "Ground station status report")
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicMessage(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT23 status report ===\n";

    // FSPEC byte 1: 010(b7) 000(b6) → 1100_0000 = 0xC0, FX=0
    // Total: 3 + 1(FSPEC) + 2(I010) + 1(I000) = 7 bytes

    std::vector<uint8_t> frame = {
        0x17,             // CAT = 23
        0x00, 0x07,       // LEN = 7
        0xC0,             // FSPEC byte 1 (FX=0): I010+I000
        0x01, 0x02,       // I010 SAC=1, SIC=2
        0x01              // I000 RTYP=1 (Ground station status report)
    };

    hexdump(frame, "Basic status report input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid, "block is valid");
    CHECK(blk.records.size() == 1, "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"), "I010 present");
    CHECK(rec.items.count("000"), "I000 present");
    CHECK(!rec.items.count("100"), "I100 absent");

    CHECK(rec.items.at("010").fields.at("SAC")  == 1, "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC")  == 2, "I010.SIC=2");
    CHECK(rec.items.at("000").fields.at("RTYP") == 1, "I000.RTYP=1 (GS status)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip basic Fixed items (I000, I010, I015, I070, I200)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripBasicFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip basic Fixed items ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=3; it.fields["SIC"]=7; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["RTYP"]=1; enc_rec.items["000"]=it; }
    { DecodedItem it; it.fields["SID"]=1; it.fields["STYP"]=2; enc_rec.items["015"]=it; } // ADS-B Ext Squitter
    { DecodedItem it; it.fields["TOD"]=0x600000; enc_rec.items["070"]=it; }
    { DecodedItem it; it.fields["RANGE"]=100; enc_rec.items["200"]=it; }  // 100 NM

    std::vector<uint8_t> encoded = codec.encode(23, {enc_rec});
    hexdump(encoded, "Basic Fixed encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "RT-basic");

    CHECK(blk.records[0].items.at("015").fields.at("SID")   == 1, "I015.SID=1");
    CHECK(blk.records[0].items.at("015").fields.at("STYP")  == 2, "I015.STYP=2 (ADS-B Ext Squitter)");
    CHECK(blk.records[0].items.at("200").fields.at("RANGE") == 100, "I200.RANGE=100 NM");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip I100 Ground Station Status (Extended)
//          4a: 1-octet (primary fields only)
//          4b: 2-octet (primary + GSSP extension)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripGroundStationStatus(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I100 Ground Station Status (Extended) ===\n";

    // --- 4a: 1-octet: octet 1 only, no GSSP ---
    {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        DecodedItem i100;
        i100.fields["NOGO"] = 1; // not operational
        i100.fields["ODP"]  = 0;
        i100.fields["OXT"]  = 0;
        i100.fields["MSC"]  = 1; // monitoring connected
        i100.fields["TSV"]  = 0;
        i100.fields["SPO"]  = 0;
        i100.fields["RN"]   = 0;
        enc_rec.items["100"] = i100;

        std::vector<uint8_t> encoded = codec.encode(23, {enc_rec});
        hexdump(encoded, "I100 1-octet encoded");

        DecodedBlock blk = codec.decode(encoded);
        CHECK(blk.valid, "100-1oct: block valid");
        if (!blk.records.empty()) {
            const auto& f = blk.records[0].items.at("100").fields;
            CHECK(f.at("NOGO") == 1, "I100.NOGO=1");
            CHECK(f.at("MSC")  == 1, "I100.MSC=1");
            CHECK(!f.count("GSSP"), "I100.GSSP absent (1-octet)");
        }
    }

    // --- 4b: 2-octet: primary + GSSP ---
    {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        DecodedItem i100;
        i100.fields["NOGO"] = 0;
        i100.fields["ODP"]  = 0;
        i100.fields["OXT"]  = 0;
        i100.fields["MSC"]  = 1;
        i100.fields["TSV"]  = 0;
        i100.fields["SPO"]  = 0;
        i100.fields["RN"]   = 0;
        i100.fields["GSSP"] = 30; // 30s reporting period
        enc_rec.items["100"] = i100;

        std::vector<uint8_t> encoded = codec.encode(23, {enc_rec});
        hexdump(encoded, "I100 2-octet encoded");

        DecodedBlock blk = codec.decode(encoded);
        CHECK(blk.valid, "100-2oct: block valid");
        if (!blk.records.empty()) {
            const auto& f = blk.records[0].items.at("100").fields;
            CHECK(f.at("NOGO") == 0,  "I100.NOGO=0");
            CHECK(f.at("MSC")  == 1,  "I100.MSC=1");
            CHECK(f.at("GSSP") == 30, "I100.GSSP=30s");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip I101 Service Configuration (Fixed 2B: RP + SC)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripServiceConfig(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I101 Service Configuration ===\n";

    // RP=4 → 4 × 0.5s = 2s report period; SC=1 (NRA class)

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i101;
    i101.fields["RP"] = 4;
    i101.fields["SC"] = 1;
    enc_rec.items["101"] = i101;

    std::vector<uint8_t> encoded = codec.encode(23, {enc_rec});
    hexdump(encoded, "I101 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("101"), "I101 present");
    const auto& f = blk.records[0].items.at("101").fields;
    CHECK(f.at("RP") == 4, "I101.RP=4 (2s period)");
    CHECK(f.at("SC") == 1, "I101.SC=1 (NRA class)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip I110 Service Status (Extended, 1-octet)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripServiceStatus(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I110 Service Status (Extended) ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i110;
    i110.fields["STAT"] = 4; // Normal
    enc_rec.items["110"] = i110;

    std::vector<uint8_t> encoded = codec.encode(23, {enc_rec});
    hexdump(encoded, "I110 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("110"), "I110 present");
    CHECK(blk.records[0].items.at("110").fields.at("STAT") == 4, "I110.STAT=4 (Normal)");

    // Test all status values
    for (uint64_t stat = 0; stat <= 5; ++stat) {
        DecodedRecord r2;
        r2.uap_variation = "default";
        DecodedItem i; i.fields["STAT"] = stat;
        r2.items["110"] = i;
        auto enc2 = codec.encode(23, {r2});
        auto blk2 = codec.decode(enc2);
        if (!blk2.records.empty())
            CHECK(blk2.records[0].items.at("110").fields.at("STAT") == stat,
                  "I110.STAT=" + std::to_string(stat) + " RT");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip I120 Service Statistics (RepetitiveGroup, 3 entries)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripServiceStatistics(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I120 Service Statistics ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i120;
    // Entry 0: TYPE=3 (total msgs received), REF=1 (from last report), CV=12345678
    i120.group_repetitions.push_back({{"TYPE", 3}, {"REF", 1}, {"CV", 12345678}});
    // Entry 1: TYPE=21 (basic msgs received), REF=0 (from midnight), CV=5000
    i120.group_repetitions.push_back({{"TYPE", 21}, {"REF", 0}, {"CV", 5000}});
    // Entry 2: TYPE=4 (total msgs transmitted), REF=1, CV=0xDEADBEEF
    i120.group_repetitions.push_back({{"TYPE", 4}, {"REF", 1}, {"CV", 0xDEADBEEFu}});
    enc_rec.items["120"] = i120;

    std::vector<uint8_t> encoded = codec.encode(23, {enc_rec});
    hexdump(encoded, "I120 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("120"), "I120 present");
    const auto& groups = blk.records[0].items.at("120").group_repetitions;
    CHECK(groups.size() == 3, "I120: 3 entries");

    if (groups.size() == 3) {
        CHECK(groups[0].at("TYPE") == 3,          "I120[0].TYPE=3");
        CHECK(groups[0].at("REF")  == 1,          "I120[0].REF=1");
        CHECK(groups[0].at("CV")   == 12345678u,  "I120[0].CV=12345678");
        CHECK(groups[1].at("TYPE") == 21,         "I120[1].TYPE=21");
        CHECK(groups[1].at("REF")  == 0,          "I120[1].REF=0");
        CHECK(groups[1].at("CV")   == 5000u,      "I120[1].CV=5000");
        CHECK(groups[2].at("CV")   == 0xDEADBEEFu,"I120[2].CV=0xDEADBEEF");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Full round-trip with all item types
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip CAT23 ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["RTYP"]=3; enc_rec.items["000"]=it; } // Service statistics
    { DecodedItem it; it.fields["SID"]=2; it.fields["STYP"]=2; enc_rec.items["015"]=it; }
    { DecodedItem it; it.fields["TOD"]=0x400000; enc_rec.items["070"]=it; }
    { DecodedItem it;
      it.fields["NOGO"]=0; it.fields["ODP"]=0; it.fields["OXT"]=0;
      it.fields["MSC"]=1;  it.fields["TSV"]=0; it.fields["SPO"]=0; it.fields["RN"]=0;
      it.fields["GSSP"]=10;
      enc_rec.items["100"]=it; }
    { DecodedItem it; it.fields["RP"]=10; it.fields["SC"]=1; enc_rec.items["101"]=it; }
    { DecodedItem it; it.fields["RANGE"]=50; enc_rec.items["200"]=it; }
    { DecodedItem it; it.fields["STAT"]=4; enc_rec.items["110"]=it; } // Normal
    {
        DecodedItem it;
        it.group_repetitions.push_back({{"TYPE", 3}, {"REF", 1}, {"CV", 99999}});
        it.group_repetitions.push_back({{"TYPE", 21}, {"REF", 0}, {"CV", 888}});
        enc_rec.items["120"] = it;
    }

    std::vector<uint8_t> encoded = codec.encode(23, {enc_rec});
    hexdump(encoded, "Full RT encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "full-RT");
    CHECK(blk.records[0].items.count("010"), "I010 (mandatory) present");
}

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = "specs/CAT23.xml";
    if (argc >= 2)
        spec_path = argv[1];

    if (!fs::exists(spec_path)) {
        std::cerr << "Spec not found: " << spec_path << '\n';
        return 1;
    }

    Codec codec;

    testSpecLoad(codec, spec_path);
    testDecodeBasicMessage(codec);
    testRoundTripBasicFixed(codec);
    testRoundTripGroundStationStatus(codec);
    testRoundTripServiceConfig(codec);
    testRoundTripServiceStatus(codec);
    testRoundTripServiceStatistics(codec);
    testFullRoundTrip(codec);

    std::cout << "\n============================\n";
    if (failures == 0)
        std::cout << "ALL TESTS PASSED\n";
    else
        std::cout << failures << " TEST(S) FAILED\n";
    std::cout << "============================\n";

    return failures ? 1 : 0;
}
