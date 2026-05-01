// test_cat65.cpp – Tests for CAT65 SDPS Service Status Reports.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat65

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
    std::cout << "\n=== Test: CAT65 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 65,         "cat number = 65");
    CHECK(cat.edition == "1.6",  "edition = 1.6");

    for (auto id : {"000","010","015","020","030","040","050","RE","SP"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 9, "9 items total");

    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    CHECK(cat.uap_variations.at("default").size() == 14, "UAP has 14 slots");

    const auto& uap = cat.uap_variations.at("default");
    // Byte 1: 010 000 015 030 020 040 050
    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "000", "UAP slot  2 = 000");
    CHECK(uap[2]  == "015", "UAP slot  3 = 015");
    CHECK(uap[3]  == "030", "UAP slot  4 = 030");
    CHECK(uap[4]  == "020", "UAP slot  5 = 020");
    CHECK(uap[5]  == "040", "UAP slot  6 = 040");
    CHECK(uap[6]  == "050", "UAP slot  7 = 050");
    // Byte 2: - - - - - RE SP
    CHECK(uap[7]  == "-",   "UAP slot  8 = - (unused)");
    CHECK(uap[11] == "-",   "UAP slot 12 = - (unused)");
    CHECK(uap[12] == "RE",  "UAP slot 13 = RE");
    CHECK(uap[13] == "SP",  "UAP slot 14 = SP");

    // All items are Fixed
    for (auto id : {"000","010","015","020","030","040","050"}) {
        CHECK(cat.items.at(id).type == ItemType::Fixed,
              std::string(id) + " is Fixed");
    }
    CHECK(cat.items.at("RE").type == ItemType::SP, "RE is SP/Explicit");

    // Fixed byte sizes
    CHECK(cat.items.at("000").fixed_bytes == 1, "000 = 1 byte");
    CHECK(cat.items.at("010").fixed_bytes == 2, "010 = 2 bytes");
    CHECK(cat.items.at("015").fixed_bytes == 1, "015 = 1 byte");
    CHECK(cat.items.at("020").fixed_bytes == 1, "020 = 1 byte");
    CHECK(cat.items.at("030").fixed_bytes == 3, "030 = 3 bytes");
    CHECK(cat.items.at("040").fixed_bytes == 1, "040 = 1 byte");
    CHECK(cat.items.at("050").fixed_bytes == 1, "050 = 1 byte");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode a basic SDPS status message from raw bytes
