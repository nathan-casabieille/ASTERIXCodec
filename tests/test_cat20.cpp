// test_cat20.cpp – Tests for CAT20 Multilateration Target Reports, Ed. 1.11.

#include "ASTERIXCodec/Codec.hpp"
#include "ASTERIXCodec/SpecLoader.hpp"

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

static uint64_t s8(int8_t   v) { return static_cast<uint64_t>(static_cast<uint8_t>(v)); }
static uint64_t s16(int16_t v) { return static_cast<uint64_t>(static_cast<uint16_t>(v)); }
static uint64_t s24(int32_t v) { return static_cast<uint64_t>(static_cast<uint32_t>(v) & 0xFFFFFF); }
static uint64_t s32(int32_t v) { return static_cast<uint64_t>(static_cast<uint32_t>(v)); }

// ─────────────────────────────────────────────────────────────────────────────
//  Test 1: Spec load
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT20 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 20,         "cat number = 20");
    CHECK(cat.edition == "1.11", "edition = 1.11");

    for (auto id : {"010","020","030","041","042","050","055","070","090","100",
                    "105","110","140","161","170","202","210","220","230","245",
                    "250","260","300","310","400","500","RE","SP"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 28, "28 items total");

    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    const auto& uap = cat.uap_variations.at("default");
    CHECK(uap.size() == 28, "UAP has 28 slots");

    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "020", "UAP slot  2 = 020");
    CHECK(uap[2]  == "140", "UAP slot  3 = 140");
    CHECK(uap[6]  == "170", "UAP slot  7 = 170");
    CHECK(uap[7]  == "070", "UAP slot  8 = 070");
    CHECK(uap[13] == "110", "UAP slot 14 = 110");
    CHECK(uap[18] == "500", "UAP slot 19 = 500");
    CHECK(uap[20] == "250", "UAP slot 21 = 250");
    CHECK(uap[23] == "030", "UAP slot 24 = 030");
    CHECK(uap[26] == "RE",  "UAP slot 27 = RE");
    CHECK(uap[27] == "SP",  "UAP slot 28 = SP");

    // Item types
    for (auto id : {"010","041","042","050","055","070","090","100","105","110",
                    "140","161","202","210","220","230","245","260","300","310"}) {
        CHECK(cat.items.at(id).type == ItemType::Fixed,
              std::string(id) + " is Fixed");
    }
    CHECK(cat.items.at("020").type == ItemType::Extended,        "020 is Extended");
    CHECK(cat.items.at("170").type == ItemType::Extended,        "170 is Extended");
    CHECK(cat.items.at("030").type == ItemType::Repetitive,      "030 is Repetitive(FX)");
    CHECK(cat.items.at("250").type == ItemType::RepetitiveGroup, "250 is RepetitiveGroup");
    CHECK(cat.items.at("400").type == ItemType::RepetitiveGroup, "400 is RepetitiveGroup");
    CHECK(cat.items.at("500").type == ItemType::Compound,        "500 is Compound");
    CHECK(cat.items.at("SP").type  == ItemType::SP,              "SP is SP/Explicit");

    // Fixed sizes
    CHECK(cat.items.at("010").fixed_bytes == 2, "010 = 2 bytes");
    CHECK(cat.items.at("041").fixed_bytes == 8, "041 = 8 bytes");
    CHECK(cat.items.at("042").fixed_bytes == 6, "042 = 6 bytes");
    CHECK(cat.items.at("050").fixed_bytes == 2, "050 = 2 bytes");
    CHECK(cat.items.at("055").fixed_bytes == 1, "055 = 1 byte");
    CHECK(cat.items.at("070").fixed_bytes == 2, "070 = 2 bytes");
    CHECK(cat.items.at("090").fixed_bytes == 2, "090 = 2 bytes");
    CHECK(cat.items.at("100").fixed_bytes == 4, "100 = 4 bytes");
    CHECK(cat.items.at("105").fixed_bytes == 2, "105 = 2 bytes");
    CHECK(cat.items.at("110").fixed_bytes == 2, "110 = 2 bytes");
    CHECK(cat.items.at("140").fixed_bytes == 3, "140 = 3 bytes");
    CHECK(cat.items.at("161").fixed_bytes == 2, "161 = 2 bytes");
    CHECK(cat.items.at("202").fixed_bytes == 4, "202 = 4 bytes");
    CHECK(cat.items.at("210").fixed_bytes == 2, "210 = 2 bytes");
    CHECK(cat.items.at("220").fixed_bytes == 3, "220 = 3 bytes");
    CHECK(cat.items.at("230").fixed_bytes == 2, "230 = 2 bytes");
    CHECK(cat.items.at("245").fixed_bytes == 7, "245 = 7 bytes");
    CHECK(cat.items.at("260").fixed_bytes == 7, "260 = 7 bytes");
    CHECK(cat.items.at("300").fixed_bytes == 1, "300 = 1 byte");
    CHECK(cat.items.at("310").fixed_bytes == 1, "310 = 1 byte");

    // Extended octet counts
    CHECK(cat.items.at("020").octets.size() == 3, "020 has 3 octets");
    CHECK(cat.items.at("170").octets.size() == 2, "170 has 2 octets");

    // RepetitiveGroup bits per entry
    CHECK(cat.items.at("250").rep_group_bits == 64, "250 rep_group_bits = 64");
    CHECK(cat.items.at("400").rep_group_bits == 8,  "400 rep_group_bits = 8");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode a minimal message from raw bytes
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicMessage(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT20 message ===\n";

    // FSPEC B1: I010(bit7)=1 I140(bit5)=1 → 0xA0 FX=0
    // Total: CAT(1)+LEN(2)+FSPEC(1)+I010(2)+I140(3) = 9 bytes
    // I140: TOD = 43200*128 = 5529600 = 0x546000
    std::vector<uint8_t> frame = {
        0x14,             // CAT = 20
        0x00, 0x09,       // LEN = 9
        0xA0,             // FSPEC: 010+140, FX=0
        0x01, 0x02,       // I010: SAC=1, SIC=2
        0x54, 0x60, 0x00  // I140: TOD = 43200*128 (12h)
    };

    hexdump(frame, "Basic message input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid,                "block is valid");
    CHECK(blk.records.size() == 1,  "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"),  "I010 present");
    CHECK(rec.items.count("140"),  "I140 present");
    CHECK(!rec.items.count("020"), "I020 absent");
    CHECK(!rec.items.count("041"), "I041 absent");

    CHECK(rec.items.at("010").fields.at("SAC") == 1,          "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC") == 2,          "I010.SIC=2");
    CHECK(rec.items.at("140").fields.at("TOD") == 43200u*128, "I140.TOD=12h");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip core Fixed items
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCoreFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip core Fixed items ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=5; it.fields["SIC"]=10; rec.items["010"]=it; }
    { DecodedItem it; it.fields["TOD"]=43200u*128u; rec.items["140"]=it; }
    { DecodedItem it; it.fields["TRN"]=0xABC; rec.items["161"]=it; }
    { DecodedItem it; it.fields["TGT"]=0x3C1234u; rec.items["220"]=it; }
    { DecodedItem it; it.fields["V"]=0; it.fields["G"]=0; it.fields["L"]=0; it.fields["MODE3A"]=0x5AA; rec.items["070"]=it; }
    { DecodedItem it; it.fields["V"]=0; it.fields["G"]=0; it.fields["FL"]=400; rec.items["090"]=it; }
    { DecodedItem it; it.fields["VFI"]=9; rec.items["300"]=it; }
    { DecodedItem it; it.fields["TRB"]=0; it.fields["MSG"]=2; rec.items["310"]=it; }

    auto encoded = codec.encode(20, {rec});
    hexdump(encoded, "CoreFixed encoded");
    CHECK(!encoded.empty(), "encode produced output");
    CHECK(encoded[0] == 0x14, "CAT byte = 0x14 (20)");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("SAC")    == 5,            "010/SAC = 5");
    CHECK(items.at("010").fields.at("SIC")    == 10,           "010/SIC = 10");
    CHECK(items.at("140").fields.at("TOD")    == 43200u*128u,  "140/TOD = 12h");
    CHECK(items.at("161").fields.at("TRN")    == 0xABC,        "161/TRN = 0xABC");
    CHECK(items.at("220").fields.at("TGT")    == 0x3C1234u,    "220/TGT = 0x3C1234");
    CHECK(items.at("070").fields.at("MODE3A") == 0x5AA,        "070/MODE3A = 0x5AA");
    CHECK(items.at("090").fields.at("FL")     == 400,          "090/FL = 400 (FL100)");
    CHECK(items.at("300").fields.at("VFI")    == 9,            "300/VFI = 9 (Bus)");
    CHECK(items.at("310").fields.at("TRB")    == 0,            "310/TRB = 0");
    CHECK(items.at("310").fields.at("MSG")    == 2,            "310/MSG = 2 (FOLLOW-ME)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip more Fixed items
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripMoreFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip more Fixed items ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; rec.items["010"]=it; }
    { DecodedItem it; it.fields["TOD"]=36000u*128u; rec.items["140"]=it; }

    // WGS-84 position (signed 32-bit fields)
    { DecodedItem it;
      it.fields["LAT"]=s32(1073741824); // ~32°
      it.fields["LON"]=s32(-536870912); // ~-16°
      rec.items["041"]=it; }

    // Cartesian position (signed 24-bit)
    { DecodedItem it;
      it.fields["X"]=s24(50000);
      it.fields["Y"]=s24(-30000);
      rec.items["042"]=it; }

    // Calculated velocity (signed 16-bit)
    { DecodedItem it;
      it.fields["VX"]=s16(1000);
      it.fields["VY"]=s16(-500);
      rec.items["202"]=it; }

    // Calculated acceleration (signed 8-bit)
    { DecodedItem it;
      it.fields["AX"]=s8(4);
      it.fields["AY"]=s8(-8);
      rec.items["210"]=it; }

    // Communications capability
    { DecodedItem it;
      it.fields["COM"]=1; it.fields["STAT"]=0; it.fields["CASEVN"]=0;
      it.fields["MSSC"]=1; it.fields["ARC"]=1; it.fields["AIC"]=1;
      it.fields["B1A"]=1; it.fields["B1B"]=0xA;
      rec.items["230"]=it; }

    // Target identification
    { DecodedItem it;
      it.fields["STI"]=2;
      it.fields["CHR"]=0xABCDEF012345ULL;
      rec.items["245"]=it; }

    // ACAS RA (56-bit raw)
    { DecodedItem it; it.fields["RA"]=0x01020304050607ULL; rec.items["260"]=it; }

    // Heights
    { DecodedItem it; it.fields["HGT"]=s16(4000); rec.items["105"]=it; }
    { DecodedItem it; it.fields["HGT"]=s16(3800); rec.items["110"]=it; }

    // Mode-2 and Mode-1
    { DecodedItem it; it.fields["V"]=0; it.fields["G"]=0; it.fields["L"]=0; it.fields["MODE2"]=0x7FF; rec.items["050"]=it; }
    { DecodedItem it; it.fields["V"]=0; it.fields["G"]=0; it.fields["L"]=0; it.fields["MODE1"]=0x1F; rec.items["055"]=it; }

    auto encoded = codec.encode(20, {rec});
    hexdump(encoded, "MoreFixed encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("041").fields.at("LAT") == s32(1073741824),      "041/LAT round-trip");
    CHECK(items.at("041").fields.at("LON") == s32(-536870912),      "041/LON round-trip");
    CHECK(items.at("042").fields.at("X")   == s24(50000),           "042/X = 50000");
    CHECK(items.at("042").fields.at("Y")   == s24(-30000),          "042/Y = -30000");
    CHECK(items.at("202").fields.at("VX")  == s16(1000),            "202/VX = 1000");
    CHECK(items.at("202").fields.at("VY")  == s16(-500),            "202/VY = -500");
    CHECK(items.at("210").fields.at("AX")  == s8(4),                "210/AX = 4");
    CHECK(items.at("210").fields.at("AY")  == s8(-8),               "210/AY = -8");
    CHECK(items.at("230").fields.at("COM") == 1,                    "230/COM = 1");
    CHECK(items.at("230").fields.at("ARC") == 1,                    "230/ARC = 1 (25 ft)");
    CHECK(items.at("230").fields.at("B1B") == 0xA,                  "230/B1B = 0xA");
    CHECK(items.at("245").fields.at("STI") == 2,                    "245/STI = 2 (callsign)");
    CHECK(items.at("245").fields.at("CHR") == 0xABCDEF012345ULL,    "245/CHR round-trip");
    CHECK(items.at("260").fields.at("RA")  == 0x01020304050607ULL,  "260/RA round-trip");
    CHECK(items.at("105").fields.at("HGT") == s16(4000),            "105/HGT = 4000");
    CHECK(items.at("110").fields.at("HGT") == s16(3800),            "110/HGT = 3800");
    CHECK(items.at("050").fields.at("MODE2") == 0x7FF,              "050/MODE2 = 0x7FF");
    CHECK(items.at("055").fields.at("MODE1") == 0x1F,               "055/MODE1 = 0x1F");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip Extended I020 (3 octets)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtendedI020(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Extended I020 (3 octets) ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; rec.items["010"]=it; }
    { DecodedItem it; it.fields["TOD"]=1000; rec.items["140"]=it; }

    // I020: set fields from all 3 octets to ensure full encoding
    { DecodedItem it;
      // Octet 1
      it.fields["SSR"]=0; it.fields["MS"]=0; it.fields["HF"]=1;
      it.fields["VDL4"]=0; it.fields["UAT"]=1; it.fields["DME"]=0; it.fields["OT"]=0;
      // Octet 2
      it.fields["RAB"]=0; it.fields["SPI"]=1; it.fields["CHN"]=0;
      it.fields["GBS"]=1; it.fields["CRT"]=0; it.fields["SIM"]=0; it.fields["TST"]=0;
      // Octet 3 (CF must be non-zero to force 3rd octet emission)
      it.fields["CF"]=3;
      rec.items["020"]=it; }

    auto encoded = codec.encode(20, {rec});
    hexdump(encoded, "Extended I020 (3 octets) encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.count("020"), "I020 present");
    const auto& f = items.at("020").fields;
    CHECK(f.at("HF")  == 1, "020/HF = 1");
    CHECK(f.at("UAT") == 1, "020/UAT = 1");
    CHECK(f.at("SSR") == 0, "020/SSR = 0");
    CHECK(f.at("SPI") == 1, "020/SPI = 1");
    CHECK(f.at("GBS") == 1, "020/GBS = 1");
    CHECK(f.at("SIM") == 0, "020/SIM = 0");
    CHECK(f.at("CF")  == 3, "020/CF = 3 (info not available)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip Extended I170 (2 octets)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtendedI170(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Extended I170 (2 octets) ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; rec.items["010"]=it; }
    { DecodedItem it; it.fields["TOD"]=5000; rec.items["140"]=it; }

    // I170: set GHO=1 (second octet) to force 2-octet encoding
    { DecodedItem it;
      it.fields["CNF"]=1; it.fields["TRE"]=0; it.fields["CST"]=1;
      it.fields["CDM"]=1; it.fields["MAH"]=0; it.fields["STH"]=1;
      it.fields["GHO"]=1;
      rec.items["170"]=it; }

    auto encoded = codec.encode(20, {rec});
    hexdump(encoded, "Extended I170 (2 octets) encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.count("170"), "I170 present");
    const auto& f = items.at("170").fields;
    CHECK(f.at("CNF") == 1, "170/CNF = 1 (initiation)");
    CHECK(f.at("CST") == 1, "170/CST = 1 (coasted)");
    CHECK(f.at("CDM") == 1, "170/CDM = 1 (climbing)");
    CHECK(f.at("STH") == 1, "170/STH = 1 (smoothed)");
    CHECK(f.at("TRE") == 0, "170/TRE = 0");
    CHECK(f.at("GHO") == 1, "170/GHO = 1 (ghost track)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip Repetitive FX I030
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripRepetitiveFX(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Repetitive FX I030 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; rec.items["010"]=it; }
    { DecodedItem it; it.fields["TOD"]=100; rec.items["140"]=it; }

    // I030: three warning codes
    DecodedItem i030;
    i030.repetitions = {0x01, 0x0B, 0x10};  // Multipath, Non-Matching Mode-3/A, Duplicated address
    rec.items["030"] = i030;

    auto encoded = codec.encode(20, {rec});
    hexdump(encoded, "Repetitive FX I030 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.count("030"), "I030 present");
    const auto& reps = items.at("030").repetitions;
    CHECK(reps.size() == 3,  "030 has 3 repetitions");
    CHECK(reps[0] == 0x01,   "030 rep[0] = 0x01 (Multipath)");
    CHECK(reps[1] == 0x0B,   "030 rep[1] = 0x0B (Non-Matching Mode-3/A)");
    CHECK(reps[2] == 0x10,   "030 rep[2] = 0x10 (Duplicated address)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Round-trip RepetitiveGroup I250 and I400
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripRepGroups(Codec& codec) {
    std::cout << "\n=== Test: Round-trip RepetitiveGroup I250 and I400 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; rec.items["010"]=it; }
    { DecodedItem it; it.fields["TOD"]=200; rec.items["140"]=it; }

    // I250: 2 BDS register entries (64-bit groups: BDSDATA(56)+BDS1(4)+BDS2(4))
    DecodedItem i250;
    i250.group_repetitions.push_back({{"BDSDATA", 0x20202020202020ULL}, {"BDS1", 2}, {"BDS2", 0}});
    i250.group_repetitions.push_back({{"BDSDATA", 0x01020304050607ULL}, {"BDS1", 3}, {"BDS2", 0}});
    rec.items["250"] = i250;

    // I400: 2 byte groups – each has 8 1-bit flags
    DecodedItem i400;
    i400.group_repetitions.push_back({{"BIT1",1},{"BIT2",0},{"BIT3",1},{"BIT4",0},
                                      {"BIT5",0},{"BIT6",0},{"BIT7",0},{"BIT8",0}});
    i400.group_repetitions.push_back({{"BIT1",0},{"BIT2",0},{"BIT3",0},{"BIT4",0},
                                      {"BIT5",1},{"BIT6",0},{"BIT7",1},{"BIT8",0}});
    rec.items["400"] = i400;

    auto encoded = codec.encode(20, {rec});
    hexdump(encoded, "RepGroups encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;

    CHECK(items.count("250"), "I250 present");
    const auto& gr250 = items.at("250").group_repetitions;
    CHECK(gr250.size() == 2,                                "250 has 2 entries");
    CHECK(gr250[0].at("BDSDATA") == 0x20202020202020ULL,    "250 rep[0] BDSDATA round-trip");
    CHECK(gr250[0].at("BDS1")    == 2,                      "250 rep[0] BDS1 = 2");
    CHECK(gr250[0].at("BDS2")    == 0,                      "250 rep[0] BDS2 = 0");
    CHECK(gr250[1].at("BDSDATA") == 0x01020304050607ULL,    "250 rep[1] BDSDATA round-trip");
    CHECK(gr250[1].at("BDS1")    == 3,                      "250 rep[1] BDS1 = 3");

    CHECK(items.count("400"), "I400 present");
    const auto& gr400 = items.at("400").group_repetitions;
    CHECK(gr400.size() == 2,           "400 has 2 entries");
    CHECK(gr400[0].at("BIT1") == 1,    "400 rep[0] BIT1 = 1");
    CHECK(gr400[0].at("BIT3") == 1,    "400 rep[0] BIT3 = 1");
    CHECK(gr400[0].at("BIT2") == 0,    "400 rep[0] BIT2 = 0");
    CHECK(gr400[1].at("BIT5") == 1,    "400 rep[1] BIT5 = 1");
    CHECK(gr400[1].at("BIT7") == 1,    "400 rep[1] BIT7 = 1");
    CHECK(gr400[1].at("BIT1") == 0,    "400 rep[1] BIT1 = 0");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 9: Round-trip Compound I500 (Position Accuracy)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCompoundI500(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Compound I500 (Position Accuracy) ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=2; it.fields["SIC"]=3; rec.items["010"]=it; }
    { DecodedItem it; it.fields["TOD"]=500; rec.items["140"]=it; }

    // I500: DOP + SDP + SDH
    DecodedItem i500;
    i500.compound_sub_fields["DOP"]["X"]  = 100;
    i500.compound_sub_fields["DOP"]["Y"]  = 100;
    i500.compound_sub_fields["DOP"]["XY"] = 50;
    i500.compound_sub_fields["SDP"]["X"]  = 200;
    i500.compound_sub_fields["SDP"]["Y"]  = 200;
    i500.compound_sub_fields["SDP"]["XY"] = 80;
    i500.compound_sub_fields["SDH"]["SDH"] = 50;
    rec.items["500"] = i500;

    auto encoded = codec.encode(20, {rec});
    hexdump(encoded, "Compound I500 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& sf = blk.records[0].items.at("500").compound_sub_fields;
    CHECK(sf.count("DOP"),                 "500/DOP present");
    CHECK(sf.count("SDP"),                 "500/SDP present");
    CHECK(sf.count("SDH"),                 "500/SDH present");
    CHECK(sf.at("DOP").at("X")  == 100,    "500/DOP/X = 100");
    CHECK(sf.at("DOP").at("Y")  == 100,    "500/DOP/Y = 100");
    CHECK(sf.at("DOP").at("XY") == 50,     "500/DOP/XY = 50");
    CHECK(sf.at("SDP").at("X")  == 200,    "500/SDP/X = 200");
    CHECK(sf.at("SDP").at("Y")  == 200,    "500/SDP/Y = 200");
    CHECK(sf.at("SDP").at("XY") == 80,     "500/SDP/XY = 80");
    CHECK(sf.at("SDH").at("SDH") == 50,    "500/SDH = 50 (25m)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 10: Full round-trip – complete multilateration target report
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip multilateration target report ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=0; it.fields["SIC"]=7; rec.items["010"]=it; }
    { DecodedItem it; it.fields["TOD"]=36000u*128u; rec.items["140"]=it; }

    // I020: 2 octets (MS=0, SPI=1 → forces octet 2)
    { DecodedItem it;
      it.fields["SSR"]=0; it.fields["MS"]=0; it.fields["HF"]=0;
      it.fields["VDL4"]=0; it.fields["UAT"]=0; it.fields["DME"]=0; it.fields["OT"]=0;
      it.fields["RAB"]=0; it.fields["SPI"]=1; it.fields["CHN"]=0;
      it.fields["GBS"]=0; it.fields["CRT"]=0; it.fields["SIM"]=0; it.fields["TST"]=0;
      rec.items["020"]=it; }

    // I041: WGS-84 position
    { DecodedItem it;
      it.fields["LAT"]=s32(1503238554); // ~45°
      it.fields["LON"]=s32(251539478);  // ~7.5°
      rec.items["041"]=it; }

    // I170: track status (GHO=1 forces 2nd octet)
    { DecodedItem it;
      it.fields["CNF"]=0; it.fields["TRE"]=0; it.fields["CST"]=0;
      it.fields["CDM"]=1; it.fields["MAH"]=0; it.fields["STH"]=1;
      it.fields["GHO"]=1;
      rec.items["170"]=it; }

    { DecodedItem it; it.fields["TRN"]=0x1F5; rec.items["161"]=it; }
    { DecodedItem it; it.fields["V"]=0; it.fields["G"]=0; it.fields["L"]=0; it.fields["MODE3A"]=0xABC; rec.items["070"]=it; }
    { DecodedItem it; it.fields["V"]=0; it.fields["G"]=0; it.fields["FL"]=600; rec.items["090"]=it; }
    { DecodedItem it; it.fields["TGT"]=0xABCDEFu; rec.items["220"]=it; }

    { DecodedItem it;
      it.fields["VX"]=s16(500); it.fields["VY"]=s16(-200);
      rec.items["202"]=it; }

    // I030: one warning code
    DecodedItem i030;
    i030.repetitions = {0x01};
    rec.items["030"] = i030;

    // I250: one BDS entry
    DecodedItem i250;
    i250.group_repetitions.push_back({{"BDSDATA", 0xCAFEBABEDEADULL}, {"BDS1", 2}, {"BDS2", 0}});
    rec.items["250"] = i250;

    auto encoded = codec.encode(20, {rec});
    hexdump(encoded, "Full round-trip encoded");
    CHECK(!encoded.empty(), "encode produced output");
    CHECK(encoded[0] == 0x14, "CAT byte = 0x14 (20)");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("SAC") == 0,               "010/SAC = 0");
    CHECK(items.at("010").fields.at("SIC") == 7,               "010/SIC = 7");
    CHECK(items.at("140").fields.at("TOD") == 36000u*128u,     "140/TOD = 10h");
    CHECK(items.count("020"),                                   "I020 present");
    CHECK(items.at("020").fields.at("SPI") == 1,               "020/SPI = 1");
    CHECK(items.at("041").fields.at("LAT") == s32(1503238554), "041/LAT round-trip");
    CHECK(items.at("041").fields.at("LON") == s32(251539478),  "041/LON round-trip");
    CHECK(items.count("170"),                                   "I170 present");
    CHECK(items.at("170").fields.at("CDM") == 1,               "170/CDM = 1 (climbing)");
    CHECK(items.at("170").fields.at("STH") == 1,               "170/STH = 1");
    CHECK(items.at("170").fields.at("GHO") == 1,               "170/GHO = 1 (ghost)");
    CHECK(items.at("161").fields.at("TRN") == 0x1F5,           "161/TRN = 0x1F5");
    CHECK(items.at("070").fields.at("MODE3A") == 0xABC,        "070/MODE3A = 0xABC");
    CHECK(items.at("090").fields.at("FL")  == 600,             "090/FL = 600 (FL150)");
    CHECK(items.at("220").fields.at("TGT") == 0xABCDEFu,       "220/TGT = 0xABCDEF");
    CHECK(items.at("202").fields.at("VX")  == s16(500),        "202/VX = 500");
    CHECK(items.at("202").fields.at("VY")  == s16(-200),       "202/VY = -200");
    CHECK(items.count("030"),                                   "I030 present");
    CHECK(items.at("030").repetitions.size() == 1,             "030 has 1 repetition");
    CHECK(items.at("030").repetitions[0] == 0x01,              "030/WE = 1 (Multipath)");
    CHECK(items.count("250"),                                   "I250 present");
    CHECK(items.at("250").group_repetitions.size() == 1,        "250 has 1 entry");
    CHECK(items.at("250").group_repetitions[0].at("BDS1") == 2, "250/BDS1 = 2");
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = (argc > 1) ? argv[1] : "specs/CAT20.xml";

    Codec codec;
    testSpecLoad(codec, spec_path);
    testDecodeBasicMessage(codec);
    testRoundTripCoreFixed(codec);
    testRoundTripMoreFixed(codec);
    testRoundTripExtendedI020(codec);
    testRoundTripExtendedI170(codec);
    testRoundTripRepetitiveFX(codec);
    testRoundTripRepGroups(codec);
    testRoundTripCompoundI500(codec);
    testFullRoundTrip(codec);

    std::cout << "\n=== Results: "
              << (failures == 0 ? "ALL PASSED" : std::to_string(failures) + " FAILED")
              << " ===\n";
    return failures == 0 ? 0 : 1;
}
