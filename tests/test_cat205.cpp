// test_cat205.cpp – Tests for CAT205 Radio Direction Finder Reports.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat205

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

// Helpers: convert signed value to the raw uint64_t stored by the codec.
// The codec uses readU() unconditionally, so the decoded value is the
// N-bit two's complement pattern with no sign extension into 64 bits.
static uint64_t s32(int32_t v) {
    return static_cast<uint64_t>(static_cast<uint32_t>(v));
}
static uint64_t s24(int32_t v) {
    return static_cast<uint64_t>(static_cast<uint32_t>(v) & 0xFFFFFFu);
}
static uint64_t s16(int16_t v) {
    return static_cast<uint64_t>(static_cast<uint16_t>(v));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 1: XML spec loads correctly
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT205 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 205,        "cat number = 205");
    CHECK(cat.edition == "1.0",  "edition = 1.0");

    for (auto id : {"000","010","015","030","040","050","060","070","080","090",
                    "100","110","120","130","140","150","160","170","180","190",
                    "200","SP"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 22, "22 items total");

    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    CHECK(cat.uap_variations.at("default").size() == 28, "UAP has 28 slots");

    const auto& uap = cat.uap_variations.at("default");
    // Byte 1: 010 015 000 030 040 090 050
    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "015", "UAP slot  2 = 015");
    CHECK(uap[2]  == "000", "UAP slot  3 = 000");
    CHECK(uap[3]  == "030", "UAP slot  4 = 030");
    CHECK(uap[4]  == "040", "UAP slot  5 = 040");
    CHECK(uap[5]  == "090", "UAP slot  6 = 090");
    CHECK(uap[6]  == "050", "UAP slot  7 = 050");
    // Byte 2: 060 070 080 100 110 120 130
    CHECK(uap[7]  == "060", "UAP slot  8 = 060");
    CHECK(uap[8]  == "070", "UAP slot  9 = 070");
    CHECK(uap[9]  == "080", "UAP slot 10 = 080");
    CHECK(uap[10] == "100", "UAP slot 11 = 100");
    CHECK(uap[11] == "110", "UAP slot 12 = 110");
    CHECK(uap[12] == "120", "UAP slot 13 = 120");
    CHECK(uap[13] == "130", "UAP slot 14 = 130");
    // Byte 3: 140 150 160 170 180 190 200
    CHECK(uap[14] == "140", "UAP slot 15 = 140");
    CHECK(uap[15] == "150", "UAP slot 16 = 150");
    CHECK(uap[16] == "160", "UAP slot 17 = 160");
    CHECK(uap[17] == "170", "UAP slot 18 = 170");
    CHECK(uap[18] == "180", "UAP slot 19 = 180");
    CHECK(uap[19] == "190", "UAP slot 20 = 190");
    CHECK(uap[20] == "200", "UAP slot 21 = 200");
    // Byte 4: SP - - - - - -
    CHECK(uap[21] == "SP",  "UAP slot 22 = SP");
    CHECK(uap[22] == "-",   "UAP slot 23 = - (unused)");

    // Item types — all Fixed except I120 and SP
    for (auto id : {"000","010","015","030","040","050","060","070","080","090",
                    "100","110","130","140","150","160","170","180","190","200"}) {
        CHECK(cat.items.at(id).type == ItemType::Fixed,
              std::string(id) + " is Fixed");
    }
    CHECK(cat.items.at("120").type == ItemType::RepetitiveGroup, "120 is RepetitiveGroup");
    CHECK(cat.items.at("SP").type  == ItemType::SP,              "SP is SP/Explicit");

    // Fixed byte sizes
    CHECK(cat.items.at("000").fixed_bytes == 1,  "000 = 1 byte");
    CHECK(cat.items.at("010").fixed_bytes == 2,  "010 = 2 bytes");
    CHECK(cat.items.at("015").fixed_bytes == 1,  "015 = 1 byte");
    CHECK(cat.items.at("030").fixed_bytes == 3,  "030 = 3 bytes");
    CHECK(cat.items.at("040").fixed_bytes == 1,  "040 = 1 byte");
    CHECK(cat.items.at("050").fixed_bytes == 8,  "050 = 8 bytes (32+32 bits)");
    CHECK(cat.items.at("060").fixed_bytes == 6,  "060 = 6 bytes (24+24 bits)");
    CHECK(cat.items.at("070").fixed_bytes == 2,  "070 = 2 bytes");
    CHECK(cat.items.at("080").fixed_bytes == 2,  "080 = 2 bytes");
    CHECK(cat.items.at("090").fixed_bytes == 7,  "090 = 7 bytes");
    CHECK(cat.items.at("100").fixed_bytes == 1,  "100 = 1 byte");
    CHECK(cat.items.at("110").fixed_bytes == 1,  "110 = 1 byte");
    CHECK(cat.items.at("130").fixed_bytes == 8,  "130 = 8 bytes");
    CHECK(cat.items.at("140").fixed_bytes == 6,  "140 = 6 bytes");
    CHECK(cat.items.at("150").fixed_bytes == 1,  "150 = 1 byte");
    CHECK(cat.items.at("160").fixed_bytes == 2,  "160 = 2 bytes");
    CHECK(cat.items.at("170").fixed_bytes == 1,  "170 = 1 byte");
    CHECK(cat.items.at("180").fixed_bytes == 2,  "180 = 2 bytes");
    CHECK(cat.items.at("190").fixed_bytes == 1,  "190 = 1 byte");
    CHECK(cat.items.at("200").fixed_bytes == 2,  "200 = 2 bytes");

    // I120 RepetitiveGroup: 1 entry = 8 bits, 1 element (no spares)
    const auto& i120 = cat.items.at("120");
    CHECK(i120.rep_group_bits == 8,             "120 rep_group_bits = 8 (1 byte/entry)");
    CHECK(i120.rep_group_elements.size() == 1,  "120 has 1 group element");
    CHECK(i120.rep_group_elements[0].name == "ID", "120 element[0] = ID");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode a basic message from raw bytes