//          I010(SAC=1,SIC=2) + I000(MSGTYP=1 "SDPS Status")
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicMessage(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT65 SDPS status message ===\n";

    // FSPEC byte 1: I010(b7)+I000(b6) = 0b11000000 = 0xC0, FX=0
    // LEN = 3(header) + 1(FSPEC) + 2(I010) + 1(I000) = 7
    std::vector<uint8_t> frame = {
        0x41,             // CAT = 65
        0x00, 0x07,       // LEN = 7
        0xC0,             // FSPEC byte 1 (FX=0): I010+I000
        0x01, 0x02,       // I010 SAC=1, SIC=2
        0x01              // I000 MSGTYP=1 (SDPS Status)
    };

    hexdump(frame, "Basic SDPS status input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid, "block is valid");
    CHECK(blk.records.size() == 1, "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"), "I010 present");
    CHECK(rec.items.count("000"), "I000 present");
    CHECK(!rec.items.count("040"), "I040 absent");

    CHECK(rec.items.at("010").fields.at("SAC")    == 1, "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC")    == 2, "I010.SIC=2");
    CHECK(rec.items.at("000").fields.at("MSGTYP") == 1, "I000.MSGTYP=1 (SDPS Status)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip all simple Fixed items (I010, I000, I015, I020, I030)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripBasicFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip basic Fixed items ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=5; it.fields["SIC"]=10; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=2; enc_rec.items["000"]=it; }  // End of Batch
    { DecodedItem it; it.fields["SI"]=7; enc_rec.items["015"]=it; }
    { DecodedItem it; it.fields["BN"]=3; enc_rec.items["020"]=it; }
    { DecodedItem it; it.fields["TOM"]=0x300000; enc_rec.items["030"]=it; }

    std::vector<uint8_t> encoded = codec.encode(65, {enc_rec});
    hexdump(encoded, "Basic Fixed encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("SAC")    == 5,        "I010.SAC=5");
    CHECK(items.at("010").fields.at("SIC")    == 10,       "I010.SIC=10");
    CHECK(items.at("000").fields.at("MSGTYP") == 2,        "I000.MSGTYP=2 (End of Batch)");
    CHECK(items.at("015").fields.at("SI")     == 7,        "I015.SI=7");
    CHECK(items.at("020").fields.at("BN")     == 3,        "I020.BN=3");
    CHECK(items.at("030").fields.at("TOM")    == 0x300000, "I030.TOM=0x300000");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip I040 SDPS Configuration and Status
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripSDPSConfig(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I040 SDPS Configuration and Status ===\n";

    // 4a: Operational, SDPS-1 selected
    {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        DecodedItem i040;
        i040.fields["NOGO"] = 0;  // Operational
        i040.fields["OVL"]  = 0;  // Default
        i040.fields["TSV"]  = 0;  // Default
        i040.fields["PSS"]  = 1;  // SDPS-1 selected
        i040.fields["STTN"] = 0;
        enc_rec.items["040"] = i040;

        auto encoded = codec.encode(65, {enc_rec});
        hexdump(encoded, "I040 Operational+SDPS-1 encoded");
        auto blk = codec.decode(encoded);
        CHECK(blk.valid, "I040-a: block valid");
        if (!blk.records.empty()) {
            const auto& f = blk.records[0].items.at("040").fields;
            CHECK(f.at("NOGO") == 0, "I040.NOGO=0 (Operational)");
            CHECK(f.at("OVL")  == 0, "I040.OVL=0 (Default)");
            CHECK(f.at("TSV")  == 0, "I040.TSV=0 (Default)");
            CHECK(f.at("PSS")  == 1, "I040.PSS=1 (SDPS-1)");
            CHECK(f.at("STTN") == 0, "I040.STTN=0");
        }
    }

    // 4b: Degraded, overload, invalid time, SDPS-2
    {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        DecodedItem i040;
        i040.fields["NOGO"] = 1;  // Degraded
        i040.fields["OVL"]  = 1;  // Overload
        i040.fields["TSV"]  = 1;  // Invalid Time Source
        i040.fields["PSS"]  = 2;  // SDPS-2
        i040.fields["STTN"] = 1;  // track re-numbering toggled
        enc_rec.items["040"] = i040;

        auto encoded = codec.encode(65, {enc_rec});
        hexdump(encoded, "I040 Degraded+Overload encoded");
        auto blk = codec.decode(encoded);
        CHECK(blk.valid, "I040-b: block valid");
        if (!blk.records.empty()) {
            const auto& f = blk.records[0].items.at("040").fields;
            CHECK(f.at("NOGO") == 1, "I040.NOGO=1 (Degraded)");
            CHECK(f.at("OVL")  == 1, "I040.OVL=1 (Overload)");
            CHECK(f.at("TSV")  == 1, "I040.TSV=1 (Invalid Time Source)");
            CHECK(f.at("PSS")  == 2, "I040.PSS=2 (SDPS-2)");
            CHECK(f.at("STTN") == 1, "I040.STTN=1");
        }
    }

    // 4c: Not connected, SDPS-3
    {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        DecodedItem i040;
        i040.fields["NOGO"] = 2;  // Not currently connected
        i040.fields["OVL"]  = 0;
        i040.fields["TSV"]  = 0;
        i040.fields["PSS"]  = 3;  // SDPS-3
        i040.fields["STTN"] = 0;
        enc_rec.items["040"] = i040;

        auto encoded = codec.encode(65, {enc_rec});
        auto blk = codec.decode(encoded);
        CHECK(blk.valid, "I040-c: block valid");
        if (!blk.records.empty()) {
            const auto& f = blk.records[0].items.at("040").fields;
            CHECK(f.at("NOGO") == 2, "I040.NOGO=2 (Not connected)");
            CHECK(f.at("PSS")  == 3, "I040.PSS=3 (SDPS-3)");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip I050 Service Status Report — all 16 values
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripServiceStatusReport(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I050 Service Status Report (all values) ===\n";

    for (uint64_t sr = 1; sr <= 16; ++sr) {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        DecodedItem i050; i050.fields["SR"] = sr;
        enc_rec.items["050"] = i050;

        auto encoded = codec.encode(65, {enc_rec});
        auto blk = codec.decode(encoded);
        if (!blk.records.empty()) {
            CHECK(blk.records[0].items.at("050").fields.at("SR") == sr,
                  "I050.SR=" + std::to_string(sr));
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Batch number boundary (0 and 255)
// ─────────────────────────────────────────────────────────────────────────────
static void testBatchNumberBoundary(Codec& codec) {
    std::cout << "\n=== Test: I020 Batch Number boundary values ===\n";

    for (uint64_t bn : {0u, 127u, 255u}) {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; enc_rec.items["010"]=it; }
        { DecodedItem it; it.fields["BN"]=bn; enc_rec.items["020"]=it; }

        auto encoded = codec.encode(65, {enc_rec});
        auto blk = codec.decode(encoded);
        if (!blk.records.empty()) {
            CHECK(blk.records[0].items.at("020").fields.at("BN") == bn,
                  "I020.BN=" + std::to_string(bn));
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Full round-trip with all items present
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip CAT65 ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=10; it.fields["SIC"]=20; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; enc_rec.items["000"]=it; }  // SDPS Status
    { DecodedItem it; it.fields["SI"]=3; enc_rec.items["015"]=it; }
    { DecodedItem it; it.fields["TOM"]=0x800000; enc_rec.items["030"]=it; }
    { DecodedItem it; it.fields["BN"]=0; enc_rec.items["020"]=it; }
    { DecodedItem it;
      it.fields["NOGO"]=0; it.fields["OVL"]=0; it.fields["TSV"]=0;
      it.fields["PSS"]=1; it.fields["STTN"]=0;
      enc_rec.items["040"]=it; }
    { DecodedItem it; it.fields["SR"]=10; enc_rec.items["050"]=it; }  // Main radar becoming operational

    std::vector<uint8_t> encoded = codec.encode(65, {enc_rec});
    hexdump(encoded, "Full RT encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.count("010"), "I010 (mandatory) present");
    CHECK(items.count("000"), "I000 present");
    CHECK(items.count("015"), "I015 present");
    CHECK(items.count("020"), "I020 present");
    CHECK(items.count("030"), "I030 present");
    CHECK(items.count("040"), "I040 present");
    CHECK(items.count("050"), "I050 present");

    CHECK(items.at("010").fields.at("SAC")    == 10,       "I010.SAC=10");
    CHECK(items.at("010").fields.at("SIC")    == 20,       "I010.SIC=20");
    CHECK(items.at("000").fields.at("MSGTYP") == 1,        "I000.MSGTYP=1");
    CHECK(items.at("015").fields.at("SI")     == 3,        "I015.SI=3");
    CHECK(items.at("020").fields.at("BN")     == 0,        "I020.BN=0");
    CHECK(items.at("030").fields.at("TOM")    == 0x800000, "I030.TOM=0x800000");
    CHECK(items.at("040").fields.at("NOGO")   == 0,        "I040.NOGO=0 (Operational)");
    CHECK(items.at("040").fields.at("PSS")    == 1,        "I040.PSS=1 (SDPS-1)");
    CHECK(items.at("050").fields.at("SR")     == 10,       "I050.SR=10 (main radar operational)");
}

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = "specs/CAT65.xml";
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
    testRoundTripSDPSConfig(codec);
    testRoundTripServiceStatusReport(codec);
    testBatchNumberBoundary(codec);
    testFullRoundTrip(codec);

    std::cout << "\n============================\n";
    if (failures == 0)
        std::cout << "ALL TESTS PASSED\n";
    else
        std::cout << failures << " TEST(S) FAILED\n";
    std::cout << "============================\n";

    return failures ? 1 : 0;
}
