// test_cat04.cpp – Tests for CAT04 Safety Net Messages.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat04

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

// Raw-bit helpers for signed fields (codec stores N-bit two's complement in uint64_t)
static uint64_t s16(int16_t v) { return static_cast<uint64_t>(static_cast<uint16_t>(v)); }
static uint64_t s24(int32_t v) { return static_cast<uint64_t>(static_cast<uint32_t>(v) & 0xFFFFFFu); }
static uint64_t s32(int32_t v) { return static_cast<uint64_t>(static_cast<uint32_t>(v)); }

// ─────────────────────────────────────────────────────────────────────────────
//  Test 1: XML spec loads correctly
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT04 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 4,          "cat number = 4");
    CHECK(cat.edition == "1.13", "edition = 1.13");

    for (auto id : {"000","010","015","020","030","035","040","045",
                    "060","070","074","075","076","100","110","120",
                    "170","171","RE","SP"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 20, "20 items total");

    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    CHECK(cat.uap_variations.at("default").size() == 21, "UAP has 21 slots");

    const auto& uap = cat.uap_variations.at("default");
    // Byte 1: 010 000 015 020 040 045 060
    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "000", "UAP slot  2 = 000");
    CHECK(uap[2]  == "015", "UAP slot  3 = 015");
    CHECK(uap[3]  == "020", "UAP slot  4 = 020");
    CHECK(uap[4]  == "040", "UAP slot  5 = 040");
    CHECK(uap[5]  == "045", "UAP slot  6 = 045");
    CHECK(uap[6]  == "060", "UAP slot  7 = 060");
    // Byte 2: 030 170 120 070 076 074 075
    CHECK(uap[7]  == "030", "UAP slot  8 = 030");
    CHECK(uap[8]  == "170", "UAP slot  9 = 170");
    CHECK(uap[9]  == "120", "UAP slot 10 = 120");
    CHECK(uap[10] == "070", "UAP slot 11 = 070");
    CHECK(uap[11] == "076", "UAP slot 12 = 076");
    CHECK(uap[12] == "074", "UAP slot 13 = 074");
    CHECK(uap[13] == "075", "UAP slot 14 = 075");
    // Byte 3: 100 035 171 110 - RE SP
    CHECK(uap[14] == "100", "UAP slot 15 = 100");
    CHECK(uap[15] == "035", "UAP slot 16 = 035");
    CHECK(uap[16] == "171", "UAP slot 17 = 171");
    CHECK(uap[17] == "110", "UAP slot 18 = 110");
    CHECK(uap[18] == "-",   "UAP slot 19 = - (unused)");
    CHECK(uap[19] == "RE",  "UAP slot 20 = RE");
    CHECK(uap[20] == "SP",  "UAP slot 21 = SP");

    // Item types
    for (auto id : {"000","010","020","030","035","040","045","074","075","076"}) {
        CHECK(cat.items.at(id).type == ItemType::Fixed,
              std::string(id) + " is Fixed");
    }
    CHECK(cat.items.at("060").type == ItemType::Extended,       "060 is Extended");
    CHECK(cat.items.at("015").type == ItemType::RepetitiveGroup,"015 is RepetitiveGroup");
    CHECK(cat.items.at("110").type == ItemType::RepetitiveGroup,"110 is RepetitiveGroup");
    CHECK(cat.items.at("070").type == ItemType::Compound,       "070 is Compound");
    CHECK(cat.items.at("100").type == ItemType::Compound,       "100 is Compound");
    CHECK(cat.items.at("120").type == ItemType::Compound,       "120 is Compound");
    CHECK(cat.items.at("170").type == ItemType::Compound,       "170 is Compound");
    CHECK(cat.items.at("171").type == ItemType::Compound,       "171 is Compound");
    CHECK(cat.items.at("RE").type  == ItemType::SP,             "RE is SP/Explicit");

    // Fixed byte sizes
    CHECK(cat.items.at("000").fixed_bytes == 1,  "000 = 1 byte");
    CHECK(cat.items.at("010").fixed_bytes == 2,  "010 = 2 bytes");
    CHECK(cat.items.at("020").fixed_bytes == 3,  "020 = 3 bytes");
    CHECK(cat.items.at("030").fixed_bytes == 2,  "030 = 2 bytes");
    CHECK(cat.items.at("035").fixed_bytes == 2,  "035 = 2 bytes");
    CHECK(cat.items.at("040").fixed_bytes == 2,  "040 = 2 bytes");
    CHECK(cat.items.at("045").fixed_bytes == 1,  "045 = 1 byte");
    CHECK(cat.items.at("074").fixed_bytes == 2,  "074 = 2 bytes");
    CHECK(cat.items.at("075").fixed_bytes == 3,  "075 = 3 bytes");
    CHECK(cat.items.at("076").fixed_bytes == 2,  "076 = 2 bytes");

    // I060 Extended octets
    CHECK(cat.items.at("060").octets.size() == 8, "060 has 8 octets");

    // I015 RepetitiveGroup: SAC(8)+SIC(8) = 16 bits/entry
    CHECK(cat.items.at("015").rep_group_bits == 16,              "015 rep_group_bits = 16");
    CHECK(cat.items.at("015").rep_group_elements.size() == 2,    "015 has 2 elements");
    // I110 RepetitiveGroup: CEN(8)+POS(8) = 16 bits/entry
    CHECK(cat.items.at("110").rep_group_bits == 16,              "110 rep_group_bits = 16");
    CHECK(cat.items.at("110").rep_group_elements.size() == 2,    "110 has 2 elements");

    // I070 Compound sub-items
    const auto& i070 = cat.items.at("070");
    CHECK(i070.compound_sub_items.size() == 6, "070 has 6 sub-items");
    CHECK(i070.compound_sub_items[0].name == "TC",  "070 sub[0] = TC");
    CHECK(i070.compound_sub_items[1].name == "TCA", "070 sub[1] = TCA");
    CHECK(i070.compound_sub_items[2].name == "CHS", "070 sub[2] = CHS");
    CHECK(i070.compound_sub_items[3].name == "MHS", "070 sub[3] = MHS");
    CHECK(i070.compound_sub_items[4].name == "CVS", "070 sub[4] = CVS");
    CHECK(i070.compound_sub_items[5].name == "MVS", "070 sub[5] = MVS");

    // I120 Compound sub-items
    const auto& i120 = cat.items.at("120");
    CHECK(i120.compound_sub_items.size() == 4, "120 has 4 sub-items");
    CHECK(i120.compound_sub_items[0].name == "CN", "120 sub[0] = CN");
    CHECK(i120.compound_sub_items[1].name == "CC", "120 sub[1] = CC");
    CHECK(i120.compound_sub_items[2].name == "CP", "120 sub[2] = CP");
    CHECK(i120.compound_sub_items[3].name == "CD", "120 sub[3] = CD");
    CHECK(i120.compound_sub_items[0].fixed_bytes == 3, "120/CN = 3 bytes");
    CHECK(i120.compound_sub_items[1].fixed_bytes == 1, "120/CC = 1 byte");

    // I170/I171 Compound sub-items
    const auto& i170 = cat.items.at("170");
    CHECK(i170.compound_sub_items.size() == 10, "170 has 10 sub-items");
    CHECK(i170.compound_sub_items[0].name == "AI1", "170 sub[0] = AI1");
    CHECK(i170.compound_sub_items[2].name == "CPW", "170 sub[2] = CPW");
    CHECK(i170.compound_sub_items[6].name == "AC1", "170 sub[6] = AC1");
    CHECK(i170.compound_sub_items[2].fixed_bytes == 10, "170/CPW = 10 bytes");
    CHECK(i170.compound_sub_items[3].fixed_bytes == 8,  "170/CPC = 8 bytes");
    CHECK(i170.compound_sub_items[6].fixed_bytes == 2,  "170/AC1 = 2 bytes (flattened)");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode a basic Alive Message from raw bytes