//          I010(SAC=1,SIC=2) + I000(MSGTYP=1 "System Position Report")
//
//  UAP slot 0=010(bit7), slot 2=000(bit5) → FSPEC=0xA0
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicMessage(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT205 message ===\n";

    // LEN = 3(header) + 1(FSPEC) + 2(I010) + 1(I000) = 7
    std::vector<uint8_t> frame = {
        0xCD,             // CAT = 205
        0x00, 0x07,       // LEN = 7
        0xA0,             // FSPEC byte 1 (FX=0): I010(b7)+I000(b5)
        0x01, 0x02,       // I010 SAC=1, SIC=2
        0x01              // I000 MSGTYP=1 (System Position Report)
    };

    hexdump(frame, "Basic RDF message input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid, "block is valid");
    CHECK(blk.records.size() == 1, "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"), "I010 present");
    CHECK(rec.items.count("000"), "I000 present");
    CHECK(!rec.items.count("050"), "I050 absent");
    CHECK(!rec.items.count("120"), "I120 absent");

    CHECK(rec.items.at("010").fields.at("SAC")    == 1, "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC")    == 2, "I010.SIC=2");
    CHECK(rec.items.at("000").fields.at("MSGTYP") == 1, "I000.MSGTYP=1 (Position Report)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip simple Fixed items (I010, I000, I015, I030, I040)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripSimpleFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip simple Fixed items ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=5; it.fields["SIC"]=10; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=2; enc_rec.items["000"]=it; }  // Bearing Report
    { DecodedItem it; it.fields["SI"]=3; enc_rec.items["015"]=it; }
    { DecodedItem it; it.fields["TOD"]=0x400000; enc_rec.items["030"]=it; }
    { DecodedItem it; it.fields["RN"]=42; enc_rec.items["040"]=it; }

    std::vector<uint8_t> encoded = codec.encode(205, {enc_rec});
    hexdump(encoded, "Simple Fixed encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("SAC")    == 5,        "I010.SAC=5");
    CHECK(items.at("010").fields.at("SIC")    == 10,       "I010.SIC=10");
    CHECK(items.at("000").fields.at("MSGTYP") == 2,        "I000.MSGTYP=2 (Bearing Report)");
    CHECK(items.at("015").fields.at("SI")     == 3,        "I015.SI=3");
    CHECK(items.at("030").fields.at("TOD")    == 0x400000, "I030.TOD=0x400000");
    CHECK(items.at("040").fields.at("RN")     == 42,       "I040.RN=42");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip I050 WGS-84 position + I060 Cartesian + I070/I080 bearings
//
//  Signed values are stored as N-bit two's complement in a uint64_t (no
//  64-bit sign extension). Use s32()/s24()/s16() to get the expected raw value.
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripPosition(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I050/I060 position and I070/I080 bearings ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    // I050: LAT=9109800 (≈48.86° N, Paris), LON=-524288 (≈-2.81° W)
    {
        DecodedItem it;
        it.fields["LAT"] = 9109800u;
        it.fields["LON"] = s32(-524288);
        enc_rec.items["050"] = it;
    }

    // I060: X=2000 (1000m east), Y=-4000 (-2000m south)
    {
        DecodedItem it;
        it.fields["X"] = 2000u;
        it.fields["Y"] = s24(-4000);
        enc_rec.items["060"] = it;
    }

    // I070: local bearing 135.00° → 13500 raw
    { DecodedItem it; it.fields["BRG"] = 13500; enc_rec.items["070"] = it; }

    // I080: system bearing 270.00° → 27000 raw
    { DecodedItem it; it.fields["BRG"] = 27000; enc_rec.items["080"] = it; }

    std::vector<uint8_t> encoded = codec.encode(205, {enc_rec});
    hexdump(encoded, "Position+Bearings encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;

    // I050 WGS-84
    CHECK(items.count("050"),                               "I050 present");
    CHECK(items.at("050").fields.at("LAT") == 9109800u,    "I050.LAT=9109800 (positive)");
    CHECK(items.at("050").fields.at("LON") == s32(-524288),"I050.LON=-524288 (negative)");

    // I060 Cartesian
    CHECK(items.count("060"),                              "I060 present");
    CHECK(items.at("060").fields.at("X") == 2000u,         "I060.X=2000 (positive)");
    CHECK(items.at("060").fields.at("Y") == s24(-4000),    "I060.Y=-4000 (negative)");

    // Bearings
    CHECK(items.at("070").fields.at("BRG") == 13500, "I070.BRG=13500 (135.00°)");
    CHECK(items.at("080").fields.at("BRG") == 27000, "I080.BRG=27000 (270.00°)");

    // Boundary: bearing near 360° (max valid = 36000)
    DecodedRecord r2;
    r2.uap_variation = "default";
    { DecodedItem it; it.fields["BRG"] = 35999; r2.items["070"] = it; }
    auto enc2 = codec.encode(205, {r2});
    auto blk2 = codec.decode(enc2);
    if (!blk2.records.empty())
        CHECK(blk2.records[0].items.at("070").fields.at("BRG") == 35999,
              "I070.BRG=35999 (359.99°)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip I120 Contributing Sensors (RepetitiveGroup)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripContributingSensors(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I120 Contributing Sensors ===\n";

    // 3 sensor IDs
    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    DecodedItem i120;
    i120.group_repetitions.push_back({{"ID", 1}});
    i120.group_repetitions.push_back({{"ID", 7}});
    i120.group_repetitions.push_back({{"ID", 255}});
    enc_rec.items["120"] = i120;

    std::vector<uint8_t> encoded = codec.encode(205, {enc_rec});
    hexdump(encoded, "I120 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("120"), "I120 present");
    const auto& groups = blk.records[0].items.at("120").group_repetitions;
    CHECK(groups.size() == 3, "I120: 3 entries");

    if (groups.size() == 3) {
        CHECK(groups[0].at("ID") == 1,   "I120[0].ID=1");
        CHECK(groups[1].at("ID") == 7,   "I120[1].ID=7");
        CHECK(groups[2].at("ID") == 255, "I120[2].ID=255");
    }

    // Single sensor
    DecodedRecord r2;
    r2.uap_variation = "default";
    DecodedItem i2;
    i2.group_repetitions.push_back({{"ID", 42}});
    r2.items["120"] = i2;
    auto enc2 = codec.encode(205, {r2});
    auto blk2 = codec.decode(enc2);
    if (!blk2.records.empty()) {
        const auto& g2 = blk2.records[0].items.at("120").group_repetitions;
        CHECK(g2.size() == 1, "I120: 1 entry");
        if (!g2.empty())
            CHECK(g2[0].at("ID") == 42, "I120[0].ID=42");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip sensor measurement items (I100, I110, I160, I170,
//          I180, I190, I200)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripSensorMeasurements(Codec& codec) {
    std::cout << "\n=== Test: Round-trip sensor measurement items ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["QM"]  = 0b10110100; enc_rec.items["100"] = it; }
    { DecodedItem it; it.fields["EU"]  = 5;          enc_rec.items["110"] = it; }  // 500m
    { DecodedItem it; it.fields["TN"]  = 0xABCD;     enc_rec.items["160"] = it; }
    { DecodedItem it; it.fields["SID"] = 3;           enc_rec.items["170"] = it; }
    // I180: signal level -45.50 dBµV → raw = s16(-4550)
    { DecodedItem it; it.fields["SL"]  = s16(-4550);  enc_rec.items["180"] = it; }
    { DecodedItem it; it.fields["SQ"]  = 200;         enc_rec.items["190"] = it; }
    // I200: elevation 15.00° → 1500 raw; also test -90.00° → s16(-9000)
    { DecodedItem it; it.fields["SE"]  = 1500;        enc_rec.items["200"] = it; }

    std::vector<uint8_t> encoded = codec.encode(205, {enc_rec});
    hexdump(encoded, "Sensor measurements encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("100").fields.at("QM")  == 0b10110100, "I100.QM=0b10110100");
    CHECK(items.at("110").fields.at("EU")  == 5,          "I110.EU=5 (500m)");
    CHECK(items.at("160").fields.at("TN")  == 0xABCD,     "I160.TN=0xABCD");
    CHECK(items.at("170").fields.at("SID") == 3,          "I170.SID=3");
    CHECK(items.at("180").fields.at("SL")  == s16(-4550), "I180.SL=-45.50 dBµV");
    CHECK(items.at("190").fields.at("SQ")  == 200,        "I190.SQ=200");
    CHECK(items.at("200").fields.at("SE")  == 1500,       "I200.SE=1500 (15.00°)");

    // Negative elevation: -90.00° → s16(-9000)
    DecodedRecord r2;
    r2.uap_variation = "default";
    { DecodedItem it; it.fields["SE"] = s16(-9000); r2.items["200"] = it; }
    auto enc2 = codec.encode(205, {r2});
    auto blk2 = codec.decode(enc2);
    if (!blk2.records.empty())
        CHECK(blk2.records[0].items.at("200").fields.at("SE") == s16(-9000),
              "I200.SE=-90.00° (negative elevation)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip conflicting transmitter items (I130, I140, I150)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripConflictingTransmitter(Codec& codec) {
    std::cout << "\n=== Test: Round-trip conflicting transmitter items (I130/I140/I150) ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    // I130: second transmitter at slightly different WGS-84 position
    {
        DecodedItem it;
        it.fields["LAT"] = 9200000u;
        it.fields["LON"] = s32(-600000);
        enc_rec.items["130"] = it;
    }
    // I140: second transmitter Cartesian
    {
        DecodedItem it;
        it.fields["X"] = 500u;
        it.fields["Y"] = s24(-800);
        enc_rec.items["140"] = it;
    }
    // I150: uncertainty 20 × 100m = 2000m
    { DecodedItem it; it.fields["EU"] = 20; enc_rec.items["150"] = it; }

    std::vector<uint8_t> encoded = codec.encode(205, {enc_rec});
    hexdump(encoded, "Conflicting transmitter encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;

    CHECK(items.count("130"),                               "I130 present");
    CHECK(items.at("130").fields.at("LAT") == 9200000u,    "I130.LAT=9200000");
    CHECK(items.at("130").fields.at("LON") == s32(-600000),"I130.LON=-600000 (negative)");

    CHECK(items.count("140"),                              "I140 present");
    CHECK(items.at("140").fields.at("X") == 500u,          "I140.X=500");
    CHECK(items.at("140").fields.at("Y") == s24(-800),     "I140.Y=-800 (negative)");

    CHECK(items.at("150").fields.at("EU") == 20, "I150.EU=20 (2000m)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Full round-trip with all items
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip CAT205 ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["SI"]=1; enc_rec.items["015"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; enc_rec.items["000"]=it; }
    { DecodedItem it; it.fields["TOD"]=0x800000; enc_rec.items["030"]=it; }
    { DecodedItem it; it.fields["RN"]=7; enc_rec.items["040"]=it; }
    // I090 channel name "121.500" in ASCII (7B)
    { uint64_t cn = (uint64_t)0x31<<48|(uint64_t)0x32<<40|(uint64_t)0x31<<32|
                    (uint64_t)0x2E<<24|(uint64_t)0x35<<16|(uint64_t)0x30<<8|0x30;
      DecodedItem it; it.fields["CN"]=cn; enc_rec.items["090"]=it; }
    { DecodedItem it;
      it.fields["LAT"] = 9109800u;
      it.fields["LON"] = s32(-524288);
      enc_rec.items["050"]=it; }
    { DecodedItem it;
      it.fields["X"] = 1000u;
      it.fields["Y"] = s24(-500);
      enc_rec.items["060"]=it; }
    { DecodedItem it; it.fields["BRG"]=9000; enc_rec.items["070"]=it; }   // 90.00°
    { DecodedItem it; it.fields["BRG"]=18000; enc_rec.items["080"]=it; }  // 180.00°
    { DecodedItem it; it.fields["QM"]=0xFF; enc_rec.items["100"]=it; }
    { DecodedItem it; it.fields["EU"]=10; enc_rec.items["110"]=it; }      // 1000m
    { DecodedItem it;
      it.group_repetitions.push_back({{"ID", 2}});
      it.group_repetitions.push_back({{"ID", 5}});
      enc_rec.items["120"]=it; }
    { DecodedItem it; it.fields["TN"]=1234; enc_rec.items["160"]=it; }
    { DecodedItem it; it.fields["SID"]=1; enc_rec.items["170"]=it; }
    { DecodedItem it; it.fields["SL"] = s16(-3000); enc_rec.items["180"]=it; }
    { DecodedItem it; it.fields["SQ"]=128; enc_rec.items["190"]=it; }
    { DecodedItem it; it.fields["SE"] = 500; enc_rec.items["200"]=it; }  // 5.00°

    std::vector<uint8_t> encoded = codec.encode(205, {enc_rec});
    hexdump(encoded, "Full RT encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.count("010"), "I010 (mandatory) present");
    CHECK(items.count("050"), "I050 present");
    CHECK(items.count("060"), "I060 present");
    CHECK(items.count("120"), "I120 present");
    CHECK(items.count("180"), "I180 present");
    CHECK(items.count("200"), "I200 present");

    CHECK(items.at("010").fields.at("SAC") == 1,       "I010.SAC=1");
    CHECK(items.at("040").fields.at("RN")  == 7,       "I040.RN=7");
    CHECK(items.at("050").fields.at("LAT") == 9109800u,"I050.LAT=9109800");
    CHECK(items.at("050").fields.at("LON") == s32(-524288), "I050.LON=-524288");
    CHECK(items.at("060").fields.at("Y")   == s24(-500),    "I060.Y=-500");
    CHECK(items.at("070").fields.at("BRG") == 9000,    "I070.BRG=9000 (90.00°)");
    CHECK(items.at("110").fields.at("EU")  == 10,      "I110.EU=10 (1000m)");
    CHECK(items.at("160").fields.at("TN")  == 1234,    "I160.TN=1234");
    CHECK(items.at("190").fields.at("SQ")  == 128,     "I190.SQ=128");
    CHECK(items.at("180").fields.at("SL")  == s16(-3000), "I180.SL=-30.00 dBµV");
    CHECK(items.at("200").fields.at("SE")  == 500,     "I200.SE=500 (5.00°)");

    const auto& groups = items.at("120").group_repetitions;
    CHECK(groups.size() == 2, "I120: 2 sensors");
    if (groups.size() == 2) {
        CHECK(groups[0].at("ID") == 2, "I120[0].ID=2");
        CHECK(groups[1].at("ID") == 5, "I120[1].ID=5");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = "specs/CAT205.xml";
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
    testRoundTripPosition(codec);
    testRoundTripContributingSensors(codec);
    testRoundTripSensorMeasurements(codec);
    testRoundTripConflictingTransmitter(codec);
    testFullRoundTrip(codec);

    std::cout << "\n============================\n";
    if (failures == 0)
        std::cout << "ALL TESTS PASSED\n";
    else
        std::cout << failures << " TEST(S) FAILED\n";
    std::cout << "============================\n";

    return failures ? 1 : 0;
}
