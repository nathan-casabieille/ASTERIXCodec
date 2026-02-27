// test_cat62.cpp – Tests for CAT62 SDPS Track Messages decode/encode round-trip.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat62

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
//  Test 1: XML spec loads and item types / UAP are correct
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT62 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 62,           "cat number = 62");
    CHECK(cat.edition == "1.21",   "edition = 1.21");

    // All expected items present
    for (auto id : {"010","015","040","060","070","080","100","105","110",
                    "120","130","135","136","185","200","210","220","245",
                    "270","290","295","300","340","380","390","500","510",
                    "RE","SP"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }

    // UAP
    CHECK(cat.uap_variations.count("default"), "UAP variation 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    CHECK(cat.uap_variations.at("default").size() == 35, "UAP has 35 slots");

    // Spot-check UAP ordering
    const auto& uap = cat.uap_variations.at("default");
    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "-",   "UAP slot  2 = - (spare)");
    CHECK(uap[2]  == "015", "UAP slot  3 = 015");
    CHECK(uap[3]  == "070", "UAP slot  4 = 070");
    CHECK(uap[10] == "380", "UAP slot 11 = 380");
    CHECK(uap[25] == "510", "UAP slot 26 = 510");
    CHECK(uap[33] == "RE",  "UAP slot 34 = RE");
    CHECK(uap[34] == "SP",  "UAP slot 35 = SP");

    // Item type assertions
    CHECK(cat.items.at("010").type == ItemType::Fixed,             "010 is Fixed");
    CHECK(cat.items.at("015").type == ItemType::Fixed,             "015 is Fixed");
    CHECK(cat.items.at("040").type == ItemType::Fixed,             "040 is Fixed");
    CHECK(cat.items.at("060").type == ItemType::Fixed,             "060 is Fixed");
    CHECK(cat.items.at("070").type == ItemType::Fixed,             "070 is Fixed");
    CHECK(cat.items.at("080").type == ItemType::Extended,          "080 is Extended");
    CHECK(cat.items.at("100").type == ItemType::Fixed,             "100 is Fixed");
    CHECK(cat.items.at("105").type == ItemType::Fixed,             "105 is Fixed");
    CHECK(cat.items.at("110").type == ItemType::Compound,          "110 is Compound");
    CHECK(cat.items.at("120").type == ItemType::Fixed,             "120 is Fixed");
    CHECK(cat.items.at("130").type == ItemType::Fixed,             "130 is Fixed");
    CHECK(cat.items.at("135").type == ItemType::Fixed,             "135 is Fixed");
    CHECK(cat.items.at("136").type == ItemType::Fixed,             "136 is Fixed");
    CHECK(cat.items.at("185").type == ItemType::Fixed,             "185 is Fixed");
    CHECK(cat.items.at("200").type == ItemType::Fixed,             "200 is Fixed");
    CHECK(cat.items.at("210").type == ItemType::Fixed,             "210 is Fixed");
    CHECK(cat.items.at("220").type == ItemType::Fixed,             "220 is Fixed");
    CHECK(cat.items.at("245").type == ItemType::Fixed,             "245 is Fixed");
    CHECK(cat.items.at("270").type == ItemType::Extended,          "270 is Extended");
    CHECK(cat.items.at("290").type == ItemType::Compound,          "290 is Compound");
    CHECK(cat.items.at("295").type == ItemType::Compound,          "295 is Compound");
    CHECK(cat.items.at("300").type == ItemType::Fixed,             "300 is Fixed");
    CHECK(cat.items.at("340").type == ItemType::Compound,          "340 is Compound");
    CHECK(cat.items.at("380").type == ItemType::Compound,          "380 is Compound");
    CHECK(cat.items.at("390").type == ItemType::Compound,          "390 is Compound");
    CHECK(cat.items.at("500").type == ItemType::Compound,          "500 is Compound");
    CHECK(cat.items.at("510").type == ItemType::RepetitiveGroupFX, "510 is RepetitiveGroupFX");
    CHECK(cat.items.at("RE").type  == ItemType::SP,                "RE is SP/Explicit");
    CHECK(cat.items.at("SP").type  == ItemType::SP,                "SP is SP/Explicit");

    // Fixed byte-sizes
    CHECK(cat.items.at("010").fixed_bytes == 2, "010 = 2 bytes");
    CHECK(cat.items.at("015").fixed_bytes == 1, "015 = 1 byte");
    CHECK(cat.items.at("040").fixed_bytes == 2, "040 = 2 bytes");
    CHECK(cat.items.at("060").fixed_bytes == 2, "060 = 2 bytes");
    CHECK(cat.items.at("070").fixed_bytes == 3, "070 = 3 bytes");
    CHECK(cat.items.at("100").fixed_bytes == 6, "100 = 6 bytes");
    CHECK(cat.items.at("105").fixed_bytes == 8, "105 = 8 bytes");
    CHECK(cat.items.at("120").fixed_bytes == 2, "120 = 2 bytes");
    CHECK(cat.items.at("130").fixed_bytes == 2, "130 = 2 bytes");
    CHECK(cat.items.at("135").fixed_bytes == 2, "135 = 2 bytes");
    CHECK(cat.items.at("136").fixed_bytes == 2, "136 = 2 bytes");
    CHECK(cat.items.at("185").fixed_bytes == 4, "185 = 4 bytes");
    CHECK(cat.items.at("200").fixed_bytes == 1, "200 = 1 byte");
    CHECK(cat.items.at("210").fixed_bytes == 2, "210 = 2 bytes");
    CHECK(cat.items.at("220").fixed_bytes == 2, "220 = 2 bytes");
    CHECK(cat.items.at("245").fixed_bytes == 7, "245 = 7 bytes");

    // Extended octet counts
    CHECK(cat.items.at("080").octets.size() == 7, "080 has 7 octets");
    CHECK(cat.items.at("270").octets.size() == 3, "270 has 3 octets");

    // Compound sub-item counts
    CHECK(cat.items.at("110").compound_sub_items.size() == 7,  "110 has 7 sub-items");
    CHECK(cat.items.at("290").compound_sub_items.size() == 10, "290 has 10 sub-items");
    CHECK(cat.items.at("340").compound_sub_items.size() == 6,  "340 has 6 sub-items");
    CHECK(cat.items.at("500").compound_sub_items.size() == 8,  "500 has 8 sub-items");

    // I510 RepetitiveGroupFX details
    const auto& i510 = cat.items.at("510");
    CHECK(i510.rep_group_bits == 23, "510 rep_group_bits = 23 (IDENT:8 + TRACK:15)");
    CHECK(i510.rep_group_elements.size() == 2, "510 has 2 group elements");
    CHECK(i510.rep_group_elements[0].name == "IDENT", "510 element[0] = IDENT");
    CHECK(i510.rep_group_elements[1].name == "TRACK", "510 element[1] = TRACK");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode basic track report (I010 + I070 + I105 + I040 + I080)
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasic(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT62 record ===\n";

    // Build a minimal record manually:
    //   I010: SAC=0x01, SIC=0x02
    //   I070: ToT = 0x0100F0  (little bit of time)
    //   I105: LAT=0, LON=0
    //   I040: TN=0x1234
    //   I080: Track Status, octet 1 only (MON=0,SPI=0,MRH=0,SRC=0,CNF=0), FX=0
    //         → byte = 0x00 (all zero, FX=0)
    //
    // FSPEC:
    //   Slot 1=010 → bit7 of byte1 → present
    //   Slot 3=015 → not present
    //   Slot 4=070 → bit4 of byte1 → present
    //   Slot 5=105 → bit3 of byte1 → present
    //   Slot 6=100 → not present
    //   Slot 7=185 → not present
    //   FSPEC byte 1: bit7(010)=1, bit6(-)=0, bit5(015)=0, bit4(070)=1,
    //                 bit3(105)=1, bit2(100)=0, bit1(185)=0, bit0(FX)=1
    //               = 1001 1001 = 0x99... wait, FX=1 means more FSPEC bytes
    //   Slot 12=040 → bit3 of byte2 → present
    //   Slot 13=080 → bit2 of byte2 → present
    //   FSPEC byte 2: bit7(210)=0, bit6(060)=0, bit5(245)=0, bit4(380)=0,
    //                 bit3(040)=1, bit2(080)=1, bit1(290)=0, bit0(FX)=0
    //               = 0000 1100 = 0x0C

    std::vector<uint8_t> frame = {
        0x3E,       // CAT=62
        0x00, 0x00, // LEN (placeholder, fix below)

        // FSPEC byte 1: 010(7), -(6), 015(5), 070(4), 105(3), 100(2), 185(1), FX(0)
        //   010=present → bit7=1
        //   070=present → bit4=1
        //   105=present → bit3=1
        //   FX=1 (more FSPEC)
        0x99,
        // FSPEC byte 2: 210(7), 060(6), 245(5), 380(4), 040(3), 080(2), 290(1), FX(0)
        //   040=present → bit3=1
        //   080=present → bit2=1
        //   FX=0 (last FSPEC)
        0x0C,

        // I010: SAC=0x01, SIC=0x02
        0x01, 0x02,

        // I070: ToT = 128 seconds → raw = 128 * 128 = 16384 = 0x004000
        0x00, 0x40, 0x00,

        // I105: LAT=0, LON=0 (8 bytes of zeros)
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,

        // I040: TN = 0x1234
        0x12, 0x34,

        // I080: octet 1 only, all zero, FX=0 → 0x00
        0x00,
    };

    // Fix the length field
    uint16_t len = static_cast<uint16_t>(frame.size());
    frame[1] = static_cast<uint8_t>(len >> 8);
    frame[2] = static_cast<uint8_t>(len & 0xFF);

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid,              "block valid");
    CHECK(blk.cat == 62,          "cat = 62");
    CHECK(blk.records.size() == 1,"one record");

    if (!blk.records.empty()) {
        const auto& rec = blk.records[0];
        CHECK(rec.valid,                    "record valid");
        CHECK(rec.items.count("010"),       "I010 present");
        CHECK(rec.items.count("070"),       "I070 present");
        CHECK(rec.items.count("105"),       "I105 present");
        CHECK(rec.items.count("040"),       "I040 present");
        CHECK(rec.items.count("080"),       "I080 present");
        CHECK(!rec.items.count("015"),      "I015 absent");

        CHECK(rec.items.at("010").fields.at("SAC") == 0x01, "I010 SAC = 1");
        CHECK(rec.items.at("010").fields.at("SIC") == 0x02, "I010 SIC = 2");
        CHECK(rec.items.at("070").fields.at("TOT") == 0x004000, "I070 ToT raw = 16384");
        CHECK(rec.items.at("040").fields.at("TN")  == 0x1234,   "I040 TN = 0x1234");

        // I080 octet 1, all zero
        const auto& i080 = rec.items.at("080");
        CHECK(i080.fields.at("MON") == 0, "I080 MON = 0");
        CHECK(i080.fields.at("SRC") == 0, "I080 SRC = 0");
        CHECK(i080.fields.at("CNF") == 0, "I080 CNF = 0");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip Fixed items (I060, I130, I135, I136, I185, I200, I210, I220)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip fixed items ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    // I010
    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 0xAB; di.fields["SIC"] = 0xCD; rec.items["010"] = di; }

    // I060: V=1, G=0, CH=1, spare, MODE3A=0777
    { DecodedItem di; di.item_id = "060"; di.type = ItemType::Fixed;
      di.fields["V"] = 1; di.fields["G"] = 0; di.fields["CH"] = 1;
      // MODE3A = 0777 octal → Octal bits: 7=111, 7=111, 7=111 → binary 111111111111 = 0xFFF
      di.fields["MODE3A"] = 0x1FF; // 0377 octal = 011 111 111 = 0xFF
      rec.items["060"] = di; }

    // I130: geometric altitude = 10000 ft → raw = 10000/6.25 = 1600 = 0x0640
    { DecodedItem di; di.item_id = "130"; di.type = ItemType::Fixed;
      di.fields["ALT"] = 1600; rec.items["130"] = di; }

    // I135: QNH=0, CTB = FL350 → raw = 350/0.25 = 1400 = 0x0578
    { DecodedItem di; di.item_id = "135"; di.type = ItemType::Fixed;
      di.fields["QNH"] = 0; di.fields["CTB"] = 1400; rec.items["135"] = di; }

    // I136: MFL = FL250 → raw = 250/0.25 = 1000 = 0x03E8
    { DecodedItem di; di.item_id = "136"; di.type = ItemType::Fixed;
      di.fields["MFL"] = 1000; rec.items["136"] = di; }

    // I185: VX = 100 m/s → raw = 100/0.25 = 400; VY = -50 m/s → raw = -200
    { DecodedItem di; di.item_id = "185"; di.type = ItemType::Fixed;
      di.fields["VX"] = 400; di.fields["VY"] = static_cast<uint64_t>(-200); rec.items["185"] = di; }

    // I200: TRANS=1 (right turn), LONG=1 (increasing), VERT=1 (climb), ADF=0
    { DecodedItem di; di.item_id = "200"; di.type = ItemType::Fixed;
      di.fields["TRANS"] = 1; di.fields["LONG"] = 1; di.fields["VERT"] = 1; di.fields["ADF"] = 0;
      rec.items["200"] = di; }

    // I210: AX = 1 m/s² → raw=4; AY = -2 m/s² → raw = -8
    { DecodedItem di; di.item_id = "210"; di.type = ItemType::Fixed;
      di.fields["AX"] = 4; di.fields["AY"] = static_cast<uint64_t>(-8); rec.items["210"] = di; }

    // I220: ROCD = 2000 ft/min → raw = 2000/6.25 = 320
    { DecodedItem di; di.item_id = "220"; di.type = ItemType::Fixed;
      di.fields["ROCD"] = 320; rec.items["220"] = di; }

    auto encoded = codec.encode(62, {rec});
    hexdump(encoded, "Fixed round-trip encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid,                   "RT-Fixed block valid");
    CHECK(blk.records.size() == 1,     "RT-Fixed one record");

    if (!blk.records.empty()) {
        const auto& r = blk.records[0];
        CHECK(r.items.at("010").fields.at("SAC")   == 0xAB,  "RT SAC");
        CHECK(r.items.at("010").fields.at("SIC")   == 0xCD,  "RT SIC");
        CHECK(r.items.at("060").fields.at("V")     == 1,     "RT I060 V");
        CHECK(r.items.at("060").fields.at("CH")    == 1,     "RT I060 CH");
        CHECK(r.items.at("130").fields.at("ALT")   == 1600,  "RT I130 ALT");
        CHECK(r.items.at("135").fields.at("QNH")   == 0,     "RT I135 QNH");
        CHECK(r.items.at("135").fields.at("CTB")   == 1400,  "RT I135 CTB");
        CHECK(r.items.at("136").fields.at("MFL")   == 1000,  "RT I136 MFL");
        CHECK(r.items.at("185").fields.at("VX")    == 400,   "RT I185 VX");
        // VY is stored as unsigned (two's complement on 16 bits): -200 & 0xFFFF = 0xFF38
        CHECK((r.items.at("185").fields.at("VY") & 0xFFFF) ==
              (static_cast<uint64_t>(-200) & 0xFFFF),        "RT I185 VY");
        CHECK(r.items.at("200").fields.at("TRANS") == 1,     "RT I200 TRANS");
        CHECK(r.items.at("200").fields.at("VERT")  == 1,     "RT I200 VERT");
        CHECK(r.items.at("220").fields.at("ROCD")  == 320,   "RT I220 ROCD");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip I080 (Extended, multiple octets)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripI080(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I080 (Extended, 3 octets) ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 0; di.fields["SIC"] = 0; rec.items["010"] = di; }

    // I080 with octets 1, 2, and 3 populated
    { DecodedItem di; di.item_id = "080"; di.type = ItemType::Extended;
      di.fields["MON"] = 1;   // Monosensor
      di.fields["SPI"] = 0;
      di.fields["MRH"] = 1;   // Geometric altitude more reliable
      di.fields["SRC"] = 1;   // GNSS
      di.fields["CNF"] = 0;   // Confirmed track
      // Octet 2
      di.fields["SIM"] = 0;
      di.fields["TSE"] = 0;
      di.fields["TSB"] = 1;   // First message for track
      di.fields["FPC"] = 1;   // Flight plan correlated
      di.fields["AFF"] = 0;
      di.fields["STP"] = 0;
      di.fields["KOS"] = 0;
      // Octet 3: set AMA=1 to ensure this octet is emitted
      di.fields["AMA"] = 1;
      di.fields["MD4"] = 0;
      di.fields["ME"]  = 0;
      di.fields["MI"]  = 0;
      di.fields["MD5"] = 0;
      rec.items["080"] = di; }

    auto encoded = codec.encode(62, {rec});
    hexdump(encoded, "I080 Extended encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid,                     "I080 RT block valid");
    CHECK(blk.records.size() == 1,       "I080 RT one record");

    if (!blk.records.empty()) {
        const auto& r = blk.records[0];
        CHECK(r.items.count("080"),           "I080 present after RT");
        const auto& i = r.items.at("080");
        CHECK(i.fields.at("MON") == 1,        "I080 MON = 1");
        CHECK(i.fields.at("MRH") == 1,        "I080 MRH = 1");
        CHECK(i.fields.at("SRC") == 1,        "I080 SRC = 1 (GNSS)");
        CHECK(i.fields.at("TSB") == 1,        "I080 TSB = 1");
        CHECK(i.fields.at("FPC") == 1,        "I080 FPC = 1");
        CHECK(i.fields.at("AMA") == 1,        "I080 AMA = 1 (amalgamation)");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip I510 (RepetitiveGroupFX, composed track number)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripI510(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I510 (RepetitiveGroupFX) ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 1; di.fields["SIC"] = 2; rec.items["010"] = di; }

    // I510 with 3 composed track number entries
    { DecodedItem di; di.item_id = "510"; di.type = ItemType::RepetitiveGroupFX;
      di.group_repetitions.push_back({{"IDENT", 0x01}, {"TRACK", 0x1234}});
      di.group_repetitions.push_back({{"IDENT", 0x02}, {"TRACK", 0x5678}});
      di.group_repetitions.push_back({{"IDENT", 0x03}, {"TRACK", 0x7FFF}});
      rec.items["510"] = di; }

    auto encoded = codec.encode(62, {rec});
    hexdump(encoded, "I510 RepGrpFX encoded");

    // I510 uses FSPEC slot 26 = byte4 bit2:
    // FSPEC byte 4: 270(7),300(6),110(5),120(4),510(3),500(2),340(1),FX(0)
    // With only I010 (slot1, FSPEC byte1) and I510 (slot26, FSPEC byte4):
    //   FSPEC byte 1: bit7=1 (010), rest=0, FX=1 → 0x81
    //   FSPEC byte 2: all 0, FX=1 → 0x01
    //   FSPEC byte 3: all 0, FX=1 → 0x01
    //   FSPEC byte 4: bit3=1 (510), FX=0 → 0x08
    // 3 entries, each 3 bytes:
    //   Entry 1: IDENT=0x01, TRACK=0x1234 (15 bits), FX=1
    //     = 0x01, (0x1234 >> 7) = ... let me think:
    //     IDENT=8 bits = 0x01
    //     TRACK=15 bits = 0x1234 = 0001 0010 0011 0100
    //     FX=1 (bit)
    //     = 0x01 | 0x1234<<1 | FX | ... = byte stream:
    //     bit layout: IDENT[7:0] TRACK[14:0] FX[0]
    //     byte0 = IDENT = 0x01
    //     byte1 = TRACK[14:7] = upper 8 bits of 0x2468 (0x1234<<1) = 0x24
    //     byte2 = TRACK[6:0]<<1 | FX = (0x1234&0x7F)<<1 | 1 = 0x68|1 = 0x69
    //   etc.

    // Round-trip check
    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid,               "I510 RT block valid");
    CHECK(blk.records.size() == 1, "I510 RT one record");

    if (!blk.records.empty()) {
        const auto& r = blk.records[0];
        CHECK(r.items.count("510"),           "I510 present after RT");
        const auto& i = r.items.at("510");
        CHECK(i.group_repetitions.size() == 3, "I510 has 3 entries");
        if (i.group_repetitions.size() == 3) {
            CHECK(i.group_repetitions[0].at("IDENT") == 0x01,   "I510[0] IDENT=1");
            CHECK(i.group_repetitions[0].at("TRACK") == 0x1234, "I510[0] TRACK=0x1234");
            CHECK(i.group_repetitions[1].at("IDENT") == 0x02,   "I510[1] IDENT=2");
            CHECK(i.group_repetitions[1].at("TRACK") == 0x5678, "I510[1] TRACK=0x5678");
            CHECK(i.group_repetitions[2].at("IDENT") == 0x03,   "I510[2] IDENT=3");
            CHECK(i.group_repetitions[2].at("TRACK") == 0x7FFF, "I510[2] TRACK=0x7FFF");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip I110 (Compound – Mode 5 summary sub-items)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripI110(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I110 (Compound, Mode 5) ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 0; di.fields["SIC"] = 0; rec.items["010"] = di; }

    // I110 with SUM and GA sub-items
    { DecodedItem di; di.item_id = "110"; di.type = ItemType::Compound;
      // SUM: M5=1, ID=1, DA=0, M1=0, M2=0, M3=0, MC=1, X=0
      di.compound_sub_fields["SUM"] = {
          {"M5", 1}, {"ID", 1}, {"DA", 0}, {"M1", 0},
          {"M2", 0}, {"M3", 0}, {"MC", 1}, {"X", 0}
      };
      // GA: spare=0, RES=1 (25 ft), GA = -200 ft → raw = -200/25 = -8
      di.compound_sub_fields["GA"] = {
          {"RES", 1}, {"GA", static_cast<uint64_t>(-8)}
      };
      rec.items["110"] = di; }

    auto encoded = codec.encode(62, {rec});
    hexdump(encoded, "I110 Compound encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid,                   "I110 RT block valid");
    CHECK(blk.records.size() == 1,     "I110 RT one record");

    if (!blk.records.empty()) {
        const auto& r = blk.records[0];
        CHECK(r.items.count("110"),          "I110 present after RT");
        const auto& i = r.items.at("110");
        CHECK(i.compound_sub_fields.count("SUM"), "I110/SUM present");
        CHECK(i.compound_sub_fields.count("GA"),  "I110/GA present");
        CHECK(!i.compound_sub_fields.count("PMN"), "I110/PMN absent");

        CHECK(i.compound_sub_fields.at("SUM").at("M5") == 1,  "I110/SUM M5=1");
        CHECK(i.compound_sub_fields.at("SUM").at("ID") == 1,  "I110/SUM ID=1");
        CHECK(i.compound_sub_fields.at("SUM").at("MC") == 1,  "I110/SUM MC=1");
        CHECK(i.compound_sub_fields.at("GA").at("RES") == 1,  "I110/GA RES=1");
        // GA raw value on 14 bits for -8: 0x3FF8
        CHECK((i.compound_sub_fields.at("GA").at("GA") & 0x3FFF) ==
              (static_cast<uint64_t>(-8) & 0x3FFF),           "I110/GA value");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip I290 (Compound – System Track Update Ages)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripI290(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I290 (Compound, Track Update Ages) ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 0; di.fields["SIC"] = 0; rec.items["010"] = di; }

    // I290 with TRK, PSR, and MLT ages
    { DecodedItem di; di.item_id = "290"; di.type = ItemType::Compound;
      // TRK = 2.5 s → raw = 2.5/0.25 = 10
      di.compound_sub_fields["TRK"] = {{"TRK", 10}};
      // PSR = 5.0 s → raw = 20
      di.compound_sub_fields["PSR"] = {{"PSR", 20}};
      // MLT = 1.25 s → raw = 5
      di.compound_sub_fields["MLT"] = {{"MLT", 5}};
      rec.items["290"] = di; }

    auto encoded = codec.encode(62, {rec});
    hexdump(encoded, "I290 Compound encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid,               "I290 RT block valid");
    CHECK(blk.records.size() == 1, "I290 RT one record");

    if (!blk.records.empty()) {
        const auto& r = blk.records[0];
        CHECK(r.items.count("290"),               "I290 present after RT");
        const auto& i = r.items.at("290");
        CHECK(i.compound_sub_fields.count("TRK"), "I290/TRK present");
        CHECK(i.compound_sub_fields.count("PSR"), "I290/PSR present");
        CHECK(!i.compound_sub_fields.count("SSR"),"I290/SSR absent");
        CHECK(i.compound_sub_fields.count("MLT"), "I290/MLT present");

        CHECK(i.compound_sub_fields.at("TRK").at("TRK") == 10, "I290/TRK = 10");
        CHECK(i.compound_sub_fields.at("PSR").at("PSR") == 20, "I290/PSR = 20");
        CHECK(i.compound_sub_fields.at("MLT").at("MLT") == 5,  "I290/MLT = 5");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Round-trip I270 (Extended – Target Size and Orientation)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripI270(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I270 (Extended, target size) ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 0; di.fields["SIC"] = 0; rec.items["010"] = di; }

    // I270: LENGTH=50m, ORIENTATION=45 deg (raw = 45/2.8125 ≈ 16), WIDTH=20m
    { DecodedItem di; di.item_id = "270"; di.type = ItemType::Extended;
      di.fields["LENGTH"]      = 50;  // 50 m
      di.fields["ORIENTATION"] = 16;  // ~45 deg
      di.fields["WIDTH"]       = 20;  // 20 m
      rec.items["270"] = di; }

    auto encoded = codec.encode(62, {rec});
    hexdump(encoded, "I270 Extended encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid,               "I270 RT block valid");
    CHECK(blk.records.size() == 1, "I270 RT one record");

    if (!blk.records.empty()) {
        const auto& r = blk.records[0];
        CHECK(r.items.count("270"),                  "I270 present after RT");
        const auto& i = r.items.at("270");
        CHECK(i.fields.at("LENGTH")      == 50,      "I270 LENGTH = 50");
        CHECK(i.fields.at("ORIENTATION") == 16,      "I270 ORIENTATION = 16");
        CHECK(i.fields.at("WIDTH")       == 20,      "I270 WIDTH = 20");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 9: Round-trip I340 (Compound – Measured Information)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripI340(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I340 (Compound, measured info) ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 0; di.fields["SIC"] = 0; rec.items["010"] = di; }

    // I340 with SID, POS, MDA sub-items
    { DecodedItem di; di.item_id = "340"; di.type = ItemType::Compound;
      di.compound_sub_fields["SID"] = {{"SAC", 0x01}, {"SIC", 0x05}};
      // POS: RHO = 50 NM → raw = 50/0.00390625 = 12800; THETA = 90 deg → raw = 90/0.00549316... = 16384
      di.compound_sub_fields["POS"] = {{"RHO", 12800}, {"THETA", 16384}};
      // MDA: V=0, G=0, L=0, spare, MODE3A=01234 octal = 668 decimal
      { std::map<std::string,uint64_t> m; m["V"]=0; m["G"]=0; m["L"]=0; m["MODE3A"]=01234;
        di.compound_sub_fields["MDA"] = std::move(m); }
      rec.items["340"] = di; }

    auto encoded = codec.encode(62, {rec});
    hexdump(encoded, "I340 Compound encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid,               "I340 RT block valid");
    CHECK(blk.records.size() == 1, "I340 RT one record");

    if (!blk.records.empty()) {
        const auto& r = blk.records[0];
        CHECK(r.items.count("340"),                "I340 present after RT");
        const auto& i = r.items.at("340");
        CHECK(i.compound_sub_fields.count("SID"),  "I340/SID present");
        CHECK(i.compound_sub_fields.count("POS"),  "I340/POS present");
        CHECK(!i.compound_sub_fields.count("HEIGHT"), "I340/HEIGHT absent");
        CHECK(i.compound_sub_fields.count("MDA"),  "I340/MDA present");

        CHECK(i.compound_sub_fields.at("SID").at("SAC") == 1,     "I340/SID SAC=1");
        CHECK(i.compound_sub_fields.at("SID").at("SIC") == 5,     "I340/SID SIC=5");
        CHECK(i.compound_sub_fields.at("POS").at("RHO") == 12800, "I340/POS RHO");
        CHECK(i.compound_sub_fields.at("MDA").at("MODE3A") == 01234, "I340/MDA MODE3A=01234o");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 10: Multi-record block with two different records
// ─────────────────────────────────────────────────────────────────────────────
static void testMultiRecord(Codec& codec) {
    std::cout << "\n=== Test: Multi-record block ===\n";

    // Record 1: I010 + I070 + I040
    DecodedRecord rec1;
    rec1.uap_variation = "default";
    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 1; di.fields["SIC"] = 1; rec1.items["010"] = di; }
    { DecodedItem di; di.item_id = "070"; di.type = ItemType::Fixed;
      di.fields["TOT"] = 0x000100; rec1.items["070"] = di; }
    { DecodedItem di; di.item_id = "040"; di.type = ItemType::Fixed;
      di.fields["TN"] = 100; rec1.items["040"] = di; }

    // Record 2: I010 + I040 + I200
    DecodedRecord rec2;
    rec2.uap_variation = "default";
    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 1; di.fields["SIC"] = 2; rec2.items["010"] = di; }
    { DecodedItem di; di.item_id = "040"; di.type = ItemType::Fixed;
      di.fields["TN"] = 200; rec2.items["040"] = di; }
    { DecodedItem di; di.item_id = "200"; di.type = ItemType::Fixed;
      di.fields["TRANS"] = 0; di.fields["LONG"] = 1;
      di.fields["VERT"] = 2; di.fields["ADF"] = 0; rec2.items["200"] = di; }

    auto encoded = codec.encode(62, {rec1, rec2});
    hexdump(encoded, "Multi-record encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid,               "multi-record block valid");
    CHECK(blk.records.size() == 2, "two records decoded");

    if (blk.records.size() == 2) {
        CHECK(blk.records[0].items.count("010"), "rec1 has I010");
        CHECK(blk.records[0].items.count("070"), "rec1 has I070");
        CHECK(blk.records[0].items.count("040"), "rec1 has I040");
        CHECK(!blk.records[0].items.count("200"),"rec1 has no I200");

        CHECK(blk.records[1].items.count("010"), "rec2 has I010");
        CHECK(!blk.records[1].items.count("070"),"rec2 has no I070");
        CHECK(blk.records[1].items.count("040"), "rec2 has I040");
        CHECK(blk.records[1].items.count("200"), "rec2 has I200");

        CHECK(blk.records[0].items.at("040").fields.at("TN") == 100, "rec1 TN=100");
        CHECK(blk.records[1].items.at("040").fields.at("TN") == 200, "rec2 TN=200");
        CHECK(blk.records[1].items.at("200").fields.at("VERT") == 2, "rec2 VERT=2 (descent)");
        CHECK(blk.records[1].items.at("010").fields.at("SIC") == 2,  "rec2 SIC=2");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = (argc > 1)
        ? fs::path(argv[1])
        : fs::path(__FILE__).parent_path().parent_path() / "specs" / "CAT62.xml";

    std::cout << "Using spec: " << spec_path << '\n';

    Codec codec;

    testSpecLoad(codec, spec_path);
    testDecodeBasic(codec);
    testRoundTripFixed(codec);
    testRoundTripI080(codec);
    testRoundTripI510(codec);
    testRoundTripI110(codec);
    testRoundTripI290(codec);
    testRoundTripI270(codec);
    testRoundTripI340(codec);
    testMultiRecord(codec);

    std::cout << "\n=== Summary: " << failures << " failure(s) ===\n";
    return failures == 0 ? 0 : 1;
}