//          I010(SAC=1,SIC=2) + I000(MSGTYP=1)
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicMessage(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT04 Alive Message ===\n";

    // FSPEC byte 1: I010(b7)+I000(b6) = 0xC0, FX=0
    // LEN = 3 + 1 + 2 + 1 = 7
    std::vector<uint8_t> frame = {
        0x04,             // CAT = 4
        0x00, 0x07,       // LEN = 7
        0xC0,             // FSPEC: I010+I000
        0x01, 0x02,       // I010 SAC=1, SIC=2
        0x01              // I000 MSGTYP=1 (Alive Message)
    };

    hexdump(frame, "Basic Alive Message input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid, "block is valid");
    CHECK(blk.records.size() == 1, "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"),  "I010 present");
    CHECK(rec.items.count("000"),  "I000 present");
    CHECK(!rec.items.count("020"), "I020 absent");
    CHECK(!rec.items.count("060"), "I060 absent");

    CHECK(rec.items.at("010").fields.at("SAC")    == 1, "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC")    == 2, "I010.SIC=2");
    CHECK(rec.items.at("000").fields.at("MSGTYP") == 1, "I000.MSGTYP=1 (Alive Message)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip simple Fixed items
//          I010 + I000 + I020 + I030 + I035 + I040 + I045
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripSimpleFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip simple Fixed items ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=3; it.fields["SIC"]=7;    enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=7;                     enc_rec.items["000"]=it; } // STCA
    { DecodedItem it; it.fields["TOD"]=0x600000;                 enc_rec.items["020"]=it; }
    { DecodedItem it; it.fields["TN1"]=0x1234;                   enc_rec.items["030"]=it; }
    { DecodedItem it; it.fields["TN2"]=0x5678;                   enc_rec.items["035"]=it; }
    { DecodedItem it; it.fields["AID"]=42;                       enc_rec.items["040"]=it; }
    { DecodedItem it;
      it.fields["EP"]=1; it.fields["VAL"]=1; it.fields["STAT"]=3;
      enc_rec.items["045"]=it; }

    std::vector<uint8_t> encoded = codec.encode(4, {enc_rec});
    hexdump(encoded, "Simple Fixed encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("SAC")    == 3,        "I010.SAC=3");
    CHECK(items.at("010").fields.at("SIC")    == 7,        "I010.SIC=7");
    CHECK(items.at("000").fields.at("MSGTYP") == 7,        "I000.MSGTYP=7 (STCA)");
    CHECK(items.at("020").fields.at("TOD")    == 0x600000, "I020.TOD=0x600000");
    CHECK(items.at("030").fields.at("TN1")    == 0x1234,   "I030.TN1=0x1234");
    CHECK(items.at("035").fields.at("TN2")    == 0x5678,   "I035.TN2=0x5678");
    CHECK(items.at("040").fields.at("AID")    == 42,       "I040.AID=42");
    CHECK(items.at("045").fields.at("EP")     == 1,        "I045.EP=1");
    CHECK(items.at("045").fields.at("VAL")    == 1,        "I045.VAL=1 (Active)");
    CHECK(items.at("045").fields.at("STAT")   == 3,        "I045.STAT=3");

    // Boundary: AID max 16-bit
    {
        DecodedRecord r; r.uap_variation = "default";
        { DecodedItem it; it.fields["SAC"]=0; it.fields["SIC"]=0; r.items["010"]=it; }
        { DecodedItem it; it.fields["AID"]=0xFFFF;                 r.items["040"]=it; }
        auto enc = codec.encode(4, {r});
        auto b   = codec.decode(enc);
        if (!b.records.empty())
            CHECK(b.records[0].items.at("040").fields.at("AID") == 0xFFFF,
                  "I040.AID=0xFFFF (max)");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip I060 Extended (Safety Net Function Status)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtendedI060(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I060 Extended ===\n";

    // 4a: Alive Message with only octet 1 (STCA+MSAW active)
    {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; enc_rec.items["010"]=it; }
        { DecodedItem it; it.fields["MSGTYP"]=1;                   enc_rec.items["000"]=it; }
        DecodedItem i060;
        i060.fields["MRVA"]=0; i060.fields["RAMLD"]=0; i060.fields["RAMHD"]=0;
        i060.fields["MSAW"]=1; i060.fields["APW"]=0;  i060.fields["CLAM"]=0;
        i060.fields["STCA"]=1;
        enc_rec.items["060"] = i060;

        auto encoded = codec.encode(4, {enc_rec});
        hexdump(encoded, "I060 octet-1 only encoded");

        auto blk = codec.decode(encoded);
        CHECK(blk.valid, "I060-a: block valid");
        if (!blk.records.empty()) {
            const auto& f = blk.records[0].items.at("060").fields;
            CHECK(f.at("MSAW") == 1, "I060.MSAW=1");
            CHECK(f.at("STCA") == 1, "I060.STCA=1");
            CHECK(f.at("RAMLD") == 0, "I060.RAMLD=0");
        }
    }

    // 4b: Full status with all 8 octets present (all functions active)
    {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        { DecodedItem it; it.fields["SAC"]=2; it.fields["SIC"]=3; enc_rec.items["010"]=it; }
        { DecodedItem it; it.fields["MSGTYP"]=1;                   enc_rec.items["000"]=it; }
        DecodedItem i060;
        // Octet 1
        i060.fields["MRVA"]=1; i060.fields["RAMLD"]=1; i060.fields["RAMHD"]=1;
        i060.fields["MSAW"]=1; i060.fields["APW"]=1;  i060.fields["CLAM"]=1;
        i060.fields["STCA"]=1;
        // Octet 2
        i060.fields["APM"]=1; i060.fields["RIMCA"]=1; i060.fields["ACASRA"]=1;
        i060.fields["NTCA"]=1; i060.fields["DG"]=0; i060.fields["OF"]=0; i060.fields["OL"]=0;
        // Octet 3
        i060.fields["AIW"]=1; i060.fields["PAIW"]=0; i060.fields["OCAT"]=1;
        i060.fields["SAM"]=1; i060.fields["VCD"]=1; i060.fields["CHAM"]=1; i060.fields["DSAM"]=1;
        // Octet 4
        i060.fields["DBPSMARR"]=1; i060.fields["DBPSMDEP"]=1; i060.fields["DBPSMTL"]=1;
        i060.fields["VRAMCRM"]=1; i060.fields["VRAMVTM"]=1; i060.fields["VRAMVRM"]=1;
        i060.fields["HAMHD"]=1;
        // Octet 5
        i060.fields["HAMRD"]=1; i060.fields["HAMVD"]=1; i060.fields["HVI"]=1;
        i060.fields["LTW"]=1; i060.fields["VPM"]=1; i060.fields["TTA"]=1; i060.fields["CRA"]=1;
        // Octet 6
        i060.fields["ASM"]=1; i060.fields["IAVM"]=1; i060.fields["FTD"]=1;
        i060.fields["ITD"]=1; i060.fields["IIA"]=1; i060.fields["SQW"]=1; i060.fields["CUW"]=1;
        // Octet 7
        i060.fields["CATC"]=1; i060.fields["NOCLR"]=1; i060.fields["NOMOV"]=1;
        i060.fields["NOH"]=1; i060.fields["WRTY"]=1; i060.fields["STOCC"]=1;
        i060.fields["ONGOING"]=1;
        // Octet 8
        i060.fields["NTZ"]=1;
        enc_rec.items["060"] = i060;

        auto encoded = codec.encode(4, {enc_rec});
        hexdump(encoded, "I060 all-8-octets encoded");

        auto blk = codec.decode(encoded);
        CHECK(blk.valid, "I060-b: block valid");
        if (!blk.records.empty()) {
            const auto& f = blk.records[0].items.at("060").fields;
            CHECK(f.at("MRVA")  == 1, "I060.MRVA=1");
            CHECK(f.at("STCA")  == 1, "I060.STCA=1");
            CHECK(f.at("APM")   == 1, "I060.APM=1");
            CHECK(f.at("RIMCA") == 1, "I060.RIMCA=1");
            CHECK(f.at("AIW")   == 1, "I060.AIW=1");
            CHECK(f.at("HAMRD") == 1, "I060.HAMRD=1");
            CHECK(f.at("ASM")   == 1, "I060.ASM=1");
            CHECK(f.at("CATC")  == 1, "I060.CATC=1");
            CHECK(f.at("NTZ")   == 1, "I060.NTZ=1");
            CHECK(f.at("DG")    == 0, "I060.DG=0 (not degraded)");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip I015 RepetitiveGroup (SDPS Identifiers)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripI015(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I015 SDPS Identifiers ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    { DecodedItem it; it.fields["SAC"]=5; it.fields["SIC"]=10; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=7;                    enc_rec.items["000"]=it; }

    DecodedItem i015;
    std::map<std::string, uint64_t> s0; s0["SAC"]=1; s0["SIC"]=11;
    std::map<std::string, uint64_t> s1; s1["SAC"]=2; s1["SIC"]=22;
    std::map<std::string, uint64_t> s2; s2["SAC"]=3; s2["SIC"]=33;
    i015.group_repetitions.push_back(s0);
    i015.group_repetitions.push_back(s1);
    i015.group_repetitions.push_back(s2);
    enc_rec.items["015"] = i015;

    auto encoded = codec.encode(4, {enc_rec});
    hexdump(encoded, "I015 encoded");

    auto blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& reps = blk.records[0].items.at("015").group_repetitions;
    CHECK(reps.size() == 3, "I015: 3 SDPS entries");
    if (reps.size() >= 3) {
        CHECK(reps[0].at("SAC") == 1 && reps[0].at("SIC") == 11, "I015[0] SAC=1 SIC=11");
        CHECK(reps[1].at("SAC") == 2 && reps[1].at("SIC") == 22, "I015[1] SAC=2 SIC=22");
        CHECK(reps[2].at("SAC") == 3 && reps[2].at("SIC") == 33, "I015[2] SAC=3 SIC=33");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip I070 Compound (Conflict Timing and Separation)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCompoundI070(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I070 Conflict Timing and Separation ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=7;                   enc_rec.items["000"]=it; }
    { DecodedItem it; it.fields["AID"]=99;                     enc_rec.items["040"]=it; }

    DecodedItem i070;
    i070.compound_sub_fields["TC"]["TC"]   = 0x000080;   // ~1 s at 1/128 LSB
    i070.compound_sub_fields["TCA"]["TCA"] = 0x000100;   // ~2 s
    i070.compound_sub_fields["CHS"]["CHS"] = 0x000400;   // 512 m (0x400 × 0.5)
    i070.compound_sub_fields["MHS"]["MHS"] = 0x0080;     // 64 m
    i070.compound_sub_fields["CVS"]["CVS"] = 0x0004;     // 100 ft (4 × 25)
    i070.compound_sub_fields["MVS"]["MVS"] = 0x0002;     // 50 ft
    enc_rec.items["070"] = i070;

    auto encoded = codec.encode(4, {enc_rec});
    hexdump(encoded, "I070 encoded");

    auto blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& csf = blk.records[0].items.at("070").compound_sub_fields;
    CHECK(csf.at("TC").at("TC")   == 0x000080, "I070.TC=0x000080");
    CHECK(csf.at("TCA").at("TCA") == 0x000100, "I070.TCA=0x000100");
    CHECK(csf.at("CHS").at("CHS") == 0x000400, "I070.CHS=0x000400");
    CHECK(csf.at("MHS").at("MHS") == 0x0080,   "I070.MHS=0x0080");
    CHECK(csf.at("CVS").at("CVS") == 0x0004,   "I070.CVS=0x0004");
    CHECK(csf.at("MVS").at("MVS") == 0x0002,   "I070.MVS=0x0002");

    // Test with only TC and CHS present
    {
        DecodedRecord r; r.uap_variation = "default";
        { DecodedItem it; it.fields["SAC"]=0; it.fields["SIC"]=0; r.items["010"]=it; }
        DecodedItem i070b;
        i070b.compound_sub_fields["TC"]["TC"]   = 0x000200;
        i070b.compound_sub_fields["CHS"]["CHS"] = 0x001000;
        r.items["070"] = i070b;
        auto enc = codec.encode(4, {r});
        auto b   = codec.decode(enc);
        if (!b.records.empty()) {
            const auto& c = b.records[0].items.at("070").compound_sub_fields;
            CHECK(c.at("TC").at("TC")   == 0x000200, "I070(partial).TC=0x000200");
            CHECK(c.at("CHS").at("CHS") == 0x001000, "I070(partial).CHS=0x001000");
            CHECK(!c.count("TCA"), "I070(partial): TCA absent");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip signed deviations (I074, I075, I076)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripSignedDeviations(Codec& codec) {
    std::cout << "\n=== Test: Round-trip signed deviation items ===\n";

    struct TestCase { int16_t ld; int32_t tdd; int16_t vd; };
    std::vector<TestCase> cases = {
        {100,  2000,  10},    // positive deviations
        {-50, -1000, -8},     // negative deviations
        {0,   0,      0},     // zero
        {32767, 8388607, 32767},  // max positive
    };

    for (const auto& tc : cases) {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        { DecodedItem it; it.fields["SAC"]=0; it.fields["SIC"]=0; enc_rec.items["010"]=it; }
        { DecodedItem it; it.fields["LD"]  = s16(tc.ld);  enc_rec.items["074"]=it; }
        { DecodedItem it; it.fields["TDD"] = s24(tc.tdd); enc_rec.items["075"]=it; }
        { DecodedItem it; it.fields["VD"]  = s16(tc.vd);  enc_rec.items["076"]=it; }

        auto encoded = codec.encode(4, {enc_rec});
        auto blk     = codec.decode(encoded);

        if (!blk.records.empty()) {
            const auto& items = blk.records[0].items;
            CHECK(items.at("074").fields.at("LD")  == s16(tc.ld),
                  "I074.LD=" + std::to_string(tc.ld));
            CHECK(items.at("075").fields.at("TDD") == s24(tc.tdd),
                  "I075.TDD=" + std::to_string(tc.tdd));
            CHECK(items.at("076").fields.at("VD")  == s16(tc.vd),
                  "I076.VD=" + std::to_string(tc.vd));
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Round-trip I120 Compound (Conflict Characteristics)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCompoundI120(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I120 Conflict Characteristics ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=7;                   enc_rec.items["000"]=it; }
    { DecodedItem it; it.fields["AID"]=55;                     enc_rec.items["040"]=it; }

    DecodedItem i120;
    // CN: set TYPE=1 (major), CROSS=1
    i120.compound_sub_fields["CN"]["MAS"]   = 0;
    i120.compound_sub_fields["CN"]["CAS"]   = 1;
    i120.compound_sub_fields["CN"]["FLD"]   = 0;
    i120.compound_sub_fields["CN"]["FVD"]   = 0;
    i120.compound_sub_fields["CN"]["TYPE"]  = 1;
    i120.compound_sub_fields["CN"]["CROSS"] = 1;
    i120.compound_sub_fields["CN"]["DIV"]   = 0;
    i120.compound_sub_fields["CN"]["RRC"]   = 0;
    i120.compound_sub_fields["CN"]["RTC"]   = 0;
    i120.compound_sub_fields["CN"]["CN_MRVA"]    = 0;
    i120.compound_sub_fields["CN"]["CN_VRAMCRM"] = 0;
    i120.compound_sub_fields["CN"]["CN_VRAMVRM"] = 0;
    i120.compound_sub_fields["CN"]["CN_VRAMVTM"] = 0;
    i120.compound_sub_fields["CN"]["CN_HAMHD"]   = 0;
    i120.compound_sub_fields["CN"]["CN_HAMRD"]   = 0;
    i120.compound_sub_fields["CN"]["CN_HAMVD"]   = 0;
    i120.compound_sub_fields["CN"]["CN_DBPSMARR"]= 0;
    i120.compound_sub_fields["CN"]["CN_DBPSMDEP"]= 0;
    i120.compound_sub_fields["CN"]["CN_DBPSMTL"] = 0;
    i120.compound_sub_fields["CN"]["CN_AIW"]     = 0;
    // CC: TID=7, CPC=1, CS=1 (HIGH)
    i120.compound_sub_fields["CC"]["TID"] = 7;
    i120.compound_sub_fields["CC"]["CPC"] = 1;
    i120.compound_sub_fields["CC"]["CS"]  = 1;
    // CP: 50% probability
    i120.compound_sub_fields["CP"]["CP"] = 100;  // 100 × 0.5 = 50%
    // CD: duration
    i120.compound_sub_fields["CD"]["CD"] = 0x000800;
    enc_rec.items["120"] = i120;

    auto encoded = codec.encode(4, {enc_rec});
    hexdump(encoded, "I120 encoded");

    auto blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& csf = blk.records[0].items.at("120").compound_sub_fields;
    CHECK(csf.at("CN").at("CAS")   == 1, "I120.CN.CAS=1 (civil airspace)");
    CHECK(csf.at("CN").at("TYPE")  == 1, "I120.CN.TYPE=1 (major)");
    CHECK(csf.at("CN").at("CROSS") == 1, "I120.CN.CROSS=1");
    CHECK(csf.at("CC").at("TID")   == 7, "I120.CC.TID=7");
    CHECK(csf.at("CC").at("CPC")   == 1, "I120.CC.CPC=1");
    CHECK(csf.at("CC").at("CS")    == 1, "I120.CC.CS=1 (HIGH)");
    CHECK(csf.at("CP").at("CP")    == 100, "I120.CP=100 (50%)");
    CHECK(csf.at("CD").at("CD")    == 0x000800, "I120.CD=0x000800");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 9: Round-trip I170 Compound (Aircraft Identification 1)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCompoundI170(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I170 Aircraft Identification 1 ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=7;                   enc_rec.items["000"]=it; }
    { DecodedItem it; it.fields["AID"]=7;                      enc_rec.items["040"]=it; }

    DecodedItem i170;
    // AI1: 7-char ASCII "AFR123 " packed as uint64_t (raw)
    uint64_t ai1 = 0;
    const char* callsign = "AFR123 ";
    for (int i = 0; i < 7; ++i)
        ai1 = (ai1 << 8) | (uint8_t)callsign[i];
    i170.compound_sub_fields["AI1"]["AI1"] = ai1;

    // CPW: WGS-84 position
    i170.compound_sub_fields["CPW"]["LAT"] = s32(100000);   // ~0.537°
    i170.compound_sub_fields["CPW"]["LON"] = s32(200000);   // ~1.073°
    i170.compound_sub_fields["CPW"]["ALT"] = s16(400);      // 400 × 25 = 10000 ft

    // AC1: GAT, IFR, RVSM approved, Normal, Maintaining, Non-primary, Not GV
    i170.compound_sub_fields["AC1"]["GATOAT"] = 1; // GAT
    i170.compound_sub_fields["AC1"]["FR1FR2"] = 0; // IFR
    i170.compound_sub_fields["AC1"]["RVSM"]   = 1; // Approved
    i170.compound_sub_fields["AC1"]["HPR"]    = 0; // Normal
    i170.compound_sub_fields["AC1"]["CDM"]    = 0; // Maintaining
    i170.compound_sub_fields["AC1"]["PRI"]    = 0; // Non-primary
    i170.compound_sub_fields["AC1"]["GV"]     = 0; // Not GV

    // CF1: cleared FL200 → 200 / 0.25 = 800
    i170.compound_sub_fields["CF1"]["CF1"] = 800;
    enc_rec.items["170"] = i170;

    auto encoded = codec.encode(4, {enc_rec});
    hexdump(encoded, "I170 encoded");

    auto blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& csf = blk.records[0].items.at("170").compound_sub_fields;
    CHECK(csf.at("AI1").at("AI1") == ai1,        "I170.AI1 matches");
    CHECK(csf.at("CPW").at("LAT") == s32(100000), "I170.CPW.LAT=100000");
    CHECK(csf.at("CPW").at("LON") == s32(200000), "I170.CPW.LON=200000");
    CHECK(csf.at("CPW").at("ALT") == s16(400),    "I170.CPW.ALT=400");
    CHECK(csf.at("AC1").at("GATOAT") == 1, "I170.AC1.GATOAT=1 (GAT)");
    CHECK(csf.at("AC1").at("FR1FR2") == 0, "I170.AC1.FR1FR2=0 (IFR)");
    CHECK(csf.at("AC1").at("RVSM")   == 1, "I170.AC1.RVSM=1 (Approved)");
    CHECK(csf.at("AC1").at("CDM")    == 0, "I170.AC1.CDM=0 (Maintaining)");
    CHECK(csf.at("CF1").at("CF1")    == 800, "I170.CF1=800 (FL200)");

    // Test negative altitude (below MSL)
    {
        DecodedRecord r; r.uap_variation = "default";
        { DecodedItem it; it.fields["SAC"]=0; it.fields["SIC"]=0; r.items["010"]=it; }
        DecodedItem i170b;
        i170b.compound_sub_fields["CPW"]["LAT"] = s32(-50000);
        i170b.compound_sub_fields["CPW"]["LON"] = s32(-100000);
        i170b.compound_sub_fields["CPW"]["ALT"] = s16(-60);  // -60 × 25 = -1500 ft
        r.items["170"] = i170b;
        auto enc = codec.encode(4, {r});
        auto b   = codec.decode(enc);
        if (!b.records.empty()) {
            const auto& c = b.records[0].items.at("170").compound_sub_fields;
            CHECK(c.at("CPW").at("LAT") == s32(-50000), "I170.CPW.LAT=-50000 (negative)");
            CHECK(c.at("CPW").at("ALT") == s16(-60),    "I170.CPW.ALT=-60 (below MSL)");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 10: Full round-trip with major items
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip CAT04 STCA message ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=10; it.fields["SIC"]=20; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=7;                     enc_rec.items["000"]=it; } // STCA
    { DecodedItem it; it.fields["TOD"]=0x800000;                 enc_rec.items["020"]=it; }
    { DecodedItem it; it.fields["TN1"]=0x0100;                   enc_rec.items["030"]=it; }
    { DecodedItem it; it.fields["TN2"]=0x0200;                   enc_rec.items["035"]=it; }
    { DecodedItem it; it.fields["AID"]=1001;                     enc_rec.items["040"]=it; }
    { DecodedItem it; it.fields["EP"]=1; it.fields["VAL"]=1; it.fields["STAT"]=0;
                      enc_rec.items["045"]=it; }

    // I070: TC=128 (1 s), CHS=2000 (1000 m), CVS=8 (200 ft)
    {
        DecodedItem it;
        it.compound_sub_fields["TC"]["TC"]   = 128;
        it.compound_sub_fields["CHS"]["CHS"] = 2000;
        it.compound_sub_fields["CVS"]["CVS"] = 8;
        enc_rec.items["070"] = it;
    }

    // I110: 2 sector positions
    {
        DecodedItem it;
        std::map<std::string, uint64_t> p0; p0["CEN"]=1; p0["POS"]=5;
        std::map<std::string, uint64_t> p1; p1["CEN"]=2; p1["POS"]=3;
        it.group_repetitions.push_back(p0);
        it.group_repetitions.push_back(p1);
        enc_rec.items["110"] = it;
    }

    std::vector<uint8_t> encoded = codec.encode(4, {enc_rec});
    hexdump(encoded, "Full STCA message encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.count("010"), "I010 present");
    CHECK(items.count("000"), "I000 present");
    CHECK(items.count("020"), "I020 present");
    CHECK(items.count("030"), "I030 present");
    CHECK(items.count("035"), "I035 present");
    CHECK(items.count("040"), "I040 present");
    CHECK(items.count("045"), "I045 present");
    CHECK(items.count("070"), "I070 present");
    CHECK(items.count("110"), "I110 present");

    CHECK(items.at("010").fields.at("SAC")    == 10,       "I010.SAC=10");
    CHECK(items.at("010").fields.at("SIC")    == 20,       "I010.SIC=20");
    CHECK(items.at("000").fields.at("MSGTYP") == 7,        "I000.MSGTYP=7 (STCA)");
    CHECK(items.at("020").fields.at("TOD")    == 0x800000, "I020.TOD=0x800000");
    CHECK(items.at("030").fields.at("TN1")    == 0x0100,   "I030.TN1=0x0100");
    CHECK(items.at("035").fields.at("TN2")    == 0x0200,   "I035.TN2=0x0200");
    CHECK(items.at("040").fields.at("AID")    == 1001,     "I040.AID=1001");
    CHECK(items.at("045").fields.at("EP")     == 1,        "I045.EP=1");
    CHECK(items.at("045").fields.at("VAL")    == 1,        "I045.VAL=1 (Active)");

    const auto& csf070 = items.at("070").compound_sub_fields;
    CHECK(csf070.at("TC").at("TC")   == 128,  "I070.TC=128");
    CHECK(csf070.at("CHS").at("CHS") == 2000, "I070.CHS=2000");
    CHECK(csf070.at("CVS").at("CVS") == 8,    "I070.CVS=8");

    const auto& reps110 = items.at("110").group_repetitions;
    CHECK(reps110.size() == 2,                              "I110: 2 sectors");
    CHECK(reps110[0].at("CEN") == 1 && reps110[0].at("POS") == 5, "I110[0] CEN=1 POS=5");
    CHECK(reps110[1].at("CEN") == 2 && reps110[1].at("POS") == 3, "I110[1] CEN=2 POS=3");
}

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = "specs/CAT04.xml";
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
    testRoundTripExtendedI060(codec);
    testRoundTripI015(codec);
    testRoundTripCompoundI070(codec);
    testRoundTripSignedDeviations(codec);
    testRoundTripCompoundI120(codec);
    testRoundTripCompoundI170(codec);
    testFullRoundTrip(codec);

    std::cout << "\n============================\n";
    if (failures == 0)
        std::cout << "ALL TESTS PASSED\n";
    else
        std::cout << failures << " TEST(S) FAILED\n";
    std::cout << "============================\n";

    return failures ? 1 : 0;
}
