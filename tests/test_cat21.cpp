// test_cat21.cpp – Tests for CAT21 (ADS-B Target Reports, Ed. 2.7) decode/encode.

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

        if (dst.group_repetitions.size() != src.group_repetitions.size()) {
            std::cerr << "FAIL [RT] " << p << " group_repetitions count mismatch\n";
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

        for (const auto& [sname, sfields] : src.compound_sub_fields) {
            auto sit = dst.compound_sub_fields.find(sname);
            if (sit == dst.compound_sub_fields.end()) {
                std::cerr << "FAIL [RT] " << p << "/" << sname << " missing\n";
                ++failures;
                continue;
            }
            for (const auto& [fname, fval] : sfields) {
                auto sfit = sit->second.find(fname);
                if (sfit == sit->second.end()) {
                    std::cerr << "FAIL [RT] " << p << "/" << sname << "."
                              << fname << " missing\n";
                    ++failures;
                } else {
                    CHECK(sfit->second == fval, p + "/" + sname + "." + fname);
                }
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 1: XML spec loads without error; item types and UAP are correct
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT21 spec load ===\n";
    try {
        CategoryDef cat = loadSpec(spec_path);
        CHECK(cat.cat == 21,           "cat number = 21");

        // Presence of all items
        CHECK(cat.items.count("008"),  "item 008 present");
        CHECK(cat.items.count("010"),  "item 010 present");
        CHECK(cat.items.count("015"),  "item 015 present");
        CHECK(cat.items.count("016"),  "item 016 present");
        CHECK(cat.items.count("020"),  "item 020 present");
        CHECK(cat.items.count("040"),  "item 040 present");
        CHECK(cat.items.count("070"),  "item 070 present");
        CHECK(cat.items.count("071"),  "item 071 present");
        CHECK(cat.items.count("072"),  "item 072 present");
        CHECK(cat.items.count("073"),  "item 073 present");
        CHECK(cat.items.count("074"),  "item 074 present");
        CHECK(cat.items.count("075"),  "item 075 present");
        CHECK(cat.items.count("076"),  "item 076 present");
        CHECK(cat.items.count("077"),  "item 077 present");
        CHECK(cat.items.count("080"),  "item 080 present");
        CHECK(cat.items.count("090"),  "item 090 present");
        CHECK(cat.items.count("110"),  "item 110 present");
        CHECK(cat.items.count("130"),  "item 130 present");
        CHECK(cat.items.count("131"),  "item 131 present");
        CHECK(cat.items.count("132"),  "item 132 present");
        CHECK(cat.items.count("140"),  "item 140 present");
        CHECK(cat.items.count("145"),  "item 145 present");
        CHECK(cat.items.count("146"),  "item 146 present");
        CHECK(cat.items.count("148"),  "item 148 present");
        CHECK(cat.items.count("150"),  "item 150 present");
        CHECK(cat.items.count("151"),  "item 151 present");
        CHECK(cat.items.count("152"),  "item 152 present");
        CHECK(cat.items.count("155"),  "item 155 present");
        CHECK(cat.items.count("157"),  "item 157 present");
        CHECK(cat.items.count("160"),  "item 160 present");
        CHECK(cat.items.count("161"),  "item 161 present");
        CHECK(cat.items.count("165"),  "item 165 present");
        CHECK(cat.items.count("170"),  "item 170 present");
        CHECK(cat.items.count("200"),  "item 200 present");
        CHECK(cat.items.count("210"),  "item 210 present");
        CHECK(cat.items.count("220"),  "item 220 present");
        CHECK(cat.items.count("230"),  "item 230 present");
        CHECK(cat.items.count("250"),  "item 250 present");
        CHECK(cat.items.count("260"),  "item 260 present");
        CHECK(cat.items.count("271"),  "item 271 present");
        CHECK(cat.items.count("295"),  "item 295 present");
        CHECK(cat.items.count("400"),  "item 400 present");
        CHECK(cat.items.count("RE"),   "item RE present");
        CHECK(cat.items.count("SP"),   "item SP present");

        // UAP
        CHECK(cat.uap_variations.count("default"), "UAP variation 'default' exists");
        CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
        CHECK(cat.uap_variations.at("default").size() == 49, "UAP has 49 slots");

        // Item type assertions
        CHECK(cat.items.at("008").type == ItemType::Fixed,           "008 is Fixed");
        CHECK(cat.items.at("010").type == ItemType::Fixed,           "010 is Fixed");
        CHECK(cat.items.at("015").type == ItemType::Fixed,           "015 is Fixed");
        CHECK(cat.items.at("016").type == ItemType::Fixed,           "016 is Fixed");
        CHECK(cat.items.at("020").type == ItemType::Fixed,           "020 is Fixed");
        CHECK(cat.items.at("040").type == ItemType::Extended,        "040 is Extended");
        CHECK(cat.items.at("070").type == ItemType::Fixed,           "070 is Fixed");
        CHECK(cat.items.at("071").type == ItemType::Fixed,           "071 is Fixed");
        CHECK(cat.items.at("080").type == ItemType::Fixed,           "080 is Fixed");
        CHECK(cat.items.at("090").type == ItemType::Extended,        "090 is Extended");
        CHECK(cat.items.at("110").type == ItemType::Compound,        "110 is Compound");
        CHECK(cat.items.at("130").type == ItemType::Fixed,           "130 is Fixed");
        CHECK(cat.items.at("131").type == ItemType::Fixed,           "131 is Fixed");
        CHECK(cat.items.at("132").type == ItemType::Fixed,           "132 is Fixed");
        CHECK(cat.items.at("140").type == ItemType::Fixed,           "140 is Fixed");
        CHECK(cat.items.at("145").type == ItemType::Fixed,           "145 is Fixed");
        CHECK(cat.items.at("146").type == ItemType::Fixed,           "146 is Fixed");
        CHECK(cat.items.at("148").type == ItemType::Fixed,           "148 is Fixed");
        CHECK(cat.items.at("150").type == ItemType::Fixed,           "150 is Fixed");
        CHECK(cat.items.at("151").type == ItemType::Fixed,           "151 is Fixed");
        CHECK(cat.items.at("152").type == ItemType::Fixed,           "152 is Fixed");
        CHECK(cat.items.at("155").type == ItemType::Fixed,           "155 is Fixed");
        CHECK(cat.items.at("157").type == ItemType::Fixed,           "157 is Fixed");
        CHECK(cat.items.at("160").type == ItemType::Fixed,           "160 is Fixed");
        CHECK(cat.items.at("161").type == ItemType::Fixed,           "161 is Fixed");
        CHECK(cat.items.at("165").type == ItemType::Fixed,           "165 is Fixed");
        CHECK(cat.items.at("170").type == ItemType::Fixed,           "170 is Fixed");
        CHECK(cat.items.at("200").type == ItemType::Fixed,           "200 is Fixed");
        CHECK(cat.items.at("210").type == ItemType::Fixed,           "210 is Fixed");
        CHECK(cat.items.at("220").type == ItemType::Compound,        "220 is Compound");
        CHECK(cat.items.at("230").type == ItemType::Fixed,           "230 is Fixed");
        CHECK(cat.items.at("250").type == ItemType::RepetitiveGroup, "250 is RepetitiveGroup");
        CHECK(cat.items.at("260").type == ItemType::Fixed,           "260 is Fixed");
        CHECK(cat.items.at("271").type == ItemType::Extended,        "271 is Extended");
        CHECK(cat.items.at("295").type == ItemType::Compound,        "295 is Compound");
        CHECK(cat.items.at("400").type == ItemType::Fixed,           "400 is Fixed");
        CHECK(cat.items.at("RE").type  == ItemType::SP,              "RE is Explicit");
        CHECK(cat.items.at("SP").type  == ItemType::SP,              "SP is Explicit");

        // Fixed item byte sizes
        CHECK(cat.items.at("008").fixed_bytes == 1,  "008 = 1 byte");
        CHECK(cat.items.at("010").fixed_bytes == 2,  "010 = 2 bytes");
        CHECK(cat.items.at("015").fixed_bytes == 1,  "015 = 1 byte");
        CHECK(cat.items.at("016").fixed_bytes == 1,  "016 = 1 byte");
        CHECK(cat.items.at("020").fixed_bytes == 1,  "020 = 1 byte");
        CHECK(cat.items.at("070").fixed_bytes == 2,  "070 = 2 bytes");
        CHECK(cat.items.at("071").fixed_bytes == 3,  "071 = 3 bytes");
        CHECK(cat.items.at("072").fixed_bytes == 3,  "072 = 3 bytes");
        CHECK(cat.items.at("073").fixed_bytes == 3,  "073 = 3 bytes");
        CHECK(cat.items.at("074").fixed_bytes == 4,  "074 = 4 bytes");
        CHECK(cat.items.at("075").fixed_bytes == 3,  "075 = 3 bytes");
        CHECK(cat.items.at("076").fixed_bytes == 4,  "076 = 4 bytes");
        CHECK(cat.items.at("077").fixed_bytes == 3,  "077 = 3 bytes");
        CHECK(cat.items.at("080").fixed_bytes == 3,  "080 = 3 bytes");
        CHECK(cat.items.at("130").fixed_bytes == 6,  "130 = 6 bytes");
        CHECK(cat.items.at("131").fixed_bytes == 8,  "131 = 8 bytes");
        CHECK(cat.items.at("132").fixed_bytes == 1,  "132 = 1 byte");
        CHECK(cat.items.at("140").fixed_bytes == 2,  "140 = 2 bytes");
        CHECK(cat.items.at("145").fixed_bytes == 2,  "145 = 2 bytes");
        CHECK(cat.items.at("146").fixed_bytes == 2,  "146 = 2 bytes");
        CHECK(cat.items.at("148").fixed_bytes == 2,  "148 = 2 bytes");
        CHECK(cat.items.at("150").fixed_bytes == 2,  "150 = 2 bytes");
        CHECK(cat.items.at("151").fixed_bytes == 2,  "151 = 2 bytes");
        CHECK(cat.items.at("152").fixed_bytes == 2,  "152 = 2 bytes");
        CHECK(cat.items.at("155").fixed_bytes == 2,  "155 = 2 bytes");
        CHECK(cat.items.at("157").fixed_bytes == 2,  "157 = 2 bytes");
        CHECK(cat.items.at("160").fixed_bytes == 4,  "160 = 4 bytes");
        CHECK(cat.items.at("161").fixed_bytes == 2,  "161 = 2 bytes");
        CHECK(cat.items.at("165").fixed_bytes == 2,  "165 = 2 bytes");
        CHECK(cat.items.at("170").fixed_bytes == 6,  "170 = 6 bytes");
        CHECK(cat.items.at("200").fixed_bytes == 1,  "200 = 1 byte");
        CHECK(cat.items.at("210").fixed_bytes == 1,  "210 = 1 byte");
        CHECK(cat.items.at("230").fixed_bytes == 2,  "230 = 2 bytes");
        CHECK(cat.items.at("260").fixed_bytes == 7,  "260 = 7 bytes");
        CHECK(cat.items.at("400").fixed_bytes == 1,  "400 = 1 byte");

        // Extended octets
        CHECK(cat.items.at("040").octets.size() == 5, "040 has 5 octets");
        CHECK(cat.items.at("090").octets.size() == 9, "090 has 9 octets");
        CHECK(cat.items.at("271").octets.size() == 2, "271 has 2 octets");

        // I250 RepetitiveGroup: 64-bit group
        CHECK(cat.items.at("250").rep_group_bits == 64, "250 group = 64 bits");

        // I220 Compound: 4 sub-items
        CHECK(cat.items.at("220").compound_sub_items.size() == 4, "220 has 4 sub-items");
        const auto& si220 = cat.items.at("220").compound_sub_items;
        CHECK(si220[0].name == "WS"  && si220[0].fixed_bytes == 2, "220.WS  = 2 bytes");
        CHECK(si220[1].name == "WD"  && si220[1].fixed_bytes == 2, "220.WD  = 2 bytes");
        CHECK(si220[2].name == "TMP" && si220[2].fixed_bytes == 2, "220.TMP = 2 bytes");
        CHECK(si220[3].name == "TRB" && si220[3].fixed_bytes == 1, "220.TRB = 1 byte");

        // I295 Compound: 23 sub-items
        CHECK(cat.items.at("295").compound_sub_items.size() == 23, "295 has 23 sub-items");

        // I110 Compound: 2 sub-items (TIS + unused TID)
        CHECK(cat.items.at("110").compound_sub_items.size() == 2, "110 has 2 sub-items");
        const auto& si110 = cat.items.at("110").compound_sub_items;
        CHECK(si110[0].name == "TIS" && si110[0].fixed_bytes == 1, "110.TIS = 1 byte");
        CHECK(si110[1].name == "-",                                  "110 slot1 = unused (TID)");

        codec.registerCategory(std::move(cat));
    } catch (const std::exception& e) {
        std::cerr << "FAIL spec load: " << e.what() << '\n';
        ++failures;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode a hand-crafted basic ADS-B target report
//
//  Items: I010, I040, I130, I080, I210
//
//  UAP slots:
//    I010 → slot  1 → FSPEC byte 1, bit 7
//    I040 → slot  2 → FSPEC byte 1, bit 6
//    I130 → slot  6 → FSPEC byte 1, bit 2
//    I080 → slot 11 → FSPEC byte 2, bit 4
//    I210 → slot 18 → FSPEC byte 3, bit 4
//
//  FSPEC byte 1: [I010][I040][ 0 ][ 0 ][ 0 ][I130][ 0 ][FX=1]  = 0xC5
//  FSPEC byte 2: [ 0  ][ 0  ][ 0 ][I080][ 0 ][ 0  ][ 0 ][FX=1] = 0x11
//  FSPEC byte 3: [ 0  ][ 0  ][ 0 ][I210][ 0 ][ 0  ][ 0 ][FX=0] = 0x10
//
//  I010: SAC=1, SIC=2                → 0x01, 0x02
//  I040: ATP=0,ARC=0,RC=0,RAB=0,FX=0 → 0x00
//  I130: LAT=1310720, LON=327680
//        LAT = 0x140000 → 0x14,0x00,0x00
//        LON = 0x050000 → 0x05,0x00,0x00
//  I080: ADR=0x4840D6                → 0x48,0x40,0xD6
//  I210: spare=0,VNS=0,VN=2,LTT=2   → 0x12
//
//  Payload = 3+2+1+6+3+1 = 16; Total = 19 = 0x13
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicADSBReport(const Codec& codec) {
    std::cout << "\n=== Test: Decode CAT21 basic ADS-B target report ===\n";

    std::vector<uint8_t> frame = {
        0x15,              // CAT=21
        0x00, 0x13,        // LEN=19
        0xC5,              // FSPEC byte 1: I010,I040,I130; FX=1
        0x11,              // FSPEC byte 2: I080; FX=1
        0x10,              // FSPEC byte 3: I210; FX=0
        0x01, 0x02,        // I010: SAC=1, SIC=2
        0x00,              // I040: ATP=0,ARC=0,RC=0,RAB=0, FX=0
        0x14, 0x00, 0x00,  // I130: LAT raw = 0x140000 = 1310720
        0x05, 0x00, 0x00,  // I130: LON raw = 0x050000 = 327680
        0x48, 0x40, 0xD6,  // I080: ADR = 0x4840D6
        0x12,              // I210: VN=2 (ED102A/DO-260B), LTT=2 (1090 ES)
    };

    hexdump(frame, "input");
    DecodedBlock block = codec.decode(frame);

    CHECK(block.valid,        "block.valid");
    CHECK(block.cat == 21,    "block.cat == 21");
    CHECK(block.length == 19, "block.length == 19");
    CHECK(block.records.size() == 1, "one record");

    if (block.records.empty()) return;
    const auto& rec = block.records[0];
    CHECK(rec.valid,                       "record.valid");
    CHECK(rec.uap_variation == "default",  "UAP = default");

    CHECK(rec.items.count("010"), "I010 present");
    CHECK(rec.items.count("040"), "I040 present");
    CHECK(rec.items.count("130"), "I130 present");
    CHECK(rec.items.count("080"), "I080 present");
    CHECK(rec.items.count("210"), "I210 present");
    CHECK(!rec.items.count("145"), "I145 absent");
    CHECK(!rec.items.count("160"), "I160 absent");

    if (rec.items.count("010")) {
        CHECK(rec.items.at("010").fields.at("SAC") == 1, "SAC == 1");
        CHECK(rec.items.at("010").fields.at("SIC") == 2, "SIC == 2");
    }
    if (rec.items.count("040")) {
        CHECK(rec.items.at("040").fields.at("ATP") == 0, "ATP == 0 (24-bit ICAO)");
        CHECK(rec.items.at("040").fields.at("ARC") == 0, "ARC == 0 (25 ft)");
    }
    if (rec.items.count("130")) {
        CHECK(rec.items.at("130").fields.at("LAT") == 1310720, "LAT raw == 1310720");
        CHECK(rec.items.at("130").fields.at("LON") == 327680,  "LON raw == 327680");
    }
    if (rec.items.count("080"))
        CHECK(rec.items.at("080").fields.at("ADR") == 0x4840D6, "ADR == 0x4840D6");
    if (rec.items.count("210")) {
        CHECK(rec.items.at("210").fields.at("VNS") == 0, "VNS == 0");
        CHECK(rec.items.at("210").fields.at("VN")  == 2, "VN  == 2 (ED102A/DO-260B)");
        CHECK(rec.items.at("210").fields.at("LTT") == 2, "LTT == 2 (1090 ES)");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip for I021/040 Extended (Target Report Descriptor)
//          – two octets: ATP/ARC/RC/RAB + DCR/GBS/SIM/TST/SAA/CL
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtended040(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip I021/040 Extended (2 octets) ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 2; di.fields["SIC"] = 5;
      src.items["010"] = std::move(di); }

    // I040: ATP=2 (surface vehicle), ARC=1 (100 ft), RC=0, RAB=0
    //       DCR=1, GBS=0, SIM=0, TST=0, SAA=0, CL=1 (suspect)
    { DecodedItem di; di.item_id = "040"; di.type = ItemType::Extended;
      di.fields["ATP"]  = 2;
      di.fields["ARC"]  = 1;
      di.fields["RC"]   = 0;
      di.fields["RAB"]  = 0;
      di.fields["DCR"]  = 1;
      di.fields["GBS"]  = 0;
      di.fields["SIM"]  = 0;
      di.fields["TST"]  = 0;
      di.fields["SAA"]  = 0;
      di.fields["CL"]   = 1;
      src.items["040"] = std::move(di); }

    auto encoded = codec.encode(21, {src});
    hexdump(encoded, "encoded");
    CHECK(encoded.size() >= 3, "encoded non-empty");

    auto block = codec.decode(encoded);
    CHECK(block.valid, "RT block valid");
    CHECK(block.cat == 21, "RT cat == 21");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");
    CHECK(rec.items.count("040"), "RT I040 present");

    if (rec.items.count("040")) {
        CHECK(rec.items.at("040").fields.at("ATP") == 2, "RT ATP == 2");
        CHECK(rec.items.at("040").fields.at("ARC") == 1, "RT ARC == 1");
        CHECK(rec.items.at("040").fields.at("RC")  == 0, "RT RC == 0");
        CHECK(rec.items.at("040").fields.at("RAB") == 0, "RT RAB == 0");
        CHECK(rec.items.at("040").fields.at("DCR") == 1, "RT DCR == 1");
        CHECK(rec.items.at("040").fields.at("GBS") == 0, "RT GBS == 0");
        CHECK(rec.items.at("040").fields.at("SIM") == 0, "RT SIM == 0");
        CHECK(rec.items.at("040").fields.at("CL")  == 1, "RT CL == 1");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip for I021/090 Extended (Quality Indicators, 2 octets)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtended090(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip I021/090 Extended Quality Indicators ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 3; di.fields["SIC"] = 3;
      src.items["010"] = std::move(di); }

    // I090 octet 1: NUCRNACV=3, NUCPNIC=5
    // I090 octet 2: NICBARO=1, SIL=2, NACP=8
    { DecodedItem di; di.item_id = "090"; di.type = ItemType::Extended;
      di.fields["NUCRNACV"] = 3;
      di.fields["NUCPNIC"]  = 5;
      di.fields["NICBARO"]  = 1;
      di.fields["SIL"]      = 2;
      di.fields["NACP"]     = 8;
      src.items["090"] = std::move(di); }

    auto encoded = codec.encode(21, {src});
    hexdump(encoded, "encoded");
    CHECK(encoded.size() >= 3, "encoded non-empty");

    auto block = codec.decode(encoded);
    CHECK(block.valid, "RT block valid");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");
    CHECK(rec.items.count("090"), "RT I090 present");

    if (rec.items.count("090")) {
        CHECK(rec.items.at("090").fields.at("NUCRNACV") == 3, "RT NUCRNACV == 3");
        CHECK(rec.items.at("090").fields.at("NUCPNIC")  == 5, "RT NUCPNIC == 5");
        CHECK(rec.items.at("090").fields.at("NICBARO")  == 1, "RT NICBARO == 1");
        CHECK(rec.items.at("090").fields.at("SIL")      == 2, "RT SIL == 2");
        CHECK(rec.items.at("090").fields.at("NACP")     == 8, "RT NACP == 8");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip for I021/271 Extended (Surface Capabilities, 2 octets)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtended271(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip I021/271 Extended Surface Capabilities ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 4; di.fields["SIC"] = 4;
      src.items["010"] = std::move(di); }

    // I271 octet 1: spare, POA=1, CDTIS=0, B2LOW=1, RAS=1, IDENT=0
    // I271 octet 2: LW=5, spare
    { DecodedItem di; di.item_id = "271"; di.type = ItemType::Extended;
      di.fields["POA"]   = 1;
      di.fields["CDTIS"] = 0;
      di.fields["B2LOW"] = 1;
      di.fields["RAS"]   = 1;
      di.fields["IDENT"] = 0;
      di.fields["LW"]    = 5;
      src.items["271"] = std::move(di); }

    auto encoded = codec.encode(21, {src});
    hexdump(encoded, "encoded");

    auto block = codec.decode(encoded);
    CHECK(block.valid, "RT block valid");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");
    CHECK(rec.items.count("271"), "RT I271 present");

    if (rec.items.count("271")) {
        CHECK(rec.items.at("271").fields.at("POA")   == 1, "RT POA == 1");
        CHECK(rec.items.at("271").fields.at("CDTIS") == 0, "RT CDTIS == 0");
        CHECK(rec.items.at("271").fields.at("B2LOW") == 1, "RT B2LOW == 1");
        CHECK(rec.items.at("271").fields.at("RAS")   == 1, "RT RAS == 1");
        CHECK(rec.items.at("271").fields.at("IDENT") == 0, "RT IDENT == 0");
        CHECK(rec.items.at("271").fields.at("LW")    == 5, "RT LW == 5");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip for I021/250 Mode S MB Data (RepetitiveGroup)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripModeSMBData(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip I021/250 Mode S MB Data ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 5; di.fields["SIC"] = 9;
      src.items["010"] = std::move(di); }

    // I250: 2 BDS register entries (64 bits each)
    { DecodedItem di; di.item_id = "250"; di.type = ItemType::RepetitiveGroup;
      di.group_repetitions.push_back({{"MBDATA", 0xA0B0C0D0E0F01020ULL}});
      di.group_repetitions.push_back({{"MBDATA", 0x1122334455667788ULL}});
      src.items["250"] = std::move(di); }

    auto encoded = codec.encode(21, {src});
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
            CHECK(grps[0].at("MBDATA") == 0xA0B0C0D0E0F01020ULL, "RT grp[0].MBDATA");
            CHECK(grps[1].at("MBDATA") == 0x1122334455667788ULL, "RT grp[1].MBDATA");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip for I021/220 Met Information (Compound)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripMetInformation(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip I021/220 Met Information (Compound) ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 6; di.fields["SIC"] = 10;
      src.items["010"] = std::move(di); }

    // I220: WS=15 kt, WD=270°, TMP=-20°C (raw=0xFFB0=65456 as 16-bit two's complement), TRB=2
    { DecodedItem di; di.item_id = "220"; di.type = ItemType::Compound;
      di.compound_sub_fields["WS"]  = {{"WS",  15}};
      di.compound_sub_fields["WD"]  = {{"WD",  270}};
      di.compound_sub_fields["TMP"] = {{"TMP", static_cast<uint64_t>(static_cast<uint16_t>(static_cast<int16_t>(-80)))}};
      di.compound_sub_fields["TRB"] = {{"TRB", 2}};
      src.items["220"] = std::move(di); }

    auto encoded = codec.encode(21, {src});
    hexdump(encoded, "encoded");
    CHECK(encoded.size() >= 3, "encoded non-empty");

    auto block = codec.decode(encoded);
    CHECK(block.valid, "RT block valid");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");
    CHECK(rec.items.count("220"), "RT I220 present");

    if (rec.items.count("220")) {
        const auto& i220 = rec.items.at("220");
        CHECK(i220.compound_sub_fields.count("WS"),  "RT I220.WS present");
        CHECK(i220.compound_sub_fields.count("WD"),  "RT I220.WD present");
        CHECK(i220.compound_sub_fields.count("TMP"), "RT I220.TMP present");
        CHECK(i220.compound_sub_fields.count("TRB"), "RT I220.TRB present");
        if (i220.compound_sub_fields.count("WS"))
            CHECK(i220.compound_sub_fields.at("WS").at("WS")   == 15,  "RT WS == 15");
        if (i220.compound_sub_fields.count("WD"))
            CHECK(i220.compound_sub_fields.at("WD").at("WD")   == 270, "RT WD == 270");
        if (i220.compound_sub_fields.count("TMP"))
            CHECK(i220.compound_sub_fields.at("TMP").at("TMP") ==
                  static_cast<uint64_t>(static_cast<uint16_t>(static_cast<int16_t>(-80))),
                  "RT TMP == 0xFFB0 (−20°C raw)");
        if (i220.compound_sub_fields.count("TRB"))
            CHECK(i220.compound_sub_fields.at("TRB").at("TRB") == 2, "RT TRB == 2");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Round-trip for I021/295 Data Ages (Compound, multi-PSF-byte)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripDataAges(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip I021/295 Data Ages (Compound) ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 7; di.fields["SIC"] = 11;
      src.items["010"] = std::move(di); }

    // I295: set a few ages across different PSF bytes
    { DecodedItem di; di.item_id = "295"; di.type = ItemType::Compound;
      di.compound_sub_fields["AOS"] = {{"AOS", 5}};   // 0.5 s
      di.compound_sub_fields["M3A"] = {{"M3A", 10}};  // 1.0 s
      di.compound_sub_fields["GH"]  = {{"GH",  25}};  // 2.5 s
      di.compound_sub_fields["FL"]  = {{"FL",  30}};  // 3.0 s (PSF byte 2)
      di.compound_sub_fields["GVR"] = {{"GVR", 15}};  // 1.5 s (PSF byte 3)
      di.compound_sub_fields["ARA"] = {{"ARA", 20}};  // 2.0 s (PSF byte 4)
      src.items["295"] = std::move(di); }

    auto encoded = codec.encode(21, {src});
    hexdump(encoded, "encoded");
    CHECK(encoded.size() >= 3, "encoded non-empty");

    auto block = codec.decode(encoded);
    CHECK(block.valid, "RT block valid");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");
    CHECK(rec.items.count("295"), "RT I295 present");

    if (rec.items.count("295")) {
        const auto& i295 = rec.items.at("295");
        CHECK(i295.compound_sub_fields.count("AOS"), "RT I295.AOS present");
        CHECK(i295.compound_sub_fields.count("M3A"), "RT I295.M3A present");
        CHECK(i295.compound_sub_fields.count("GH"),  "RT I295.GH present");
        CHECK(i295.compound_sub_fields.count("FL"),  "RT I295.FL present");
        CHECK(i295.compound_sub_fields.count("GVR"), "RT I295.GVR present");
        CHECK(i295.compound_sub_fields.count("ARA"), "RT I295.ARA present");
        CHECK(!i295.compound_sub_fields.count("TRD"), "RT I295.TRD absent");
        if (i295.compound_sub_fields.count("AOS"))
            CHECK(i295.compound_sub_fields.at("AOS").at("AOS") == 5,  "RT AOS == 5");
        if (i295.compound_sub_fields.count("M3A"))
            CHECK(i295.compound_sub_fields.at("M3A").at("M3A") == 10, "RT M3A == 10");
        if (i295.compound_sub_fields.count("GH"))
            CHECK(i295.compound_sub_fields.at("GH").at("GH")   == 25, "RT GH == 25");
        if (i295.compound_sub_fields.count("FL"))
            CHECK(i295.compound_sub_fields.at("FL").at("FL")   == 30, "RT FL == 30");
        if (i295.compound_sub_fields.count("GVR"))
            CHECK(i295.compound_sub_fields.at("GVR").at("GVR") == 15, "RT GVR == 15");
        if (i295.compound_sub_fields.count("ARA"))
            CHECK(i295.compound_sub_fields.at("ARA").at("ARA") == 20, "RT ARA == 20");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 9: Full encode-decode round-trip across all major item types
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(const Codec& codec) {
    std::cout << "\n=== Test: Full CAT21 encode-decode round-trip ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    // I010 – Data Source Identification
    { DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
      di.fields["SAC"] = 12; di.fields["SIC"] = 34;
      src.items["010"] = std::move(di); }

    // I080 – Target Address
    { DecodedItem di; di.item_id = "080"; di.type = ItemType::Fixed;
      di.fields["ADR"] = 0x3C4E71;
      src.items["080"] = std::move(di); }

    // I130 – Position in WGS-84 (signed; ~43° lat, ~7° lon)
    { DecodedItem di; di.item_id = "130"; di.type = ItemType::Fixed;
      di.fields["LAT"] = 2003943;   // ≈ 43.0°
      di.fields["LON"] = 327680;    // ≈  7.0°
      src.items["130"] = std::move(di); }

    // I040 – Target Report Descriptor (Extended, 1 octet)
    { DecodedItem di; di.item_id = "040"; di.type = ItemType::Extended;
      di.fields["ATP"] = 0;
      di.fields["ARC"] = 0;
      di.fields["RC"]  = 0;
      di.fields["RAB"] = 0;
      src.items["040"] = std::move(di); }

    // I210 – MOPS Version
    { DecodedItem di; di.item_id = "210"; di.type = ItemType::Fixed;
      di.fields["VNS"] = 0;
      di.fields["VN"]  = 2;
      di.fields["LTT"] = 2;
      src.items["210"] = std::move(di); }

    // I140 – Geometric Height (signed, 16-bit; 10000 ft → raw=1600)
    { DecodedItem di; di.item_id = "140"; di.type = ItemType::Fixed;
      di.fields["GH"] = 1600;
      src.items["140"] = std::move(di); }

    // I145 – Flight Level (signed; FL350 → raw=1400)
    { DecodedItem di; di.item_id = "145"; di.type = ItemType::Fixed;
      di.fields["FL"] = 1400;
      src.items["145"] = std::move(di); }

    // I160 – Airborne Ground Vector
    { DecodedItem di; di.item_id = "160"; di.type = ItemType::Fixed;
      di.fields["RE"] = 0;
      di.fields["GS"] = 16384;   // ≈ 1.0 NM/s
      di.fields["TA"] = 8192;    // ≈ 45° track angle
      src.items["160"] = std::move(di); }

    // I200 – Target Status
    { DecodedItem di; di.item_id = "200"; di.type = ItemType::Fixed;
      di.fields["ICF"]  = 0;
      di.fields["LNAV"] = 0;
      di.fields["ME"]   = 0;
      di.fields["PS"]   = 0;
      di.fields["SS"]   = 0;
      src.items["200"] = std::move(di); }

    // I020 – Emitter Category
    { DecodedItem di; di.item_id = "020"; di.type = ItemType::Fixed;
      di.fields["ECAT"] = 3;  // 75000–300000 lbs medium aircraft
      src.items["020"] = std::move(di); }

    // I170 – Target Identification (raw 48 bits = "AFR123" in 6-bit ICAO)
    { DecodedItem di; di.item_id = "170"; di.type = ItemType::Fixed;
      di.fields["IDENT"] = 0x20C8649A2800ULL;
      src.items["170"] = std::move(di); }

    // I152 – Magnetic Heading (360° / 65536 per LSB; 180° → raw=32768)
    { DecodedItem di; di.item_id = "152"; di.type = ItemType::Fixed;
      di.fields["MH"] = 32768;
      src.items["152"] = std::move(di); }

    // I220 – Met Information (Compound)
    { DecodedItem di; di.item_id = "220"; di.type = ItemType::Compound;
      di.compound_sub_fields["WS"] = {{"WS", 20}};
      di.compound_sub_fields["WD"] = {{"WD", 180}};
      src.items["220"] = std::move(di); }

    // I008 – Aircraft Operational Status
    { DecodedItem di; di.item_id = "008"; di.type = ItemType::Fixed;
      di.fields["RA"]      = 0;
      di.fields["TC"]      = 2;
      di.fields["TS"]      = 1;
      di.fields["ARV"]     = 1;
      di.fields["CDTIA"]   = 0;
      di.fields["NOTTCAS"] = 0;
      di.fields["SA"]      = 0;
      src.items["008"] = std::move(di); }

    // I400 – Receiver ID
    { DecodedItem di; di.item_id = "400"; di.type = ItemType::Fixed;
      di.fields["RID"] = 7;
      src.items["400"] = std::move(di); }

    // Encode
    auto encoded = codec.encode(21, {src});
    hexdump(encoded, "encoded");
    CHECK(encoded.size() >= 3, "encoded non-empty");

    // Decode
    auto block = codec.decode(encoded);
    CHECK(block.valid, "RT block valid");
    CHECK(block.cat == 21, "RT cat == 21");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");
    CHECK(rec.uap_variation == "default", "RT UAP = default");

    // Verify all items are present
    CHECK(rec.items.count("010"), "RT I010 present");
    CHECK(rec.items.count("080"), "RT I080 present");
    CHECK(rec.items.count("130"), "RT I130 present");
    CHECK(rec.items.count("040"), "RT I040 present");
    CHECK(rec.items.count("210"), "RT I210 present");
    CHECK(rec.items.count("140"), "RT I140 present");
    CHECK(rec.items.count("145"), "RT I145 present");
    CHECK(rec.items.count("160"), "RT I160 present");
    CHECK(rec.items.count("200"), "RT I200 present");
    CHECK(rec.items.count("020"), "RT I020 present");
    CHECK(rec.items.count("170"), "RT I170 present");
    CHECK(rec.items.count("152"), "RT I152 present");
    CHECK(rec.items.count("220"), "RT I220 present");
    CHECK(rec.items.count("008"), "RT I008 present");
    CHECK(rec.items.count("400"), "RT I400 present");

    // Cross-check field values
    checkItemsMatch(rec.items, src.items, "FullRT");
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = (argc >= 2) ? argv[1] : "specs/CAT21.xml";
    if (!fs::exists(spec_path)) {
        std::cerr << "Spec file not found: " << spec_path << '\n';
        return 1;
    }

    Codec codec;

    testSpecLoad(codec, spec_path);
    testDecodeBasicADSBReport(codec);
    testRoundTripExtended040(codec);
    testRoundTripExtended090(codec);
    testRoundTripExtended271(codec);
    testRoundTripModeSMBData(codec);
    testRoundTripMetInformation(codec);
    testRoundTripDataAges(codec);
    testFullRoundTrip(codec);

    std::cout << '\n';
    if (failures == 0)
        std::cout << "All tests passed.\n";
    else
        std::cout << failures << " test(s) FAILED.\n";

    return failures == 0 ? 0 : 1;
}
