// test_cat63.cpp – Tests for CAT63 Sensor Status Reports decode/encode round-trip.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat63

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
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 1: XML spec loads and item types / UAP are correct
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT63 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 63,         "cat number = 63");
    CHECK(cat.edition == "1.7",  "edition = 1.7");

    // All expected items present
    for (auto id : {"010","015","030","050","060","070","080","081","090","091","092","RE","SP"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 13, "13 items total");

    // UAP
    CHECK(cat.uap_variations.count("default"), "UAP variation 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    CHECK(cat.uap_variations.at("default").size() == 14, "UAP has 14 slots");

    // Spot-check UAP ordering
    const auto& uap = cat.uap_variations.at("default");
    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "015", "UAP slot  2 = 015");
    CHECK(uap[2]  == "030", "UAP slot  3 = 030");
    CHECK(uap[3]  == "050", "UAP slot  4 = 050");
    CHECK(uap[4]  == "060", "UAP slot  5 = 060");
    CHECK(uap[5]  == "070", "UAP slot  6 = 070");
    CHECK(uap[6]  == "080", "UAP slot  7 = 080");
    CHECK(uap[7]  == "081", "UAP slot  8 = 081");
    CHECK(uap[8]  == "090", "UAP slot  9 = 090");
    CHECK(uap[9]  == "091", "UAP slot 10 = 091");
    CHECK(uap[10] == "092", "UAP slot 11 = 092");
    CHECK(uap[11] == "-",   "UAP slot 12 = - (unused)");
    CHECK(uap[12] == "RE",  "UAP slot 13 = RE");
    CHECK(uap[13] == "SP",  "UAP slot 14 = SP");

    // Item types
    CHECK(cat.items.at("010").type == ItemType::Fixed,    "010 is Fixed");
    CHECK(cat.items.at("015").type == ItemType::Fixed,    "015 is Fixed");
    CHECK(cat.items.at("030").type == ItemType::Fixed,    "030 is Fixed");
    CHECK(cat.items.at("050").type == ItemType::Fixed,    "050 is Fixed");
    CHECK(cat.items.at("060").type == ItemType::Extended, "060 is Extended");
    CHECK(cat.items.at("070").type == ItemType::Fixed,    "070 is Fixed");
    CHECK(cat.items.at("080").type == ItemType::Fixed,    "080 is Fixed");
    CHECK(cat.items.at("081").type == ItemType::Fixed,    "081 is Fixed");
    CHECK(cat.items.at("090").type == ItemType::Fixed,    "090 is Fixed");
    CHECK(cat.items.at("091").type == ItemType::Fixed,    "091 is Fixed");
    CHECK(cat.items.at("092").type == ItemType::Fixed,    "092 is Fixed");
    CHECK(cat.items.at("RE").type  == ItemType::SP,       "RE is SP/Explicit");
    CHECK(cat.items.at("SP").type  == ItemType::SP,       "SP is SP/Explicit");

    // Fixed byte sizes
    CHECK(cat.items.at("010").fixed_bytes == 2, "010 = 2 bytes");
    CHECK(cat.items.at("015").fixed_bytes == 1, "015 = 1 byte");
    CHECK(cat.items.at("030").fixed_bytes == 3, "030 = 3 bytes");
    CHECK(cat.items.at("050").fixed_bytes == 2, "050 = 2 bytes");
    CHECK(cat.items.at("070").fixed_bytes == 2, "070 = 2 bytes");
    CHECK(cat.items.at("080").fixed_bytes == 4, "080 = 4 bytes");
    CHECK(cat.items.at("081").fixed_bytes == 2, "081 = 2 bytes");
    CHECK(cat.items.at("090").fixed_bytes == 4, "090 = 4 bytes");
    CHECK(cat.items.at("091").fixed_bytes == 2, "091 = 2 bytes");
    CHECK(cat.items.at("092").fixed_bytes == 2, "092 = 2 bytes");

    // Extended octet count for I060
    CHECK(cat.items.at("060").octets.size() == 3, "060 has 3 octets");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode basic sensor status message
//          I010 (SAC=1,SIC=2), I015 (SID=5), I030 (TOD=64s), I060 (CON=1,FX=0)
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicSensorStatus(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT63 record ===\n";

    // FSPEC byte 1: slots 1-7, FX=0
    //   bit7=1 (I010), bit6=1 (I015), bit5=1 (I030), bit4=0, bit3=1 (I060), bit2=0, bit1=0, bit0=0(FX)
    //   = 1110_1000 = 0xE8
    //
    // I010: SAC=0x01, SIC=0x02
    // I015: SID=0x05
    // I030: TOD = 0x002000 = 8192 → 8192 * 0.0078125 = 64.0 s
    // I060: octet 1 only (FX=0)
    //   CON=1(01b), PSR=0, SSR=0, MDS=0, ADS=0, MLT=0 → data bits = 0100000
    //   byte = 0100_000_0 = 0x40

    std::vector<uint8_t> frame = {
        0x3F,             // CAT = 63
        0x00, 0x0B,       // LEN = 11
        0xE8,             // FSPEC
        0x01, 0x02,       // I010 SAC=1, SIC=2
        0x05,             // I015 SID=5
        0x00, 0x20, 0x00, // I030 TOD=0x002000=8192
        0x40              // I060 CON=1, FX=0
    };

    hexdump(frame, "Decode basic input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid, "block is valid");
    CHECK(blk.records.size() == 1, "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"), "I010 present");
    CHECK(rec.items.count("015"), "I015 present");
    CHECK(rec.items.count("030"), "I030 present");
    CHECK(rec.items.count("060"), "I060 present");

    // I010
    CHECK(rec.items.at("010").fields.at("SAC") == 1, "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC") == 2, "I010.SIC=2");

    // I015
    CHECK(rec.items.at("015").fields.at("SID") == 5, "I015.SID=5");

    // I030 TOD raw = 8192
    CHECK(rec.items.at("030").fields.at("TOD") == 8192, "I030.TOD=8192 raw");

    // I060 CON=1 (Degraded), only 1 octet decoded
    CHECK(rec.items.at("060").fields.at("CON") == 1, "I060.CON=1");
    CHECK(rec.items.at("060").fields.at("PSR") == 0, "I060.PSR=0");
    CHECK(rec.items.at("060").fields.at("SSR") == 0, "I060.SSR=0");
    CHECK(rec.items.at("060").fields.at("MDS") == 0, "I060.MDS=0");
    CHECK(rec.items.at("060").fields.at("ADS") == 0, "I060.ADS=0");
    CHECK(rec.items.at("060").fields.at("MLT") == 0, "I060.MLT=0");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip I060 Extended – all 3 octets
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtended060(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I060 Extended (3 octets) ===\n";

    // I060 octet 1: CON=2(10b), PSR=1, SSR=0, MDS=1, ADS=0, MLT=1  FX=1
    //   data bits: 10_1_0_1_0_1 = 1010101, byte = 1010101_1 = 0xAB
    // I060 octet 2: OPS=1, ODP=0, OXT=1, MSC=0, TSV=1, NPW=0, spare=0  FX=1
    //   data bits: 1_0_1_0_1_0_0 = 1010100, byte = 1010100_1 = 0xA9
    // I060 octet 3: TTF_EP=1, TTF_VAL=0, SPO_EP=1, SPO_VAL=1, spare=000  FX=0
    //   data bits: 1_0_1_1_000 = 1011000, byte = 1011000_0 = 0xB0

    // FSPEC: slot 5 = I060 only, byte 1
    //   bit7=0, bit6=0, bit5=0, bit4=0, bit3=1(I060), bit2=0, bit1=0, bit0=0(FX)
    //   = 0000_1000 = 0x08

    std::vector<uint8_t> frame = {
        0x3F,             // CAT=63
        0x00, 0x07,       // LEN=7
        0x08,             // FSPEC: I060 only
        0xAB, 0xA9, 0xB0  // I060: 3 octets
    };

    hexdump(frame, "Extended060 input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("060"), "I060 present");

    const auto& f = rec.items.at("060").fields;
    // Octet 1
    CHECK(f.at("CON") == 2, "I060.CON=2");
    CHECK(f.at("PSR") == 1, "I060.PSR=1");
    CHECK(f.at("SSR") == 0, "I060.SSR=0");
    CHECK(f.at("MDS") == 1, "I060.MDS=1");
    CHECK(f.at("ADS") == 0, "I060.ADS=0");
    CHECK(f.at("MLT") == 1, "I060.MLT=1");
    // Octet 2
    CHECK(f.at("OPS") == 1, "I060.OPS=1");
    CHECK(f.at("ODP") == 0, "I060.ODP=0");
    CHECK(f.at("OXT") == 1, "I060.OXT=1");
    CHECK(f.at("MSC") == 0, "I060.MSC=0");
    CHECK(f.at("TSV") == 1, "I060.TSV=1");
    CHECK(f.at("NPW") == 0, "I060.NPW=0");
    // Octet 3
    CHECK(f.at("TTF_EP")  == 1, "I060.TTF_EP=1");
    CHECK(f.at("TTF_VAL") == 0, "I060.TTF_VAL=0");
    CHECK(f.at("SPO_EP")  == 1, "I060.SPO_EP=1");
    CHECK(f.at("SPO_VAL") == 1, "I060.SPO_VAL=1");

    // Encode and re-decode
    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    enc_rec.items["060"] = rec.items.at("060");

    std::vector<uint8_t> encoded = codec.encode(63, {enc_rec});
    hexdump(encoded, "Extended060 encoded");

    DecodedBlock blk2 = codec.decode(encoded);
    CHECK(blk2.valid, "re-decode valid");
    if (blk2.records.empty()) return;

    checkItemsMatch(blk2.records[0].items, enc_rec.items, "RT-060");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip I060 – single octet (FX=0)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtended060SingleOctet(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I060 Extended (1 octet) ===\n";

    // Octet 1: CON=3, PSR=1, SSR=1, MDS=0, ADS=1, MLT=0  FX=0
    //   data bits: 11_1_1_0_1_0 = 1111010, byte = 1111010_0 = 0xF4 (wait, FX=0)
    // Let me recalculate: 7 data bits + 1 FX bit
    // CON(2)=11, PSR(1)=1, SSR(1)=1, MDS(1)=0, ADS(1)=1, MLT(1)=0 → 1111010
    // byte: 1111010 | FX=0 → 1111_0100 = 0xF4? No wait:
    // The byte is: [data bit 6][data bit 5][...][data bit 0][FX]
    // = [CON_msb][CON_lsb][PSR][SSR][MDS][ADS][MLT][FX]
    // = [1][1][1][1][0][1][0][0] = 0xF4

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    DecodedItem i060;
    i060.fields["CON"] = 3;
    i060.fields["PSR"] = 1;
    i060.fields["SSR"] = 1;
    i060.fields["MDS"] = 0;
    i060.fields["ADS"] = 1;
    i060.fields["MLT"] = 0;
    enc_rec.items["060"] = i060;

    std::vector<uint8_t> encoded = codec.encode(63, {enc_rec});
    hexdump(encoded, "Extended060-1oct encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "RT-060-1oct");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip I070 Time Stamping Bias (signed)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripTimeBias(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I070 Time Stamping Bias ===\n";

    // TSB = -500 ms → 16-bit two's complement = 65536 - 500 = 65036 = 0xFE0C
    // codec stores raw unsigned bit pattern
    const uint64_t tsb_raw = static_cast<uint64_t>(static_cast<uint16_t>(static_cast<int16_t>(-500)));

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    DecodedItem i070;
    i070.fields["TSB"] = tsb_raw;
    enc_rec.items["070"] = i070;

    std::vector<uint8_t> encoded = codec.encode(63, {enc_rec});
    hexdump(encoded, "TSB encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("070"), "I070 present");
    CHECK(blk.records[0].items.at("070").fields.at("TSB") == tsb_raw, "I070.TSB round-trip");

    // Also test positive TSB
    DecodedRecord enc_rec2;
    enc_rec2.uap_variation = "default";
    DecodedItem i070b;
    i070b.fields["TSB"] = 250; // +250 ms
    enc_rec2.items["070"] = i070b;

    std::vector<uint8_t> encoded2 = codec.encode(63, {enc_rec2});
    DecodedBlock blk2 = codec.decode(encoded2);
    CHECK(blk2.valid, "block2 valid");
    if (!blk2.records.empty())
        CHECK(blk2.records[0].items.at("070").fields.at("TSB") == 250, "I070.TSB=+250 round-trip");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip I080 SSR/ModeS Range Gain and Bias
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripSSRGainBias(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I080 SSR/ModeS Range Gain and Bias ===\n";

    // SRG: signed 16-bit, scale=0.00001
    //   SRG=1000 raw → 0.01 gain bias
    // SRB: signed 16-bit, scale=0.0078125 NM
    //   SRB=128 raw → 1.0 NM bias

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    DecodedItem i080;
    i080.fields["SRG"] = 1000;
    i080.fields["SRB"] = 128;
    enc_rec.items["080"] = i080;

    std::vector<uint8_t> encoded = codec.encode(63, {enc_rec});
    hexdump(encoded, "I080 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("080"), "I080 present");
    CHECK(blk.records[0].items.at("080").fields.at("SRG") == 1000, "I080.SRG=1000");
    CHECK(blk.records[0].items.at("080").fields.at("SRB") == 128,  "I080.SRB=128");

    // Test with negative values
    const uint64_t srg_neg = static_cast<uint64_t>(static_cast<uint16_t>(static_cast<int16_t>(-500)));
    const uint64_t srb_neg = static_cast<uint64_t>(static_cast<uint16_t>(static_cast<int16_t>(-64)));

    DecodedRecord enc_rec2;
    enc_rec2.uap_variation = "default";
    DecodedItem i080b;
    i080b.fields["SRG"] = srg_neg;
    i080b.fields["SRB"] = srb_neg;
    enc_rec2.items["080"] = i080b;

    std::vector<uint8_t> encoded2 = codec.encode(63, {enc_rec2});
    DecodedBlock blk2 = codec.decode(encoded2);
    CHECK(blk2.valid, "block2 valid");
    if (blk2.records.empty()) return;

    CHECK(blk2.records[0].items.at("080").fields.at("SRG") == srg_neg, "I080.SRG negative round-trip");
    CHECK(blk2.records[0].items.at("080").fields.at("SRB") == srb_neg, "I080.SRB negative round-trip");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip I081, I091, I092 azimuth/elevation bias fields
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripAzimuthElevationBias(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I081/I091/I092 azimuth and elevation bias ===\n";

    // SAB = 1820 raw → 1820 * (360/65536) ≈ 9.99°
    // PAB = 3640 raw → 3640 * (360/65536) ≈ 19.97°
    // PEB = -910 raw → raw 16-bit two's complement
    const uint64_t peb_raw = static_cast<uint64_t>(static_cast<uint16_t>(static_cast<int16_t>(-910)));

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i081;
    i081.fields["SAB"] = 1820;
    enc_rec.items["081"] = i081;

    DecodedItem i091;
    i091.fields["PAB"] = 3640;
    enc_rec.items["091"] = i091;

    DecodedItem i092;
    i092.fields["PEB"] = peb_raw;
    enc_rec.items["092"] = i092;

    std::vector<uint8_t> encoded = codec.encode(63, {enc_rec});
    hexdump(encoded, "I081/091/092 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("081"), "I081 present");
    CHECK(blk.records[0].items.count("091"), "I091 present");
    CHECK(blk.records[0].items.count("092"), "I092 present");

    CHECK(blk.records[0].items.at("081").fields.at("SAB") == 1820,    "I081.SAB=1820");
    CHECK(blk.records[0].items.at("091").fields.at("PAB") == 3640,    "I091.PAB=3640");
    CHECK(blk.records[0].items.at("092").fields.at("PEB") == peb_raw, "I092.PEB negative round-trip");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Round-trip PSR Range Gain and Bias (I090)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripPSRGainBias(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I090 PSR Range Gain and Bias ===\n";

    const uint64_t prg_neg = static_cast<uint64_t>(static_cast<uint16_t>(static_cast<int16_t>(-200)));

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    DecodedItem i090;
    i090.fields["PRG"] = prg_neg;
    i090.fields["PRB"] = 256; // +2.0 NM
    enc_rec.items["090"] = i090;

    std::vector<uint8_t> encoded = codec.encode(63, {enc_rec});
    hexdump(encoded, "I090 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("090"), "I090 present");
    CHECK(blk.records[0].items.at("090").fields.at("PRG") == prg_neg, "I090.PRG negative");
    CHECK(blk.records[0].items.at("090").fields.at("PRB") == 256,     "I090.PRB=256");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 9: Full round-trip – all fixed items + I060 two octets
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip CAT63 ===\n";

    // I070 TSB = -100 ms
    const uint64_t tsb_raw = static_cast<uint64_t>(static_cast<uint16_t>(static_cast<int16_t>(-100)));
    // I080 SRG=-300, SRB=64
    const uint64_t srg_raw = static_cast<uint64_t>(static_cast<uint16_t>(static_cast<int16_t>(-300)));

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    // I010
    {
        DecodedItem it;
        it.fields["SAC"] = 10;
        it.fields["SIC"] = 20;
        enc_rec.items["010"] = it;
    }
    // I015
    {
        DecodedItem it;
        it.fields["SID"] = 7;
        enc_rec.items["015"] = it;
    }
    // I030 TOD = 0x600000 = 6291456 → 6291456 * 0.0078125 = 49152.0 s
    {
        DecodedItem it;
        it.fields["TOD"] = 0x600000;
        enc_rec.items["030"] = it;
    }
    // I050
    {
        DecodedItem it;
        it.fields["SAC"] = 10;
        it.fields["SIC"] = 30;
        enc_rec.items["050"] = it;
    }
    // I060 (2 octets: CON=0+OPS=1,TSV=1)
    {
        DecodedItem it;
        it.fields["CON"] = 0;
        it.fields["PSR"] = 0;
        it.fields["SSR"] = 0;
        it.fields["MDS"] = 0;
        it.fields["ADS"] = 0;
        it.fields["MLT"] = 0;
        it.fields["OPS"] = 1;
        it.fields["ODP"] = 0;
        it.fields["OXT"] = 0;
        it.fields["MSC"] = 0;
        it.fields["TSV"] = 1;
        it.fields["NPW"] = 0;
        enc_rec.items["060"] = it;
    }
    // I070
    {
        DecodedItem it;
        it.fields["TSB"] = tsb_raw;
        enc_rec.items["070"] = it;
    }
    // I080
    {
        DecodedItem it;
        it.fields["SRG"] = srg_raw;
        it.fields["SRB"] = 64;
        enc_rec.items["080"] = it;
    }
    // I081
    {
        DecodedItem it;
        it.fields["SAB"] = 910;
        enc_rec.items["081"] = it;
    }
    // I090
    {
        DecodedItem it;
        it.fields["PRG"] = 500;
        it.fields["PRB"] = 0;
        enc_rec.items["090"] = it;
    }
    // I091
    {
        DecodedItem it;
        it.fields["PAB"] = 0;
        enc_rec.items["091"] = it;
    }
    // I092
    {
        DecodedItem it;
        it.fields["PEB"] = 0;
        enc_rec.items["092"] = it;
    }

    std::vector<uint8_t> encoded = codec.encode(63, {enc_rec});
    hexdump(encoded, "Full RT encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "full-RT");

    // Verify mandatory item I010 is present
    CHECK(blk.records[0].items.count("010"), "I010 (mandatory) present after full RT");
}

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = "specs/CAT63.xml";
    if (argc >= 2)
        spec_path = argv[1];

    if (!fs::exists(spec_path)) {
        std::cerr << "Spec not found: " << spec_path << '\n';
        return 1;
    }

    Codec codec;

    testSpecLoad(codec, spec_path);
    testDecodeBasicSensorStatus(codec);
    testRoundTripExtended060(codec);
    testRoundTripExtended060SingleOctet(codec);
    testRoundTripTimeBias(codec);
    testRoundTripSSRGainBias(codec);
    testRoundTripAzimuthElevationBias(codec);
    testRoundTripPSRGainBias(codec);
    testFullRoundTrip(codec);

    std::cout << "\n============================\n";
    if (failures == 0)
        std::cout << "ALL TESTS PASSED\n";
    else
        std::cout << failures << " TEST(S) FAILED\n";
    std::cout << "============================\n";

    return failures ? 1 : 0;
}
