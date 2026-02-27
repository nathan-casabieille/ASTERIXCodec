// test_cat34.cpp – Tests for CAT34 decode/encode round-trip, including
//                  the new Compound item type used by I034/050 and I034/060.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat34

#include "ASTERIXCodec/Codec.hpp"
#include "ASTERIXCodec/SpecLoader.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
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
//  Test 1: XML spec loads without error
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT34 spec load ===\n";
    try {
        CategoryDef cat = loadSpec(spec_path);
        CHECK(cat.cat == 34,           "cat number = 34");
        CHECK(cat.items.count("010"), "item 010 present");
        CHECK(cat.items.count("000"), "item 000 present");
        CHECK(cat.items.count("020"), "item 020 present");
        CHECK(cat.items.count("030"), "item 030 present");
        CHECK(cat.items.count("041"), "item 041 present");
        CHECK(cat.items.count("050"), "item 050 present");
        CHECK(cat.items.count("060"), "item 060 present");
        CHECK(cat.items.count("070"), "item 070 present");
        CHECK(cat.items.count("090"), "item 090 present");
        CHECK(cat.items.count("100"), "item 100 present");
        CHECK(cat.items.count("110"), "item 110 present");
        CHECK(cat.items.count("120"), "item 120 present");
        CHECK(cat.items.count("RE"),  "item RE present");
        CHECK(cat.items.count("SP"),  "item SP present");
        CHECK(cat.uap_variations.count("default"), "UAP variation 'default' exists");
        CHECK(!cat.uap_case.has_value(), "no UAP case discriminator");

        // Item type assertions
        CHECK(cat.items.at("010").type == ItemType::Fixed,          "010 is Fixed");
        CHECK(cat.items.at("000").type == ItemType::Fixed,          "000 is Fixed");
        CHECK(cat.items.at("020").type == ItemType::Fixed,          "020 is Fixed");
        CHECK(cat.items.at("030").type == ItemType::Fixed,          "030 is Fixed");
        CHECK(cat.items.at("041").type == ItemType::Fixed,          "041 is Fixed");
        CHECK(cat.items.at("050").type == ItemType::Compound,       "050 is Compound");
        CHECK(cat.items.at("060").type == ItemType::Compound,       "060 is Compound");
        CHECK(cat.items.at("070").type == ItemType::RepetitiveGroup,"070 is RepetitiveGroup");
        CHECK(cat.items.at("070").rep_group_bits == 16,             "070 group = 16 bits");
        CHECK(cat.items.at("090").type == ItemType::Fixed,          "090 is Fixed");
        CHECK(cat.items.at("100").type == ItemType::Fixed,          "100 is Fixed");
        CHECK(cat.items.at("110").type == ItemType::Fixed,          "110 is Fixed");
        CHECK(cat.items.at("120").type == ItemType::Fixed,          "120 is Fixed");
        CHECK(cat.items.at("SP").type  == ItemType::SP,             "SP is SP/Explicit");
        CHECK(cat.items.at("RE").type  == ItemType::SP,             "RE is SP/Explicit");

        // Compound sub-item count: COM + - + - + PSR + SSR + MDS = 6
        CHECK(cat.items.at("050").compound_sub_items.size() == 6,   "050 has 6 sub-items");
        CHECK(cat.items.at("060").compound_sub_items.size() == 6,   "060 has 6 sub-items");

        // Sub-item byte sizes for 050
        const auto& si050 = cat.items.at("050").compound_sub_items;
        CHECK(si050[0].name == "COM" && si050[0].fixed_bytes == 1, "050.COM = 1 byte");
        CHECK(si050[1].name == "-",                                 "050 slot1 = unused");
        CHECK(si050[2].name == "-",                                 "050 slot2 = unused");
        CHECK(si050[3].name == "PSR" && si050[3].fixed_bytes == 1, "050.PSR = 1 byte");
        CHECK(si050[4].name == "SSR" && si050[4].fixed_bytes == 1, "050.SSR = 1 byte");
        CHECK(si050[5].name == "MDS" && si050[5].fixed_bytes == 2, "050.MDS = 2 bytes");

        codec.registerCategory(std::move(cat));
    } catch (const std::exception& e) {
        std::cerr << "FAIL spec load: " << e.what() << '\n';
        ++failures;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode a hand-crafted North Marker message
//
//  CAT34 North Marker: I010 + I000 + I030
//  UAP order: slot1=010, slot2=000, slot3=030
//
//  FSPEC: slot1=1, slot2=1, slot3=1, rest=0, FX=0
//         bits 7..1: 1 1 1 0 0 0 0 | FX=0 → 0b11100000 = 0xE0
//
//  I034/010: SAC=0x05, SIC=0x0C
//  I034/000: MT=1 (North marker) → 0x01
//  I034/030: TOD = 43200s → raw = 43200 * 128 = 5529600 = 0x546000
//
//  Total: 3(header) + 1(fspec) + 2(I010) + 1(I000) + 3(I030) = 10 bytes
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeNorthMarker(const Codec& codec) {
    std::cout << "\n=== Test: Decode CAT34 North Marker message ===\n";

    std::vector<uint8_t> frame = {
        0x22,              // CAT=34
        0x00, 0x0A,        // LEN=10
        0xE0,              // FSPEC: I010(bit7)=1, I000(bit6)=1, I030(bit5)=1, FX=0
        0x05, 0x0C,        // I034/010: SAC=5, SIC=12
        0x01,              // I034/000: MT=1 (North marker)
        0x54, 0x60, 0x00,  // I034/030: TOD raw=5529600 (43200.0 s)
    };

    hexdump(frame, "input");
    DecodedBlock block = codec.decode(frame);

    CHECK(block.valid,        "block.valid");
    CHECK(block.cat == 34,    "block.cat == 34");
    CHECK(block.length == 10, "block.length == 10");
    CHECK(block.records.size() == 1, "one record");

    if (block.records.empty()) return;
    const auto& rec = block.records[0];
    CHECK(rec.valid, "record.valid");
    CHECK(rec.uap_variation == "default", "UAP variation = default");
    CHECK(rec.items.count("010"), "I010 present");
    CHECK(rec.items.count("000"), "I000 present");
    CHECK(rec.items.count("030"), "I030 present");
    CHECK(!rec.items.count("020"), "I020 absent");
    CHECK(!rec.items.count("050"), "I050 absent");

    if (rec.items.count("010")) {
        CHECK(rec.items.at("010").fields.at("SAC") == 5,  "SAC == 5");
        CHECK(rec.items.at("010").fields.at("SIC") == 12, "SIC == 12");
    }
    if (rec.items.count("000")) {
        CHECK(rec.items.at("000").fields.at("MT") == 1, "MT == 1 (North marker)");
    }
    if (rec.items.count("030")) {
        CHECK(rec.items.at("030").fields.at("TOD") == 5529600, "TOD raw == 5529600 (43200.0 s)");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Decode a message containing I034/050 Compound item (COM + PSR)
//
//  Items: I034/010, I034/000, I034/050
//  UAP: slot1=010(bit7), slot2=000(bit6), slot6=050(bit2)
//  FSPEC byte: 1,1,0,0,0,1,0,FX=0 → 0b11000100 = 0xC4
//
//  I034/050 PSF: COM(slot0→bit7)=1, PSR(slot3→bit4)=1, FX=0
//    → PSF = 0b10010000 = 0x90
//  COM byte: NOGO=0, RDPC=0, RDPR=0, OVLRDP=0, OVLXMT=0, MSC=0, TSV=0, spare=0
//    → 0x00
//  PSR byte: ANT=0, CHAB=1 (channel A only), OVL=0, MSC=0, spare=0
//    → bits: 0 01 0 0 000 = 0b00100000 = 0x20
//
//  Total: 3(header) + 1(fspec) + 2(I010) + 1(I000) + 3(I050: PSF+COM+PSR) = 10 bytes
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeCompound050(const Codec& codec) {
    std::cout << "\n=== Test: Decode I034/050 Compound item (COM + PSR) ===\n";

    std::vector<uint8_t> frame = {
        0x22,              // CAT=34
        0x00, 0x0A,        // LEN=10
        0xC4,              // FSPEC: I010(bit7), I000(bit6), I050(bit2); FX=0
        0x05, 0x0C,        // I034/010: SAC=5, SIC=12
        0x01,              // I034/000: MT=1 (North marker)
        0x90,              // I034/050 PSF: COM(bit7)=1, PSR(bit4)=1, FX=0
        0x00,              // I034/050 COM: all zeros (system operational)
        0x20,              // I034/050 PSR: ANT=0, CHAB=1(ChA), OVL=0, MSC=0
    };

    hexdump(frame, "input");
    DecodedBlock block = codec.decode(frame);

    CHECK(block.valid,        "block.valid");
    CHECK(block.cat == 34,    "block.cat == 34");
    CHECK(block.length == 10, "block.length == 10");
    CHECK(block.records.size() == 1, "one record");

    if (block.records.empty()) return;
    const auto& rec = block.records[0];
    CHECK(rec.valid, "record.valid");
    CHECK(rec.items.count("050"), "I050 present");

    if (!rec.items.count("050")) return;
    const auto& i050 = rec.items.at("050");
    CHECK(i050.type == ItemType::Compound, "I050 type == Compound");
    CHECK(i050.compound_sub_fields.count("COM"), "I050.COM present");
    CHECK(i050.compound_sub_fields.count("PSR"), "I050.PSR present");
    CHECK(!i050.compound_sub_fields.count("SSR"), "I050.SSR absent");
    CHECK(!i050.compound_sub_fields.count("MDS"), "I050.MDS absent");

    if (i050.compound_sub_fields.count("COM")) {
        CHECK(i050.compound_sub_fields.at("COM").at("NOGO")   == 0, "COM.NOGO == 0");
        CHECK(i050.compound_sub_fields.at("COM").at("RDPC")   == 0, "COM.RDPC == 0");
        CHECK(i050.compound_sub_fields.at("COM").at("MSC")    == 0, "COM.MSC == 0");
        CHECK(i050.compound_sub_fields.at("COM").at("TSV")    == 0, "COM.TSV == 0");
    }
    if (i050.compound_sub_fields.count("PSR")) {
        CHECK(i050.compound_sub_fields.at("PSR").at("ANT")  == 0, "PSR.ANT == 0 (antenna 1)");
        CHECK(i050.compound_sub_fields.at("PSR").at("CHAB") == 1, "PSR.CHAB == 1 (Channel A only)");
        CHECK(i050.compound_sub_fields.at("PSR").at("OVL")  == 0, "PSR.OVL == 0");
        CHECK(i050.compound_sub_fields.at("PSR").at("MSC")  == 0, "PSR.MSC == 0");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip for Compound I034/050 with all sub-items (COM+PSR+SSR+MDS)
//
//  I034/050 PSF: COM(slot0→bit7), PSR(slot3→bit4), SSR(slot4→bit3), MDS(slot5→bit2)
//    → PSF = 0b10011100 = 0x9C
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCompound050Full(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip I034/050 with COM+PSR+SSR+MDS ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    // I034/010: SAC=8, SIC=17
    {
        DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
        di.fields["SAC"] = 8; di.fields["SIC"] = 17;
        src.items["010"] = std::move(di);
    }
    // I034/000: MT=2 (Sector crossing)
    {
        DecodedItem di; di.item_id = "000"; di.type = ItemType::Fixed;
        di.fields["MT"] = 2;
        src.items["000"] = std::move(di);
    }
    // I034/050: COM + PSR + SSR + MDS
    {
        DecodedItem di; di.item_id = "050"; di.type = ItemType::Compound;
        // COM: NOGO=0, RDPC=1, RDPR=0, OVLRDP=0, OVLXMT=0, MSC=0, TSV=0
        di.compound_sub_fields["COM"] = {
            {"NOGO", 0}, {"RDPC", 1}, {"RDPR", 0},
            {"OVLRDP", 0}, {"OVLXMT", 0}, {"MSC", 0}, {"TSV", 0}
        };
        // PSR: ANT=1 (antenna 2), CHAB=3 (diversity), OVL=0, MSC=0
        di.compound_sub_fields["PSR"] = {
            {"ANT", 1}, {"CHAB", 3}, {"OVL", 0}, {"MSC", 0}
        };
        // SSR: ANT=0, CHAB=2 (channel B only), OVL=1, MSC=0
        di.compound_sub_fields["SSR"] = {
            {"ANT", 0}, {"CHAB", 2}, {"OVL", 1}, {"MSC", 0}
        };
        // MDS: ANT=0, CHAB=1, OVLSUR=0, MSC=0, SCF=1, DLF=0, OVLSCF=0, OVLDLF=0
        di.compound_sub_fields["MDS"] = {
            {"ANT", 0}, {"CHAB", 1}, {"OVLSUR", 0}, {"MSC", 0},
            {"SCF", 1}, {"DLF", 0}, {"OVLSCF", 0}, {"OVLDLF", 0}
        };
        src.items["050"] = std::move(di);
    }

    auto encoded = codec.encode(34, {src});
    hexdump(encoded, "encoded");
    CHECK(encoded.size() >= 3, "encoded block non-empty");

    auto block = codec.decode(encoded);
    CHECK(block.valid,       "RT block valid");
    CHECK(block.cat == 34,   "RT cat == 34");
    CHECK(block.records.size() == 1, "RT one record");

    if (block.records.empty()) return;
    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");
    CHECK(rec.items.count("050"), "RT I050 present");

    if (!rec.items.count("050")) return;
    const auto& i050 = rec.items.at("050");
    CHECK(i050.compound_sub_fields.count("COM"), "RT I050.COM present");
    CHECK(i050.compound_sub_fields.count("PSR"), "RT I050.PSR present");
    CHECK(i050.compound_sub_fields.count("SSR"), "RT I050.SSR present");
    CHECK(i050.compound_sub_fields.count("MDS"), "RT I050.MDS present");

    if (i050.compound_sub_fields.count("COM"))
        CHECK(i050.compound_sub_fields.at("COM").at("RDPC") == 1, "RT COM.RDPC == 1");
    if (i050.compound_sub_fields.count("PSR")) {
        CHECK(i050.compound_sub_fields.at("PSR").at("ANT")  == 1, "RT PSR.ANT == 1");
        CHECK(i050.compound_sub_fields.at("PSR").at("CHAB") == 3, "RT PSR.CHAB == 3");
    }
    if (i050.compound_sub_fields.count("SSR")) {
        CHECK(i050.compound_sub_fields.at("SSR").at("CHAB") == 2, "RT SSR.CHAB == 2");
        CHECK(i050.compound_sub_fields.at("SSR").at("OVL")  == 1, "RT SSR.OVL == 1");
    }
    if (i050.compound_sub_fields.count("MDS")) {
        CHECK(i050.compound_sub_fields.at("MDS").at("CHAB") == 1, "RT MDS.CHAB == 1");
        CHECK(i050.compound_sub_fields.at("MDS").at("SCF")  == 1, "RT MDS.SCF == 1");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip for I034/060 (System Processing Mode) Compound
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCompound060(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip I034/060 Compound (COM+PSR+SSR+MDS) ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    {
        DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
        di.fields["SAC"] = 3; di.fields["SIC"] = 5;
        src.items["010"] = std::move(di);
    }
    {
        DecodedItem di; di.item_id = "000"; di.type = ItemType::Fixed;
        di.fields["MT"] = 1;
        src.items["000"] = std::move(di);
    }
    // I034/060: COM(REDRDP=2,REDXMT=0) + PSR(POL=0,REDRAD=3,STC=1) + SSR(REDRAD=1) + MDS(REDRAD=2,CLU=1)
    {
        DecodedItem di; di.item_id = "060"; di.type = ItemType::Compound;
        di.compound_sub_fields["COM"] = {{"REDRDP", 2}, {"REDXMT", 0}};
        di.compound_sub_fields["PSR"] = {{"POL", 0}, {"REDRAD", 3}, {"STC", 1}};
        di.compound_sub_fields["SSR"] = {{"REDRAD", 1}};
        di.compound_sub_fields["MDS"] = {{"REDRAD", 2}, {"CLU", 1}};
        src.items["060"] = std::move(di);
    }

    auto encoded = codec.encode(34, {src});
    hexdump(encoded, "encoded");

    auto block = codec.decode(encoded);
    CHECK(block.valid, "RT block valid");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");
    CHECK(rec.items.count("060"), "RT I060 present");

    if (!rec.items.count("060")) return;
    const auto& i060 = rec.items.at("060");
    CHECK(i060.compound_sub_fields.count("COM"), "RT I060.COM present");
    CHECK(i060.compound_sub_fields.count("PSR"), "RT I060.PSR present");
    CHECK(i060.compound_sub_fields.count("SSR"), "RT I060.SSR present");
    CHECK(i060.compound_sub_fields.count("MDS"), "RT I060.MDS present");

    if (i060.compound_sub_fields.count("COM")) {
        CHECK(i060.compound_sub_fields.at("COM").at("REDRDP") == 2, "RT COM.REDRDP == 2");
        CHECK(i060.compound_sub_fields.at("COM").at("REDXMT") == 0, "RT COM.REDXMT == 0");
    }
    if (i060.compound_sub_fields.count("PSR")) {
        CHECK(i060.compound_sub_fields.at("PSR").at("REDRAD") == 3, "RT PSR.REDRAD == 3");
        CHECK(i060.compound_sub_fields.at("PSR").at("STC")    == 1, "RT PSR.STC == 1");
    }
    if (i060.compound_sub_fields.count("SSR"))
        CHECK(i060.compound_sub_fields.at("SSR").at("REDRAD") == 1, "RT SSR.REDRAD == 1");
    if (i060.compound_sub_fields.count("MDS")) {
        CHECK(i060.compound_sub_fields.at("MDS").at("REDRAD") == 2, "RT MDS.REDRAD == 2");
        CHECK(i060.compound_sub_fields.at("MDS").at("CLU")    == 1, "RT MDS.CLU == 1");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Decode I034/070 Message Count Values (RepetitiveGroup)
//
//  Group layout: TYP(5b) + COUNT(11b) = 16 bits = 2 bytes each.
//
//  UAP slot 8 = I070, so we need two FSPEC bytes.
//  FSPEC byte 1: I010(bit7), I000(bit6), FX(bit0)=1 → 0xC1
//  FSPEC byte 2: I070(bit7), FX(bit0)=0             → 0x80
//
//  I034/010: SAC=1, SIC=2 → 0x01, 0x02
//  I034/000: MT=1 → 0x01
//  I034/070: REP=2
//    Group 1: TYP=1 (Single PSR), COUNT=100
//      bits: 00001|00001100100 → 0x08, 0x64
//    Group 2: TYP=4 (All-Call Mode S), COUNT=50
//      bits: 00100|00000110010 → 0x20, 0x32
//
//  Total: 3(header) + 2(fspec) + 2(I010) + 1(I000) + 1+4(I070) = 13 bytes
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeMessageCountValues(const Codec& codec) {
    std::cout << "\n=== Test: Decode I034/070 Message Count Values (RepetitiveGroup) ===\n";

    std::vector<uint8_t> frame = {
        0x22,              // CAT=34
        0x00, 0x0D,        // LEN=13
        0xC1,              // FSPEC byte 1: I010(bit7)=1, I000(bit6)=1, FX(bit0)=1
        0x80,              // FSPEC byte 2: I070(bit7)=1, FX(bit0)=0
        0x01, 0x02,        // I034/010: SAC=1, SIC=2
        0x01,              // I034/000: MT=1 (North marker)
        0x02,              // I034/070: REP=2
        0x08, 0x64,        // Group 1: TYP=1, COUNT=100  (00001 00001100100)
        0x20, 0x32,        // Group 2: TYP=4, COUNT=50   (00100 00000110010)
    };

    hexdump(frame, "input");
    DecodedBlock block = codec.decode(frame);

    CHECK(block.valid,        "block.valid");
    CHECK(block.cat == 34,    "block.cat == 34");
    CHECK(block.length == 13, "block.length == 13");
    CHECK(block.records.size() == 1, "one record");

    if (block.records.empty()) return;
    const auto& rec = block.records[0];
    CHECK(rec.valid, "record.valid");
    CHECK(rec.items.count("070"), "I070 present");

    if (rec.items.count("070")) {
        const auto& grps = rec.items.at("070").group_repetitions;
        CHECK(grps.size() == 2, "I070 has 2 groups");
        if (grps.size() >= 1) {
            CHECK(grps[0].at("TYP")   == 1,   "group[0].TYP == 1 (Single PSR)");
            CHECK(grps[0].at("COUNT") == 100,  "group[0].COUNT == 100");
        }
        if (grps.size() >= 2) {
            CHECK(grps[1].at("TYP")   == 4,   "group[1].TYP == 4 (All-Call Mode S)");
            CHECK(grps[1].at("COUNT") == 50,   "group[1].COUNT == 50");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip for Sector Crossing message with multiple items
//          I010, I000, I020, I030, I041, I090, I100
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripSectorCrossing(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip sector crossing message ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    // I034/010: SAC=5, SIC=7
    {
        DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
        di.fields["SAC"] = 5; di.fields["SIC"] = 7;
        src.items["010"] = std::move(di);
    }
    // I034/000: MT=2 (Sector crossing)
    {
        DecodedItem di; di.item_id = "000"; di.type = ItemType::Fixed;
        di.fields["MT"] = 2;
        src.items["000"] = std::move(di);
    }
    // I034/020: SN=64 raw → 64 × 1.40625 = 90°
    {
        DecodedItem di; di.item_id = "020"; di.type = ItemType::Fixed;
        di.fields["SN"] = 64;
        src.items["020"] = std::move(di);
    }
    // I034/030: TOD raw=6400 → 6400/128 = 50.0 s
    {
        DecodedItem di; di.item_id = "030"; di.type = ItemType::Fixed;
        di.fields["TOD"] = 6400;
        src.items["030"] = std::move(di);
    }
    // I034/041: ARS raw=2560 → 2560/128 = 20.0 s
    {
        DecodedItem di; di.item_id = "041"; di.type = ItemType::Fixed;
        di.fields["ARS"] = 2560;
        src.items["041"] = std::move(di);
    }
    // I034/090: RNG=5 (raw, signed), AZM=-3 (raw, signed)
    {
        DecodedItem di; di.item_id = "090"; di.type = ItemType::Fixed;
        di.fields["RNG"] = 5;
        di.fields["AZM"] = static_cast<uint64_t>(static_cast<int8_t>(-3)); // 0xFD
        src.items["090"] = std::move(di);
    }
    // I034/100: RHOST=256, RHOEND=512, THETAST=8192, THETAEND=16384
    {
        DecodedItem di; di.item_id = "100"; di.type = ItemType::Fixed;
        di.fields["RHOST"]    = 256;
        di.fields["RHOEND"]   = 512;
        di.fields["THETAST"]  = 8192;
        di.fields["THETAEND"] = 16384;
        src.items["100"] = std::move(di);
    }

    auto encoded = codec.encode(34, {src});
    hexdump(encoded, "encoded");
    CHECK(encoded.size() >= 3, "encoded block non-empty");

    auto block = codec.decode(encoded);
    CHECK(block.valid,       "RT block valid");
    CHECK(block.cat == 34,   "RT cat == 34");
    CHECK(block.records.size() == 1, "RT one record");

    if (block.records.empty()) return;
    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");
    CHECK(rec.uap_variation == "default", "RT UAP = default");

    if (rec.items.count("010")) {
        CHECK(rec.items.at("010").fields.at("SAC") == 5, "RT SAC == 5");
        CHECK(rec.items.at("010").fields.at("SIC") == 7, "RT SIC == 7");
    } else { std::cerr << "FAIL RT: I010 missing\n"; ++failures; }

    if (rec.items.count("000"))
        CHECK(rec.items.at("000").fields.at("MT") == 2, "RT MT == 2");
    else { std::cerr << "FAIL RT: I000 missing\n"; ++failures; }

    if (rec.items.count("020"))
        CHECK(rec.items.at("020").fields.at("SN") == 64, "RT SN == 64");
    else { std::cerr << "FAIL RT: I020 missing\n"; ++failures; }

    if (rec.items.count("030"))
        CHECK(rec.items.at("030").fields.at("TOD") == 6400, "RT TOD == 6400");
    else { std::cerr << "FAIL RT: I030 missing\n"; ++failures; }

    if (rec.items.count("041"))
        CHECK(rec.items.at("041").fields.at("ARS") == 2560, "RT ARS == 2560");
    else { std::cerr << "FAIL RT: I041 missing\n"; ++failures; }

    if (rec.items.count("090")) {
        CHECK(rec.items.at("090").fields.at("RNG") == 5, "RT RNG == 5");
        CHECK(rec.items.at("090").fields.at("AZM") == (uint64_t)(uint8_t)(-3), "RT AZM == -3 raw");
    } else { std::cerr << "FAIL RT: I090 missing\n"; ++failures; }

    if (rec.items.count("100")) {
        CHECK(rec.items.at("100").fields.at("RHOST")    == 256,   "RT RHOST == 256");
        CHECK(rec.items.at("100").fields.at("RHOEND")   == 512,   "RT RHOEND == 512");
        CHECK(rec.items.at("100").fields.at("THETAST")  == 8192,  "RT THETAST == 8192");
        CHECK(rec.items.at("100").fields.at("THETAEND") == 16384, "RT THETAEND == 16384");
    } else { std::cerr << "FAIL RT: I100 missing\n"; ++failures; }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Round-trip for I034/070 Message Count Values
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripMessageCountValues(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip I034/070 Message Count Values ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    {
        DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
        di.fields["SAC"] = 2; di.fields["SIC"] = 3;
        src.items["010"] = std::move(di);
    }
    {
        DecodedItem di; di.item_id = "000"; di.type = ItemType::Fixed;
        di.fields["MT"] = 1;
        src.items["000"] = std::move(di);
    }
    {
        DecodedItem di; di.item_id = "070"; di.type = ItemType::RepetitiveGroup;
        di.group_repetitions.push_back({{"TYP", 1},  {"COUNT", 200}});
        di.group_repetitions.push_back({{"TYP", 2},  {"COUNT", 150}});
        di.group_repetitions.push_back({{"TYP", 17}, {"COUNT", 42}});
        src.items["070"] = std::move(di);
    }

    auto encoded = codec.encode(34, {src});
    hexdump(encoded, "encoded");

    auto block = codec.decode(encoded);
    CHECK(block.valid, "RT block valid");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");
    CHECK(rec.items.count("070"), "RT I070 present");

    if (rec.items.count("070")) {
        const auto& grps = rec.items.at("070").group_repetitions;
        CHECK(grps.size() == 3, "RT I070 has 3 groups");
        if (grps.size() >= 3) {
            CHECK(grps[0].at("TYP")   == 1,   "RT group[0].TYP == 1");
            CHECK(grps[0].at("COUNT") == 200,  "RT group[0].COUNT == 200");
            CHECK(grps[1].at("TYP")   == 2,   "RT group[1].TYP == 2");
            CHECK(grps[1].at("COUNT") == 150,  "RT group[1].COUNT == 150");
            CHECK(grps[2].at("TYP")   == 17,   "RT group[2].TYP == 17");
            CHECK(grps[2].at("COUNT") == 42,   "RT group[2].COUNT == 42");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 9: Round-trip for I034/120 3D-Position Of Data Source
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTrip3DPosition(const Codec& codec) {
    std::cout << "\n=== Test: Round-trip I034/120 3D-Position ===\n";

    DecodedRecord src;
    src.uap_variation = "default";

    {
        DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
        di.fields["SAC"] = 1; di.fields["SIC"] = 1;
        src.items["010"] = std::move(di);
    }
    {
        DecodedItem di; di.item_id = "000"; di.type = ItemType::Fixed;
        di.fields["MT"] = 1;
        src.items["000"] = std::move(di);
    }
    // HGT = 100 m, LAT raw = 2000000 (~42.9°), LON raw = -1000000 (~-21.5°)
    {
        DecodedItem di; di.item_id = "120"; di.type = ItemType::Fixed;
        di.fields["HGT"] = 100;
        di.fields["LAT"] = 2000000u;
        // Encode -1000000 as 24-bit two's complement:
        //   uint32_t wraps to 0xFFF0BDC0, masked to 24 bits → 0xF0BDC0
        int32_t lon_s = -1000000;
        di.fields["LON"] = static_cast<uint64_t>(static_cast<uint32_t>(lon_s)) & 0xFFFFFFu;
        src.items["120"] = std::move(di);
    }

    auto encoded = codec.encode(34, {src});
    hexdump(encoded, "encoded");

    auto block = codec.decode(encoded);
    CHECK(block.valid, "RT block valid");
    if (block.records.empty()) { ++failures; return; }

    const auto& rec = block.records[0];
    CHECK(rec.valid, "RT record valid");
    CHECK(rec.items.count("120"), "RT I120 present");

    if (rec.items.count("120")) {
        CHECK(rec.items.at("120").fields.at("HGT") == 100, "RT HGT == 100");
        CHECK(rec.items.at("120").fields.at("LAT") == 2000000u, "RT LAT == 2000000");
        int32_t expected_lon_s = -1000000;
        uint64_t expected_lon = static_cast<uint64_t>(static_cast<uint32_t>(expected_lon_s)) & 0xFFFFFFu;
        CHECK(rec.items.at("120").fields.at("LON") == expected_lon, "RT LON == -1000000 raw");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 10: Multi-record North Marker + Sector Crossing in same block
// ─────────────────────────────────────────────────────────────────────────────
static void testMultiRecord(const Codec& codec) {
    std::cout << "\n=== Test: Multi-record block (North Marker + Sector Crossing) ===\n";

    DecodedRecord nm, sc;
    nm.uap_variation = sc.uap_variation = "default";

    {
        DecodedItem di; di.item_id = "010"; di.type = ItemType::Fixed;
        di.fields["SAC"] = 1; di.fields["SIC"] = 1;
        nm.items["010"] = di; sc.items["010"] = std::move(di);
    }
    {
        DecodedItem di; di.item_id = "000"; di.type = ItemType::Fixed;
        di.fields["MT"] = 1; nm.items["000"] = std::move(di);
    }
    {
        DecodedItem di; di.item_id = "000"; di.type = ItemType::Fixed;
        di.fields["MT"] = 2; sc.items["000"] = std::move(di);
    }
    {
        DecodedItem di; di.item_id = "020"; di.type = ItemType::Fixed;
        di.fields["SN"] = 128; // 128 * 1.40625 = 180°
        sc.items["020"] = std::move(di);
    }

    auto encoded = codec.encode(34, {nm, sc});
    hexdump(encoded, "encoded");

    auto block = codec.decode(encoded);
    CHECK(block.valid,       "multi-rec block valid");
    CHECK(block.cat == 34,   "multi-rec cat == 34");
    CHECK(block.records.size() == 2, "multi-rec: 2 records");

    if (block.records.size() >= 2) {
        CHECK(block.records[0].items.at("000").fields.at("MT") == 1,  "record[0] MT == 1");
        CHECK(block.records[1].items.at("000").fields.at("MT") == 2,  "record[1] MT == 2");
        CHECK(!block.records[0].items.count("020"), "record[0] no I020");
        CHECK(block.records[1].items.at("020").fields.at("SN") == 128, "record[1] SN == 128");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = (argc > 1)
        ? fs::path(argv[1])
        : fs::path(__FILE__).parent_path().parent_path() / "specs" / "CAT34.xml";

    std::cout << "Using spec: " << spec_path << '\n';

    Codec codec;
    testSpecLoad(codec, spec_path);

    if (failures == 0) {
        testDecodeNorthMarker(codec);
        testDecodeCompound050(codec);
        testRoundTripCompound050Full(codec);
        testRoundTripCompound060(codec);
        testDecodeMessageCountValues(codec);
        testRoundTripSectorCrossing(codec);
        testRoundTripMessageCountValues(codec);
        testRoundTrip3DPosition(codec);
        testMultiRecord(codec);
    }

    std::cout << "\n──────────────────────────────────\n";
    if (failures == 0) {
        std::cout << "ALL TESTS PASSED\n";
    } else {
        std::cout << failures << " TEST(S) FAILED\n";
    }
    return failures == 0 ? 0 : 1;
}
