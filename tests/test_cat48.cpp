// test_cat48.cpp – Tests for CAT48 decode/encode round-trip.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat48

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

// ─────────────────────────────────────────────────────────────────────────────
//  Test 1: XML spec loads without error; item types and UAP are correct
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT48 spec load ===\n";
    try {
        CategoryDef cat = loadSpec(spec_path);
        CHECK(cat.cat == 48,           "cat number = 48");
        CHECK(cat.items.count("010"),  "item 010 present");
        CHECK(cat.items.count("020"),  "item 020 present");
        CHECK(cat.items.count("030"),  "item 030 present");
        CHECK(cat.items.count("040"),  "item 040 present");
        CHECK(cat.items.count("042"),  "item 042 present");
        CHECK(cat.items.count("050"),  "item 050 present");
        CHECK(cat.items.count("055"),  "item 055 present");
        CHECK(cat.items.count("060"),  "item 060 present");
        CHECK(cat.items.count("065"),  "item 065 present");
        CHECK(cat.items.count("070"),  "item 070 present");
        CHECK(cat.items.count("080"),  "item 080 present");
        CHECK(cat.items.count("090"),  "item 090 present");
        CHECK(cat.items.count("100"),  "item 100 present");
        CHECK(cat.items.count("110"),  "item 110 present");
        CHECK(cat.items.count("120"),  "item 120 present");
        CHECK(cat.items.count("130"),  "item 130 present");
        CHECK(cat.items.count("140"),  "item 140 present");
        CHECK(cat.items.count("161"),  "item 161 present");
        CHECK(cat.items.count("170"),  "item 170 present");
        CHECK(cat.items.count("200"),  "item 200 present");
        CHECK(cat.items.count("210"),  "item 210 present");
        CHECK(cat.items.count("220"),  "item 220 present");
        CHECK(cat.items.count("230"),  "item 230 present");
        CHECK(cat.items.count("240"),  "item 240 present");
        CHECK(cat.items.count("250"),  "item 250 present");
        CHECK(cat.items.count("260"),  "item 260 present");
        CHECK(cat.items.count("SP"),   "item SP present");
        CHECK(cat.items.count("RE"),   "item RE present");

        // UAP
        CHECK(cat.uap_variations.count("default"), "UAP variation 'default' exists");
        CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
        CHECK(cat.uap_variations.at("default").size() == 28, "UAP has 28 slots");

        // Item type assertions
        CHECK(cat.items.at("010").type == ItemType::Fixed,           "010 is Fixed");
        CHECK(cat.items.at("020").type == ItemType::Extended,        "020 is Extended");
        CHECK(cat.items.at("030").type == ItemType::Repetitive,      "030 is Repetitive");
        CHECK(cat.items.at("040").type == ItemType::Fixed,           "040 is Fixed");
        CHECK(cat.items.at("042").type == ItemType::Fixed,           "042 is Fixed");
        CHECK(cat.items.at("050").type == ItemType::Fixed,           "050 is Fixed");
        CHECK(cat.items.at("055").type == ItemType::Fixed,           "055 is Fixed");
        CHECK(cat.items.at("060").type == ItemType::Fixed,           "060 is Fixed");
        CHECK(cat.items.at("065").type == ItemType::Fixed,           "065 is Fixed");
        CHECK(cat.items.at("070").type == ItemType::Fixed,           "070 is Fixed");
        CHECK(cat.items.at("080").type == ItemType::Fixed,           "080 is Fixed");
        CHECK(cat.items.at("090").type == ItemType::Fixed,           "090 is Fixed");
        CHECK(cat.items.at("100").type == ItemType::Fixed,           "100 is Fixed");
        CHECK(cat.items.at("110").type == ItemType::Fixed,           "110 is Fixed");
        CHECK(cat.items.at("120").type == ItemType::Compound,        "120 is Compound");
        CHECK(cat.items.at("130").type == ItemType::Compound,        "130 is Compound");
        CHECK(cat.items.at("140").type == ItemType::Fixed,           "140 is Fixed");
        CHECK(cat.items.at("161").type == ItemType::Fixed,           "161 is Fixed");
        CHECK(cat.items.at("170").type == ItemType::Extended,        "170 is Extended");
        CHECK(cat.items.at("200").type == ItemType::Fixed,           "200 is Fixed");
        CHECK(cat.items.at("210").type == ItemType::Fixed,           "210 is Fixed");
        CHECK(cat.items.at("220").type == ItemType::Fixed,           "220 is Fixed");
        CHECK(cat.items.at("230").type == ItemType::Fixed,           "230 is Fixed");
        CHECK(cat.items.at("240").type == ItemType::Fixed,           "240 is Fixed");
        CHECK(cat.items.at("250").type == ItemType::RepetitiveGroup, "250 is RepetitiveGroup");
        CHECK(cat.items.at("260").type == ItemType::Fixed,           "260 is Fixed");
        CHECK(cat.items.at("SP").type  == ItemType::SP,              "SP is SP/Explicit");
        CHECK(cat.items.at("RE").type  == ItemType::SP,              "RE is SP/Explicit");

        // Fixed item byte sizes
        CHECK(cat.items.at("010").fixed_bytes == 2,  "010 = 2 bytes");
        CHECK(cat.items.at("040").fixed_bytes == 4,  "040 = 4 bytes");
        CHECK(cat.items.at("042").fixed_bytes == 4,  "042 = 4 bytes");
        CHECK(cat.items.at("050").fixed_bytes == 2,  "050 = 2 bytes");
        CHECK(cat.items.at("055").fixed_bytes == 1,  "055 = 1 byte");
        CHECK(cat.items.at("060").fixed_bytes == 2,  "060 = 2 bytes");
        CHECK(cat.items.at("065").fixed_bytes == 1,  "065 = 1 byte");
        CHECK(cat.items.at("070").fixed_bytes == 2,  "070 = 2 bytes");
        CHECK(cat.items.at("080").fixed_bytes == 2,  "080 = 2 bytes");
        CHECK(cat.items.at("090").fixed_bytes == 2,  "090 = 2 bytes");
        CHECK(cat.items.at("100").fixed_bytes == 4,  "100 = 4 bytes");
        CHECK(cat.items.at("110").fixed_bytes == 2,  "110 = 2 bytes");
        CHECK(cat.items.at("140").fixed_bytes == 3,  "140 = 3 bytes");
        CHECK(cat.items.at("161").fixed_bytes == 2,  "161 = 2 bytes");
        CHECK(cat.items.at("200").fixed_bytes == 4,  "200 = 4 bytes");
        CHECK(cat.items.at("210").fixed_bytes == 4,  "210 = 4 bytes");
        CHECK(cat.items.at("220").fixed_bytes == 3,  "220 = 3 bytes");
        CHECK(cat.items.at("230").fixed_bytes == 2,  "230 = 2 bytes");
        CHECK(cat.items.at("240").fixed_bytes == 6,  "240 = 6 bytes");
        CHECK(cat.items.at("260").fixed_bytes == 7,  "260 = 7 bytes");

        // I020 Extended: 6 octets
        CHECK(cat.items.at("020").octets.size() == 6,  "020 has 6 octets");
        // I170 Extended: 2 octets
        CHECK(cat.items.at("170").octets.size() == 2,  "170 has 2 octets");

        // I250 RepetitiveGroup: 64-bit group (MBDATA 56 + BDS1 4 + BDS2 4)
        CHECK(cat.items.at("250").rep_group_bits == 64, "250 group = 64 bits (8 bytes)");

        // I130 Compound: 7 sub-items
        CHECK(cat.items.at("130").compound_sub_items.size() == 7, "130 has 7 sub-items");
        const auto& si130 = cat.items.at("130").compound_sub_items;
        CHECK(si130[0].name == "SRL" && si130[0].fixed_bytes == 1, "130.SRL = 1 byte");
        CHECK(si130[1].name == "SRR" && si130[1].fixed_bytes == 1, "130.SRR = 1 byte");
        CHECK(si130[2].name == "SAM" && si130[2].fixed_bytes == 1, "130.SAM = 1 byte");
        CHECK(si130[3].name == "PRL" && si130[3].fixed_bytes == 1, "130.PRL = 1 byte");
        CHECK(si130[4].name == "PAM" && si130[4].fixed_bytes == 1, "130.PAM = 1 byte");
        CHECK(si130[5].name == "RPD" && si130[5].fixed_bytes == 1, "130.RPD = 1 byte");
        CHECK(si130[6].name == "APD" && si130[6].fixed_bytes == 1, "130.APD = 1 byte");

        // I120 Compound: 2 sub-items (CAL + unused)
        CHECK(cat.items.at("120").compound_sub_items.size() == 2, "120 has 2 sub-items");
        const auto& si120 = cat.items.at("120").compound_sub_items;
        CHECK(si120[0].name == "CAL" && si120[0].fixed_bytes == 2, "120.CAL = 2 bytes");
        CHECK(si120[1].name == "-",                                 "120 slot1 = unused (RDS)");

        codec.registerCategory(std::move(cat));
    } catch (const std::exception& e) {
        std::cerr << "FAIL spec load: " << e.what() << '\n';
        ++failures;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode a hand-crafted basic target report
//
//  Items: I010, I140, I020, I040, I070, I090
//  All in FSPEC byte 1 (slots 1-6 set, slot 7 clear):
//
//  FSPEC: bit7=1(010), bit6=1(140), bit5=1(020), bit4=1(040),
//         bit3=1(070), bit2=1(090), bit1=0, bit0=0(FX=0) → 0xFC
//
//  I048/010: SAC=10, SIC=1 → 0x0A, 0x01
//  I048/140: TOD=43200s → raw=43200*128=5529600=0x546000 → 0x54, 0x60, 0x00
//  I048/020: TYP=2(Single SSR), SIM=0, RDP=0, SPI=0, RAB=0, FX=0
//            = 010 0000 | FX=0 → 0x40
//  I048/040: RHO=100NM → raw=100/0.00390625=25600=0x6400
//            THETA=90° → raw=90/0.0054931640625=16384=0x4000
//            → 0x64,0x00,0x40,0x00
//  I048/070: V=0,G=0,L=0,spare=0, MODE3A=octal 2345=1253=0x4E5
//            → 0x00|0x04, 0xE5 → 0x04, 0xE5
//  I048/090: V=0,G=0, FL=350FL → raw=350*4=1400=0x578 in 14 bits
//            16-bit word: 0x0578 → 0x05, 0x78
//
//  Total: 3(hdr) + 1(fspec) + 2 + 3 + 1 + 4 + 2 + 2 = 18 bytes
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicTargetReport(const Codec& codec) {
    std::cout << "\n=== Test: Decode CAT48 basic target report ===\n";

    std::vector<uint8_t> frame = {
        0x30,              // CAT=48
        0x00, 0x12,        // LEN=18
        0xFC,              // FSPEC: I010,I140,I020,I040,I070,I090; FX=0
        0x0A, 0x01,        // I048/010: SAC=10, SIC=1
        0x54, 0x60, 0x00,  // I048/140: TOD raw=5529600 (43200.0 s)
        0x40,              // I048/020: TYP=2 (Single SSR), FX=0
        0x64, 0x00,        // I048/040: RHO=25600 raw (100.0 NM)
        0x40, 0x00,        // I048/040: THETA=16384 raw (90.0 deg)
        0x04, 0xE5,        // I048/070: V=0,G=0,L=0,spare=0, MODE3A=0x4E5 (2345 oct)
        0x05, 0x78,        // I048/090: V=0,G=0, FL=1400 raw (350.0 FL)
    };

    hexdump(frame, "input");
    DecodedBlock block = codec.decode(frame);

    CHECK(block.valid,        "block.valid");
    CHECK(block.cat == 48,    "block.cat == 48");
    CHECK(block.length == 18, "block.length == 18");
    CHECK(block.records.size() == 1, "one record");

    if (block.records.empty()) return;
    const auto& rec = block.records[0];
    CHECK(rec.valid,                       "record.valid");
    CHECK(rec.uap_variation == "default",  "UAP = default");

    CHECK(rec.items.count("010"), "I010 present");
    CHECK(rec.items.count("140"), "I140 present");
    CHECK(rec.items.count("020"), "I020 present");
    CHECK(rec.items.count("040"), "I040 present");
    CHECK(rec.items.count("070"), "I070 present");
    CHECK(rec.items.count("090"), "I090 present");
    CHECK(!rec.items.count("220"), "I220 absent");
    CHECK(!rec.items.count("230"), "I230 absent");

    if (rec.items.count("010")) {
        CHECK(rec.items.at("010").fields.at("SAC") == 10, "SAC == 10");
        CHECK(rec.items.at("010").fields.at("SIC") == 1,  "SIC == 1");
    }
    if (rec.items.count("140"))
        CHECK(rec.items.at("140").fields.at("TOD") == 5529600, "TOD raw == 5529600");
    if (rec.items.count("020"))
        CHECK(rec.items.at("020").fields.at("TYP") == 2, "I020.TYP == 2 (Single SSR)");
    if (rec.items.count("040")) {
        CHECK(rec.items.at("040").fields.at("RHO")   == 25600, "RHO raw == 25600");
        CHECK(rec.items.at("040").fields.at("THETA") == 16384, "THETA raw == 16384");
    }
    if (rec.items.count("070"))
        CHECK(rec.items.at("070").fields.at("MODE3A") == 0x4E5, "MODE3A == 0x4E5");
    if (rec.items.count("090"))
        CHECK(rec.items.at("090").fields.at("FL") == 1400, "FL raw == 1400 (350.0 FL)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip for I048/020 Extended (first two octets)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtended020(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip I048/020 Extended (2 octets) ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    // I010
    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 5; di.fields["SIC"] = 9;
      src.items["010"] = std::move(di); }

    // I140
    { DecodedItem di; di.item_id = "140"; di.type = ItemType::Fixed;
      di.fields["TOD"] = 6400; // 50.0 s
      src.items["140"] = std::move(di); }

    // I020: TYP=5 (ModeS Roll-Call), SIM=0, RDP=1, SPI=0, RAB=0
    //        TST=0, ERR=0, XPP=1, ME=0, MI=0, FOEFRI=1
    { DecodedItem di; di.item_id = "020"; di.type = ItemType::Extended;
      di.fields["TYP"]    = 5;
      di.fields["SIM"]    = 0;
      di.fields["RDP"]    = 1;
      di.fields["SPI"]    = 0;
      di.fields["RAB"]    = 0;
      di.fields["TST"]    = 0;
      di.fields["ERR"]    = 0;
      di.fields["XPP"]    = 1;
      di.fields["ME"]     = 0;
      di.fields["MI"]     = 0;
      di.fields["FOEFRI"] = 1;
      src.items["020"] = std::move(di); }

    auto encoded = codec.encode(48, {src});
    hexdump(encoded, "encoded");
    CHECK(encoded.size() >= 3, "encoded non-empty");

    auto block = codec.decode(encoded);
    CHECK(block.valid, "RT block valid");
    CHECK(block.cat == 48, "RT cat == 48");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");
    CHECK(rec.items.count("020"), "RT I020 present");

    if (rec.items.count("020")) {
        CHECK(rec.items.at("020").fields.at("TYP")    == 5, "RT TYP == 5");
        CHECK(rec.items.at("020").fields.at("RDP")    == 1, "RT RDP == 1");
        CHECK(rec.items.at("020").fields.at("XPP")    == 1, "RT XPP == 1");
        CHECK(rec.items.at("020").fields.at("FOEFRI") == 1, "RT FOEFRI == 1");
        CHECK(rec.items.at("020").fields.at("SIM")    == 0, "RT SIM == 0");
        CHECK(rec.items.at("020").fields.at("ERR")    == 0, "RT ERR == 0");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip for I048/170 Extended (Track Status)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtended170(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip I048/170 Extended (Track Status) ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 1; di.fields["SIC"] = 1;
      src.items["010"] = std::move(di); }

    { DecodedItem di; di.item_id = "140"; di.type = ItemType::Fixed;
      di.fields["TOD"] = 1000;
      src.items["140"] = std::move(di); }

    // I170: CNF=0 (confirmed), RAD=2 (SSR/ModeS), DOU=0, MAH=1, CDM=1 (climbing)
    //   second octet: TRE=0, GHO=0, SUP=1, TCC=1
    { DecodedItem di; di.item_id = "170"; di.type = ItemType::Extended;
      di.fields["CNF"] = 0;
      di.fields["RAD"] = 2;
      di.fields["DOU"] = 0;
      di.fields["MAH"] = 1;
      di.fields["CDM"] = 1;
      di.fields["TRE"] = 0;
      di.fields["GHO"] = 0;
      di.fields["SUP"] = 1;
      di.fields["TCC"] = 1;
      src.items["170"] = std::move(di); }

    auto encoded = codec.encode(48, {src});
    hexdump(encoded, "encoded");

    auto block = codec.decode(encoded);
    CHECK(block.valid, "RT block valid");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");
    CHECK(rec.items.count("170"), "RT I170 present");

    if (rec.items.count("170")) {
        CHECK(rec.items.at("170").fields.at("CNF") == 0, "RT CNF == 0 (confirmed)");
        CHECK(rec.items.at("170").fields.at("RAD") == 2, "RT RAD == 2 (SSR/ModeS)");
        CHECK(rec.items.at("170").fields.at("MAH") == 1, "RT MAH == 1");
        CHECK(rec.items.at("170").fields.at("CDM") == 1, "RT CDM == 1 (climbing)");
        CHECK(rec.items.at("170").fields.at("SUP") == 1, "RT SUP == 1");
        CHECK(rec.items.at("170").fields.at("TCC") == 1, "RT TCC == 1");
        CHECK(rec.items.at("170").fields.at("TRE") == 0, "RT TRE == 0");
        CHECK(rec.items.at("170").fields.at("GHO") == 0, "RT GHO == 0");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Decode I048/030 (Repetitive FX warning codes)
//
//  Warning codes 1, 15, 23 encoded as FX-terminated bytes:
//    code=1,  FX=1 → (1<<1)|1  = 0x03
//    code=15, FX=1 → (15<<1)|1 = 0x1F
//    code=23, FX=0 → (23<<1)|0 = 0x2E
//
//  Items: I010, I030  (slots 1 and 16)
//  FSPEC byte 1: bit7=1(I010), rest=0, FX=1 → 0x81
//  FSPEC byte 2: bit7=0..bit2=0, bit1=0..  → slot16=I030 is in byte3
//  Actually: slot 16 → fspec byte (16-1)/7 = 15/7 = 2 (0-indexed), bit = 7-((16-1)%7) = 7-1 = 6
//  Wait: FSPEC byte index = (slot-1)/7, bit = 7 - ((slot-1)%7)
//  slot 16: (16-1)/7 = 15/7 = 2 (byte index 2), (15%7)=1, bit=7-1=6 → FSPEC byte 3, bit 6
//
//  FSPEC byte 1: I010(bit7)=1, FX=1 → 0x81
//  FSPEC byte 2: all zeros, FX=1    → 0x01
//  FSPEC byte 3: I030(bit6)=1, FX=0 → 0x40
//
//  Total: 3(hdr) + 3(fspec) + 2(I010) + 3(I030) = 11 bytes
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeWarningCodes(const Codec& codec) {
    std::cout << "\n=== Test: Decode I048/030 Repetitive FX warning codes ===\n";

    std::vector<uint8_t> frame = {
        0x30,              // CAT=48
        0x00, 0x0B,        // LEN=11
        0x81,              // FSPEC byte 1: I010(bit7)=1, FX=1
        0x01,              // FSPEC byte 2: all zeros, FX=1
        0x40,              // FSPEC byte 3: I030(bit6)=1, FX=0
        0x0A, 0x01,        // I010: SAC=10, SIC=1
        0x03,              // I030: code=1, FX=1
        0x1F,              // I030: code=15, FX=1
        0x2E,              // I030: code=23, FX=0
    };

    hexdump(frame, "input");
    DecodedBlock block = codec.decode(frame);

    CHECK(block.valid,        "block.valid");
    CHECK(block.cat == 48,    "block.cat == 48");
    CHECK(block.length == 11, "block.length == 11");
    CHECK(block.records.size() == 1, "one record");

    if (block.records.empty()) return;
    const auto& rec = block.records[0];
    CHECK(rec.valid,               "record.valid");
    CHECK(rec.items.count("030"), "I030 present");
    CHECK(!rec.items.count("020"), "I020 absent");

    if (rec.items.count("030")) {
        const auto& reps = rec.items.at("030").repetitions;
        CHECK(reps.size() == 3,  "I030 has 3 repetitions");
        if (reps.size() >= 3) {
            CHECK(reps[0] == 1,  "reps[0] == 1");
            CHECK(reps[1] == 15, "reps[1] == 15");
            CHECK(reps[2] == 23, "reps[2] == 23");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip for I048/250 BDS Register Data (RepetitiveGroup)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripBDSRegisterData(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip I048/250 BDS Register Data ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 3; di.fields["SIC"] = 7;
      src.items["010"] = std::move(di); }

    { DecodedItem di; di.item_id = "140"; di.type = ItemType::Fixed;
      di.fields["TOD"] = 10000;
      src.items["140"] = std::move(di); }

    { DecodedItem di; di.item_id = "220"; di.type = ItemType::Fixed;
      di.fields["ADR"] = 0xABCDEF;
      src.items["220"] = std::move(di); }

    // I250: 2 BDS register groups
    { DecodedItem di; di.item_id = "250"; di.type = ItemType::RepetitiveGroup;
      // Group 1: MBDATA=0x1122334455667 (masked to 56 bits), BDS1=2, BDS2=0
      di.group_repetitions.push_back({{"MBDATA", 0x11223344556677ULL}, {"BDS1", 2}, {"BDS2", 0}});
      // Group 2: MBDATA=0x0000000000000 (all zeros), BDS1=3, BDS2=0
      di.group_repetitions.push_back({{"MBDATA", 0ULL}, {"BDS1", 3}, {"BDS2", 0}});
      src.items["250"] = std::move(di); }

    auto encoded = codec.encode(48, {src});
    hexdump(encoded, "encoded");
    CHECK(encoded.size() >= 3, "encoded non-empty");

    auto block = codec.decode(encoded);
    CHECK(block.valid, "RT block valid");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");
    CHECK(rec.items.count("250"), "RT I250 present");

    if (rec.items.count("250")) {
        const auto& grps = rec.items.at("250").group_repetitions;
        CHECK(grps.size() == 2, "RT I250 has 2 groups");
        if (grps.size() >= 2) {
            CHECK(grps[0].at("MBDATA") == 0x11223344556677ULL, "RT grp[0].MBDATA");
            CHECK(grps[0].at("BDS1")   == 2, "RT grp[0].BDS1 == 2");
            CHECK(grps[0].at("BDS2")   == 0, "RT grp[0].BDS2 == 0");
            CHECK(grps[1].at("MBDATA") == 0, "RT grp[1].MBDATA == 0");
            CHECK(grps[1].at("BDS1")   == 3, "RT grp[1].BDS1 == 3");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip for I048/130 Compound (Radar Plot Characteristics)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripRadarPlotChar(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip I048/130 Radar Plot Characteristics ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 4; di.fields["SIC"] = 8;
      src.items["010"] = std::move(di); }

    { DecodedItem di; di.item_id = "140"; di.type = ItemType::Fixed;
      di.fields["TOD"] = 2000;
      src.items["140"] = std::move(di); }

    // I130: SRL=40 raw, SAM=(-50) raw, RPD=(-2) raw
    // SAM is a signed 8-bit field stored as raw uint64. -50 as int8 = 0xCE = 206.
    // RPD is signed 8-bit. -2 as int8 = 0xFE = 254.
    { DecodedItem di; di.item_id = "130"; di.type = ItemType::Compound;
      di.compound_sub_fields["SRL"] = {{"SRL", 40}};
      di.compound_sub_fields["SAM"] = {{"SAM", static_cast<uint64_t>(static_cast<int8_t>(-50))}};
      di.compound_sub_fields["RPD"] = {{"RPD", static_cast<uint64_t>(static_cast<int8_t>(-2))}};
      src.items["130"] = std::move(di); }

    auto encoded = codec.encode(48, {src});
    hexdump(encoded, "encoded");
    CHECK(encoded.size() >= 3, "encoded non-empty");

    auto block = codec.decode(encoded);
    CHECK(block.valid, "RT block valid");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");
    CHECK(rec.items.count("130"), "RT I130 present");

    if (rec.items.count("130")) {
        const auto& i130 = rec.items.at("130");
        CHECK(i130.compound_sub_fields.count("SRL"), "RT I130.SRL present");
        CHECK(i130.compound_sub_fields.count("SAM"), "RT I130.SAM present");
        CHECK(i130.compound_sub_fields.count("RPD"), "RT I130.RPD present");
        CHECK(!i130.compound_sub_fields.count("SRR"), "RT I130.SRR absent");
        CHECK(!i130.compound_sub_fields.count("PRL"), "RT I130.PRL absent");

        if (i130.compound_sub_fields.count("SRL"))
            CHECK(i130.compound_sub_fields.at("SRL").at("SRL") == 40, "RT SRL.SRL == 40");
        if (i130.compound_sub_fields.count("SAM"))
            CHECK(i130.compound_sub_fields.at("SAM").at("SAM") ==
                  static_cast<uint64_t>(static_cast<uint8_t>(-50)), "RT SAM.SAM == -50 raw");
        if (i130.compound_sub_fields.count("RPD"))
            CHECK(i130.compound_sub_fields.at("RPD").at("RPD") ==
                  static_cast<uint64_t>(static_cast<uint8_t>(-2)), "RT RPD.RPD == -2 raw");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Round-trip for I048/120 Compound – CAL sub-item only
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripDopplerCAL(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip I048/120 Compound (CAL only) ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 2; di.fields["SIC"] = 4;
      src.items["010"] = std::move(di); }

    { DecodedItem di; di.item_id = "140"; di.type = ItemType::Fixed;
      di.fields["TOD"] = 3000;
      src.items["140"] = std::move(di); }

    // I120.CAL: D=0 (valid), CAL=75 m/s
    { DecodedItem di; di.item_id = "120"; di.type = ItemType::Compound;
      di.compound_sub_fields["CAL"] = {{"D", 0}, {"CAL", 75}};
      src.items["120"] = std::move(di); }

    auto encoded = codec.encode(48, {src});
    hexdump(encoded, "encoded");
    CHECK(encoded.size() >= 3, "encoded non-empty");

    auto block = codec.decode(encoded);
    CHECK(block.valid, "RT block valid");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");
    CHECK(rec.items.count("120"), "RT I120 present");

    if (rec.items.count("120")) {
        const auto& i120 = rec.items.at("120");
        CHECK(i120.compound_sub_fields.count("CAL"), "RT I120.CAL present");
        if (i120.compound_sub_fields.count("CAL")) {
            CHECK(i120.compound_sub_fields.at("CAL").at("D")   == 0,  "RT CAL.D == 0");
            CHECK(i120.compound_sub_fields.at("CAL").at("CAL") == 75, "RT CAL.CAL == 75 m/s");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 9: Round-trip full Mode S record
//  Items: I010, I140, I020, I040, I070, I090, I220, I230, I240, I161, I170
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripModeSRecord(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip full Mode S target record ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 11; di.fields["SIC"] = 22;
      src.items["010"] = std::move(di); }

    { DecodedItem di; di.item_id = "140"; di.type = ItemType::Fixed;
      di.fields["TOD"] = 7680000; // 60000 s = 1000 min
      src.items["140"] = std::move(di); }

    // I020: TYP=4 (Single ModeS All-Call), SIM=0, RDP=0, SPI=0, RAB=0
    { DecodedItem di; di.item_id = "020"; di.type = ItemType::Extended;
      di.fields["TYP"] = 4; di.fields["SIM"] = 0; di.fields["RDP"] = 0;
      di.fields["SPI"] = 0; di.fields["RAB"] = 0;
      src.items["020"] = std::move(di); }

    { DecodedItem di; di.item_id = "040"; di.type = ItemType::Fixed;
      di.fields["RHO"] = 12800;  // 50 NM raw
      di.fields["THETA"] = 32768; // 180° raw
      src.items["040"] = std::move(di); }

    { DecodedItem di; di.item_id = "070"; di.type = ItemType::Fixed;
      di.fields["V"] = 0; di.fields["G"] = 0; di.fields["L"] = 0;
      di.fields["MODE3A"] = 0xFC0; // octal 7700 = squawk 7700
      src.items["070"] = std::move(di); }

    { DecodedItem di; di.item_id = "090"; di.type = ItemType::Fixed;
      di.fields["V"] = 0; di.fields["G"] = 0;
      di.fields["FL"] = 1480; // 370 FL * 4
      src.items["090"] = std::move(di); }

    // I220: 24-bit Mode S address
    { DecodedItem di; di.item_id = "220"; di.type = ItemType::Fixed;
      di.fields["ADR"] = 0x3C4A5B;
      src.items["220"] = std::move(di); }

    // I230: COM=1, STAT=0, SI=0, MSSC=1, ARC=1 (25ft), AIC=1, B1A=0, B1B=5
    { DecodedItem di; di.item_id = "230"; di.type = ItemType::Fixed;
      di.fields["COM"]  = 1; di.fields["STAT"] = 0; di.fields["SI"]   = 0;
      di.fields["MSSC"] = 1; di.fields["ARC"]  = 1; di.fields["AIC"]  = 1;
      di.fields["B1A"]  = 0; di.fields["B1B"]  = 5;
      src.items["230"] = std::move(di); }

    // I240: Aircraft ID raw (e.g. "BAW001 " encoded as 8 ICAO chars × 6 bits)
    { DecodedItem di; di.item_id = "240"; di.type = ItemType::Fixed;
      di.fields["IDENT"] = 0x0820A32040A0ULL; // example raw value
      src.items["240"] = std::move(di); }

    // I161: Track Number
    { DecodedItem di; di.item_id = "161"; di.type = ItemType::Fixed;
      di.fields["TRN"] = 1234;
      src.items["161"] = std::move(di); }

    // I170: CNF=0, RAD=2, DOU=0, MAH=0, CDM=0 (first octet only)
    { DecodedItem di; di.item_id = "170"; di.type = ItemType::Extended;
      di.fields["CNF"] = 0; di.fields["RAD"] = 2; di.fields["DOU"] = 0;
      di.fields["MAH"] = 0; di.fields["CDM"] = 0;
      src.items["170"] = std::move(di); }

    auto encoded = codec.encode(48, {src});
    hexdump(encoded, "encoded");
    CHECK(encoded.size() >= 3, "encoded non-empty");

    auto block = codec.decode(encoded);
    CHECK(block.valid,       "RT block valid");
    CHECK(block.cat == 48,   "RT cat == 48");
    CHECK(block.records.size() == 1, "RT one record");
    if (block.records.empty()) return;

    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");

    if (rec.items.count("010")) {
        CHECK(rec.items.at("010").fields.at("SAC") == 11, "RT SAC == 11");
        CHECK(rec.items.at("010").fields.at("SIC") == 22, "RT SIC == 22");
    } else { std::cerr << "FAIL RT: I010 missing\n"; ++failures; }

    if (rec.items.count("040")) {
        CHECK(rec.items.at("040").fields.at("RHO")   == 12800, "RT RHO == 12800");
        CHECK(rec.items.at("040").fields.at("THETA") == 32768, "RT THETA == 32768");
    } else { std::cerr << "FAIL RT: I040 missing\n"; ++failures; }

    if (rec.items.count("070"))
        CHECK(rec.items.at("070").fields.at("MODE3A") == 0xFC0, "RT MODE3A == 0xFC0 (7700 oct)");
    else { std::cerr << "FAIL RT: I070 missing\n"; ++failures; }

    if (rec.items.count("090"))
        CHECK(rec.items.at("090").fields.at("FL") == 1480, "RT FL == 1480 (370 FL)");
    else { std::cerr << "FAIL RT: I090 missing\n"; ++failures; }

    if (rec.items.count("220"))
        CHECK(rec.items.at("220").fields.at("ADR") == 0x3C4A5B, "RT ADR == 0x3C4A5B");
    else { std::cerr << "FAIL RT: I220 missing\n"; ++failures; }

    if (rec.items.count("230")) {
        CHECK(rec.items.at("230").fields.at("COM")  == 1, "RT COM == 1");
        CHECK(rec.items.at("230").fields.at("ARC")  == 1, "RT ARC == 1 (25ft)");
        CHECK(rec.items.at("230").fields.at("B1B")  == 5, "RT B1B == 5");
    } else { std::cerr << "FAIL RT: I230 missing\n"; ++failures; }

    if (rec.items.count("240"))
        CHECK(rec.items.at("240").fields.at("IDENT") == 0x0820A32040A0ULL, "RT IDENT matches");
    else { std::cerr << "FAIL RT: I240 missing\n"; ++failures; }

    if (rec.items.count("161"))
        CHECK(rec.items.at("161").fields.at("TRN") == 1234, "RT TRN == 1234");
    else { std::cerr << "FAIL RT: I161 missing\n"; ++failures; }

    if (rec.items.count("020"))
        CHECK(rec.items.at("020").fields.at("TYP") == 4, "RT TYP == 4 (ModeS All-Call)");
    else { std::cerr << "FAIL RT: I020 missing\n"; ++failures; }

    if (rec.items.count("170"))
        CHECK(rec.items.at("170").fields.at("RAD") == 2, "RT RAD == 2 (SSR/ModeS)");
    else { std::cerr << "FAIL RT: I170 missing\n"; ++failures; }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 10: Multi-record block (two records in one block)
// ─────────────────────────────────────────────────────────────────────────────
static void testMultiRecord(const Codec& codec) {
    std::cout << "\n=== Test: Multi-record block ===\n";

    DecodedRecord r1, r2;
    r1.uap_variation = r2.uap_variation = "default";

    // Record 1: SSR-only plot (TYP=2)
    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 1; di.fields["SIC"] = 2;
      r1.items["010"] = di; r2.items["010"] = std::move(di); }

    { DecodedItem di; di.item_id = "140"; di.type = ItemType::Fixed;
      di.fields["TOD"] = 500; r1.items["140"] = std::move(di); }

    { DecodedItem di; di.item_id = "020"; di.type = ItemType::Extended;
      di.fields["TYP"] = 2; r1.items["020"] = std::move(di); }

    { DecodedItem di; di.item_id = "040"; di.type = ItemType::Fixed;
      di.fields["RHO"] = 5120;   // 20 NM raw
      di.fields["THETA"] = 8192; // 45° raw
      r1.items["040"] = std::move(di); }

    // Record 2: Mode S roll-call (TYP=5) with aircraft address
    { DecodedItem di; di.item_id = "140"; di.type = ItemType::Fixed;
      di.fields["TOD"] = 510; r2.items["140"] = std::move(di); }

    { DecodedItem di; di.item_id = "020"; di.type = ItemType::Extended;
      di.fields["TYP"] = 5; r2.items["020"] = std::move(di); }

    { DecodedItem di; di.item_id = "040"; di.type = ItemType::Fixed;
      di.fields["RHO"] = 10240;  // 40 NM raw
      di.fields["THETA"] = 24576; // 135° raw
      r2.items["040"] = std::move(di); }

    { DecodedItem di; di.item_id = "220"; di.type = ItemType::Fixed;
      di.fields["ADR"] = 0xDEADBEu;
      r2.items["220"] = std::move(di); }

    auto encoded = codec.encode(48, {r1, r2});
    hexdump(encoded, "encoded");
    CHECK(encoded.size() >= 3, "encoded non-empty");

    auto block = codec.decode(encoded);
    CHECK(block.valid,       "multi-rec block valid");
    CHECK(block.cat == 48,   "multi-rec cat == 48");
    CHECK(block.records.size() == 2, "multi-rec: 2 records");

    if (block.records.size() < 2) { ++failures; return; }

    const auto& rec1 = block.records[0];
    const auto& rec2 = block.records[1];

    CHECK(rec1.items.count("020"), "rec1: I020 present");
    CHECK(rec2.items.count("020"), "rec2: I020 present");

    if (rec1.items.count("020"))
        CHECK(rec1.items.at("020").fields.at("TYP") == 2, "rec1: TYP=2 (SSR)");
    if (rec2.items.count("020"))
        CHECK(rec2.items.at("020").fields.at("TYP") == 5, "rec2: TYP=5 (ModeS Roll-Call)");

    CHECK(!rec1.items.count("220"), "rec1: no I220");
    CHECK(rec2.items.count("220"),  "rec2: I220 present");

    if (rec2.items.count("220"))
        CHECK(rec2.items.at("220").fields.at("ADR") == 0xDEADBEu, "rec2: ADR == 0xDEADBE");

    if (rec1.items.count("040")) {
        CHECK(rec1.items.at("040").fields.at("RHO")   == 5120, "rec1: RHO == 5120");
        CHECK(rec1.items.at("040").fields.at("THETA") == 8192, "rec1: THETA == 8192");
    }
    if (rec2.items.count("040")) {
        CHECK(rec2.items.at("040").fields.at("RHO")   == 10240, "rec2: RHO == 10240");
        CHECK(rec2.items.at("040").fields.at("THETA") == 24576, "rec2: THETA == 24576");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 11: Decode a real operational CAT48 frame (318 bytes, 9 records)
//
//  All records share FSPEC=FF D6, meaning these items are present in each:
//    I010, I140, I020, I040, I070, I090, I130, I220, I240, I161, I200, I170
//
//  Common across all 9 records:
//    I010: SAC=8, SIC=1
//    I020: TYP=5 (ModeS Roll-Call), RDP=1 (first octet only, FX=0)
//    I130: PSF=0x60 → SRR and SAM present
//    I170: RAD=2 (SSR/ModeS Track, first octet only, FX=0)
//
//  Record 0 spot-check values:
//    I140: TOD=0x657AD7=6650583
//    I040: RHO=0x72BA=29370, THETA=0xD16E=53614
//    I070: MODE3A=0x462
//    I090: FL=0x5C8=1480
//    I130: SRR=2, SAM=0xC0=192
//    I220: ADR=0x484F6D
//    I240: IDENT=0x512075DF0C60
//    I161: TRN=0xDB=219
//    I200: GSP=0x0803=2051, HDG=0x96D4=38612
// ─────────────────────────────────────────────────────────────────────────────
static void testRealFrame(const Codec& codec) {
    std::cout << "\n=== Test: Decode real CAT48 operational frame (318 B, 9 records) ===\n";

    // clang-format off
    std::vector<uint8_t> frame = {
        // Header
        0x30, 0x01, 0x3e,
        // Record 0
        0xff, 0xd6, 0x08, 0x01, 0x65, 0x7a, 0xd7, 0xa8, 0x72, 0xba, 0xd1, 0x6e,
        0x04, 0x62, 0x05, 0xc8, 0x60, 0x02, 0xc0, 0x48, 0x4f, 0x6d, 0x51, 0x20,
        0x75, 0xdf, 0x0c, 0x60, 0x00, 0xdb, 0x08, 0x03, 0x96, 0xd4, 0x40,
        // Record 1
        0xff, 0xd6, 0x08, 0x01, 0x65, 0x7a, 0xed, 0xa8, 0x49, 0x8f, 0xd7, 0x58,
        0x0b, 0x49, 0x05, 0x52, 0x60, 0x02, 0xc2, 0x4d, 0x23, 0x5a, 0x15, 0x71,
        0xf3, 0x55, 0x98, 0x20, 0x02, 0xed, 0x08, 0x80, 0x33, 0x79, 0x40,
        // Record 2
        0xff, 0xd6, 0x08, 0x01, 0x65, 0x7a, 0xe6, 0xa8, 0x69, 0xc6, 0xd5, 0xb9,
        0x02, 0x00, 0x01, 0xc5, 0x60, 0x02, 0xb5, 0xab, 0xaf, 0x47, 0x18, 0x46,
        0x32, 0xc6, 0x08, 0x20, 0x07, 0xb6, 0x05, 0xe4, 0xb0, 0xd0, 0x40,
        // Record 3
        0xff, 0xd6, 0x08, 0x01, 0x65, 0x7a, 0xda, 0xa8, 0x8a, 0x7a, 0xd2, 0x9c,
        0x0a, 0xed, 0x05, 0xf0, 0x60, 0x02, 0xba, 0x4d, 0x21, 0xfe, 0x49, 0x94,
        0xb3, 0x0c, 0x28, 0x20, 0x01, 0xee, 0x07, 0xb4, 0x1e, 0xcd, 0x40,
        // Record 4
        0xff, 0xd6, 0x08, 0x01, 0x65, 0x7a, 0xe0, 0xa8, 0xc4, 0xa1, 0xd3, 0x83,
        0x0c, 0xe7, 0x04, 0x38, 0x60, 0x06, 0xba, 0x40, 0x09, 0xd8, 0x08, 0x15,
        0xf3, 0xdb, 0x26, 0x60, 0x04, 0xd3, 0x08, 0x5d, 0x68, 0x26, 0x40,
        // Record 5
        0xff, 0xd6, 0x08, 0x01, 0x65, 0x7a, 0xd9, 0xa8, 0x66, 0xf7, 0xd2, 0x88,
        0x02, 0x00, 0x01, 0xb8, 0x60, 0x02, 0xba, 0x39, 0xd3, 0x06, 0x51, 0x61,
        0xb9, 0xd4, 0xc5, 0x60, 0x07, 0x98, 0x05, 0xee, 0xb8, 0x73, 0x40,
        // Record 6
        0xff, 0xd6, 0x08, 0x01, 0x65, 0x7a, 0xec, 0xa8, 0xa8, 0xcd, 0xd6, 0xfc,
        0x0b, 0xe0, 0x05, 0xa0, 0x60, 0x02, 0xba, 0x4d, 0x22, 0x8f, 0x49, 0x94,
        0xb6, 0xe5, 0x63, 0xa0, 0x03, 0x76, 0x06, 0x39, 0xe3, 0xc2, 0x40,
        // Record 7
        0xff, 0xd6, 0x08, 0x01, 0x65, 0x7a, 0xd8, 0xa8, 0xb8, 0x49, 0xd2, 0x39,
        0x01, 0x5b, 0x04, 0x9f, 0x60, 0x02, 0xb7, 0x40, 0x0c, 0xeb, 0x08, 0x15,
        0xf1, 0xd3, 0x13, 0x60, 0x00, 0x27, 0x09, 0x42, 0x69, 0xad, 0x40,
        // Record 8
        0xff, 0xd6, 0x08, 0x01, 0x65, 0x7a, 0xd7, 0xa8, 0x73, 0xe9, 0xd1, 0x63,
        0x0d, 0xea, 0x05, 0xf0, 0x60, 0x02, 0xba, 0x48, 0x41, 0xaa, 0x51, 0x20,
        0x78, 0xd9, 0x58, 0x20, 0x03, 0x5a, 0x06, 0xa5, 0xee, 0xa4, 0x40,
    };
    // clang-format on

    hexdump(frame, "real frame");
    DecodedBlock block = codec.decode(frame);

    // ── Block-level checks ────────────────────────────────────────────────────
    CHECK(block.valid,                    "block.valid");
    CHECK(block.cat == 48,                "block.cat == 48");
    CHECK(block.length == 318,            "block.length == 318");
    CHECK(block.records.size() == 9,      "block has 9 records");

    if (!block.valid || block.records.size() != 9) return;

    // ── Per-record invariants (all 9 records) ─────────────────────────────────
    for (size_t i = 0; i < block.records.size(); ++i) {
        const auto& r = block.records[i];
        std::string pfx = "rec[" + std::to_string(i) + "] ";
        CHECK(r.valid,                                               pfx + "valid");
        CHECK(r.uap_variation == "default",                          pfx + "UAP=default");
        CHECK(r.items.count("010"),                                  pfx + "I010 present");
        CHECK(r.items.count("020"),                                  pfx + "I020 present");
        CHECK(r.items.count("170"),                                  pfx + "I170 present");
        if (r.items.count("010")) {
            CHECK(r.items.at("010").fields.at("SAC") == 8,          pfx + "SAC==8");
            CHECK(r.items.at("010").fields.at("SIC") == 1,          pfx + "SIC==1");
        }
        if (r.items.count("020"))
            CHECK(r.items.at("020").fields.at("TYP") == 5,          pfx + "TYP==5 (ModeS Roll-Call)");
        if (r.items.count("170"))
            CHECK(r.items.at("170").fields.at("RAD") == 2,          pfx + "RAD==2 (SSR/ModeS)");
    }

    // ── Record 0 spot-check ───────────────────────────────────────────────────
    const auto& r0 = block.records[0];
    std::cout << "  -- Record 0 detail checks --\n";

    // I020: TYP=5, RDP=1, first octet only (FX=0)
    if (r0.items.count("020")) {
        CHECK(r0.items.at("020").fields.at("TYP") == 5,  "r0 I020.TYP==5");
        CHECK(r0.items.at("020").fields.at("RDP") == 1,  "r0 I020.RDP==1");
    }
    // I140: TOD = 0x657AD7 = 6650583
    if (r0.items.count("140"))
        CHECK(r0.items.at("140").fields.at("TOD") == 0x657AD7u, "r0 I140.TOD==0x657AD7");
    // I040: RHO=0x72BA=29370, THETA=0xD16E=53614
    if (r0.items.count("040")) {
        CHECK(r0.items.at("040").fields.at("RHO")   == 0x72BAu, "r0 I040.RHO==0x72BA");
        CHECK(r0.items.at("040").fields.at("THETA") == 0xD16Eu, "r0 I040.THETA==0xD16E");
    }
    // I070: MODE3A=0x462
    if (r0.items.count("070"))
        CHECK(r0.items.at("070").fields.at("MODE3A") == 0x462u, "r0 I070.MODE3A==0x462");
    // I090: FL=0x5C8=1480
    if (r0.items.count("090"))
        CHECK(r0.items.at("090").fields.at("FL") == 0x5C8u, "r0 I090.FL==0x5C8 (370FL)");
    // I130: PSF=0x60 → SRR=2, SAM=0xC0=192; SRL/PRL/PAM/RPD/APD absent
    if (r0.items.count("130")) {
        const auto& i130 = r0.items.at("130");
        CHECK(i130.compound_sub_fields.count("SRR"),          "r0 I130.SRR present");
        CHECK(i130.compound_sub_fields.count("SAM"),          "r0 I130.SAM present");
        CHECK(!i130.compound_sub_fields.count("SRL"),         "r0 I130.SRL absent");
        CHECK(!i130.compound_sub_fields.count("PRL"),         "r0 I130.PRL absent");
        if (i130.compound_sub_fields.count("SRR"))
            CHECK(i130.compound_sub_fields.at("SRR").at("SRR") == 2,   "r0 I130.SRR==2");
        if (i130.compound_sub_fields.count("SAM"))
            CHECK(i130.compound_sub_fields.at("SAM").at("SAM") == 0xC0u, "r0 I130.SAM==0xC0");
    }
    // I220: ADR=0x484F6D
    if (r0.items.count("220"))
        CHECK(r0.items.at("220").fields.at("ADR") == 0x484F6Du, "r0 I220.ADR==0x484F6D");
    // I240: IDENT=0x512075DF0C60
    if (r0.items.count("240"))
        CHECK(r0.items.at("240").fields.at("IDENT") == 0x512075DF0C60ULL, "r0 I240.IDENT==0x512075DF0C60");
    // I161: TRN=0xDB=219  (4 spare bits + 12-bit TRN)
    if (r0.items.count("161"))
        CHECK(r0.items.at("161").fields.at("TRN") == 0xDBu, "r0 I161.TRN==219");
    // I200: GSP=0x0803=2051, HDG=0x96D4=38612
    if (r0.items.count("200")) {
        CHECK(r0.items.at("200").fields.at("GSP") == 0x0803u, "r0 I200.GSP==0x0803");
        CHECK(r0.items.at("200").fields.at("HDG") == 0x96D4u, "r0 I200.HDG==0x96D4");
    }
    // I170: CNF=0, RAD=2 (first octet only)
    if (r0.items.count("170")) {
        CHECK(r0.items.at("170").fields.at("CNF") == 0, "r0 I170.CNF==0 (confirmed)");
        CHECK(r0.items.at("170").fields.at("RAD") == 2, "r0 I170.RAD==2 (SSR/ModeS)");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = (argc > 1)
        ? fs::path(argv[1])
        : fs::path(__FILE__).parent_path().parent_path() / "specs" / "CAT48.xml";

    std::cout << "Using spec: " << spec_path << '\n';

    Codec codec;
    testSpecLoad(codec, spec_path);

    if (failures == 0) {
        testDecodeBasicTargetReport(codec);
        testRoundTripExtended020(codec);
        testRoundTripExtended170(codec);
        testDecodeWarningCodes(codec);
        testRoundTripBDSRegisterData(codec);
        testRoundTripRadarPlotChar(codec);
        testRoundTripDopplerCAL(codec);
        testRoundTripModeSRecord(codec);
        testMultiRecord(codec);
        testRealFrame(codec);
    }

    std::cout << "\n──────────────────────────────────\n";
    if (failures == 0) {
        std::cout << "ALL TESTS PASSED\n";
    } else {
        std::cout << failures << " TEST(S) FAILED\n";
    }
    return failures == 0 ? 0 : 1;
}
