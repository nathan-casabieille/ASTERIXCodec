// test_cat15.cpp – Tests for CAT15 Independent Non-Cooperative Surveillance System Target Reports, Ed. 1.2.

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

static uint64_t s8(int8_t   v) { return static_cast<uint64_t>(static_cast<uint8_t>(v)); }
static uint64_t s16(int16_t v) { return static_cast<uint64_t>(static_cast<uint16_t>(v)); }
static uint64_t s24(int32_t v) { return static_cast<uint64_t>(static_cast<uint32_t>(v) & 0xFFFFFF); }
static uint64_t s32(int32_t v) { return static_cast<uint64_t>(static_cast<uint32_t>(v)); }

// ─────────────────────────────────────────────────────────────────────────────
//  Test 1: Spec load
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT15 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 15,        "cat number = 15");
    CHECK(cat.edition == "1.2", "edition = 1.2");

    for (auto id : {"000","010","015","020","030","050","145","161","170",
                    "270","300","400","480","600","601","602","603","604",
                    "605","625","626","627","628","630","631","SP"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 26, "26 items total");

    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    const auto& uap = cat.uap_variations.at("default");
    CHECK(uap.size() == 26, "UAP has 26 slots");

    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "000", "UAP slot  2 = 000");
    CHECK(uap[2]  == "015", "UAP slot  3 = 015");
    CHECK(uap[3]  == "020", "UAP slot  4 = 020");
    CHECK(uap[4]  == "030", "UAP slot  5 = 030");
    CHECK(uap[5]  == "145", "UAP slot  6 = 145");
    CHECK(uap[6]  == "161", "UAP slot  7 = 161");
    CHECK(uap[7]  == "170", "UAP slot  8 = 170");
    CHECK(uap[8]  == "050", "UAP slot  9 = 050");
    CHECK(uap[9]  == "270", "UAP slot 10 = 270");
    CHECK(uap[10] == "300", "UAP slot 11 = 300");
    CHECK(uap[11] == "400", "UAP slot 12 = 400");
    CHECK(uap[12] == "600", "UAP slot 13 = 600");
    CHECK(uap[13] == "601", "UAP slot 14 = 601");
    CHECK(uap[14] == "602", "UAP slot 15 = 602");
    CHECK(uap[15] == "603", "UAP slot 16 = 603");
    CHECK(uap[16] == "604", "UAP slot 17 = 604");
    CHECK(uap[17] == "605", "UAP slot 18 = 605");
    CHECK(uap[18] == "480", "UAP slot 19 = 480");
    CHECK(uap[19] == "625", "UAP slot 20 = 625");
    CHECK(uap[20] == "626", "UAP slot 21 = 626");
    CHECK(uap[21] == "627", "UAP slot 22 = 627");
    CHECK(uap[22] == "628", "UAP slot 23 = 628");
    CHECK(uap[23] == "630", "UAP slot 24 = 630");
    CHECK(uap[24] == "631", "UAP slot 25 = 631");
    CHECK(uap[25] == "SP",  "UAP slot 26 = SP");

    for (auto id : {"000","010","015","050","145","161","400"}) {
        CHECK(cat.items.at(id).type == ItemType::Fixed,
              std::string(id) + " is Fixed");
    }
    CHECK(cat.items.at("020").type == ItemType::Extended,        "020 is Extended");
    CHECK(cat.items.at("170").type == ItemType::Extended,        "170 is Extended");
    CHECK(cat.items.at("030").type == ItemType::Repetitive,      "030 is Repetitive(FX)");
    CHECK(cat.items.at("270").type == ItemType::Compound,        "270 is Compound");
    CHECK(cat.items.at("600").type == ItemType::Compound,        "600 is Compound");
    CHECK(cat.items.at("601").type == ItemType::Compound,        "601 is Compound");
    CHECK(cat.items.at("625").type == ItemType::Compound,        "625 is Compound");
    CHECK(cat.items.at("626").type == ItemType::Compound,        "626 is Compound");
    CHECK(cat.items.at("627").type == ItemType::Compound,        "627 is Compound");
    CHECK(cat.items.at("628").type == ItemType::Compound,        "628 is Compound");
    CHECK(cat.items.at("630").type == ItemType::Compound,        "630 is Compound");
    CHECK(cat.items.at("300").type == ItemType::RepetitiveGroup, "300 is RepetitiveGroup");
    CHECK(cat.items.at("480").type == ItemType::RepetitiveGroup, "480 is RepetitiveGroup");
    CHECK(cat.items.at("631").type == ItemType::RepetitiveGroup, "631 is RepetitiveGroup");
    CHECK(cat.items.at("SP").type  == ItemType::SP,              "SP is SP/Explicit");

    CHECK(cat.items.at("000").fixed_bytes == 1,  "000 = 1 byte");
    CHECK(cat.items.at("010").fixed_bytes == 2,  "010 = 2 bytes");
    CHECK(cat.items.at("015").fixed_bytes == 1,  "015 = 1 byte");
    CHECK(cat.items.at("050").fixed_bytes == 2,  "050 = 2 bytes");
    CHECK(cat.items.at("145").fixed_bytes == 3,  "145 = 3 bytes");
    CHECK(cat.items.at("161").fixed_bytes == 2,  "161 = 2 bytes");
    CHECK(cat.items.at("400").fixed_bytes == 5,  "400 = 5 bytes");

    CHECK(cat.items.at("020").octets.size() == 1, "020 has 1 octet");
    CHECK(cat.items.at("170").octets.size() == 1, "170 has 1 octet");

    CHECK(cat.items.at("300").rep_group_bits == 16, "300 rep_group_bits = 16");
    CHECK(cat.items.at("480").rep_group_bits == 40, "480 rep_group_bits = 40");
    CHECK(cat.items.at("631").rep_group_bits == 64, "631 rep_group_bits = 64");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode a minimal target report from raw bytes
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicTargetReport(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT15 target report ===\n";

    // UAP B1: 010(b7) 000(b6) → FSPEC 0xC0, FX=0
    // I000: MT(7)+RG(1), MT=2 RG=0 → (2<<1)|0 = 0x04
    // Total: CAT(1)+LEN(2)+FSPEC(1)+010(2)+000(1) = 7 bytes
    std::vector<uint8_t> frame = {
        0x0F,             // CAT = 15
        0x00, 0x07,       // LEN = 7
        0xC0,             // FSPEC: 010+000, FX=0
        0x01, 0x02,       // I010: SAC=1, SIC=2
        0x04              // I000: MT=2 (Measurement Track), RG=0 (Periodic)
    };

    hexdump(frame, "Basic target report input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid,                "block is valid");
    CHECK(blk.records.size() == 1,  "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"),   "I010 present");
    CHECK(rec.items.count("000"),   "I000 present");
    CHECK(!rec.items.count("145"),  "I145 absent");

    CHECK(rec.items.at("010").fields.at("SAC") == 1, "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC") == 2, "I010.SIC=2");
    CHECK(rec.items.at("000").fields.at("MT")  == 2, "I000.MT=2 (Measurement Track)");
    CHECK(rec.items.at("000").fields.at("RG")  == 0, "I000.RG=0 (Periodic)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip simple Fixed items
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip simple Fixed items ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=5; it.fields["SIC"]=10; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MT"]=2; it.fields["RG"]=0; rec.items["000"]=it; }
    { DecodedItem it; it.fields["SID"]=3; rec.items["015"]=it; }
    // I145: TOA = 43200*128 LSBs (12h)
    { DecodedItem it; it.fields["TOA"]=43200u*128u; rec.items["145"]=it; }
    // I161: TPN=0x5678
    { DecodedItem it; it.fields["TPN"]=0x5678; rec.items["161"]=it; }
    // I050: spare(2)+UPD(14), UPD=128 (1s at 1/128 s/LSB)
    { DecodedItem it; it.fields["UPD"]=128; rec.items["050"]=it; }
    // I400: PID=0xABCD, ON=0x123456
    { DecodedItem it; it.fields["PID"]=0xABCD; it.fields["ON"]=0x123456; rec.items["400"]=it; }

    auto encoded = codec.encode(15, {rec});
    hexdump(encoded, "SimpleFixed encoded");
    CHECK(!encoded.empty(), "encode produced output");
    CHECK(encoded[0] == 0x0F, "CAT byte = 0x0F (15)");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("SAC") == 5,           "010/SAC = 5");
    CHECK(items.at("010").fields.at("SIC") == 10,          "010/SIC = 10");
    CHECK(items.at("000").fields.at("MT")  == 2,           "000/MT = 2");
    CHECK(items.at("000").fields.at("RG")  == 0,           "000/RG = 0");
    CHECK(items.at("015").fields.at("SID") == 3,           "015/SID = 3");
    CHECK(items.at("145").fields.at("TOA") == 43200u*128u, "145/TOA round-trip");
    CHECK(items.at("161").fields.at("TPN") == 0x5678,      "161/TPN = 0x5678");
    CHECK(items.at("050").fields.at("UPD") == 128,         "050/UPD = 128 (1s)");
    CHECK(items.at("400").fields.at("PID") == 0xABCD,      "400/PID = 0xABCD");
    CHECK(items.at("400").fields.at("ON")  == 0x123456,    "400/ON = 0x123456");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip Extended I020 and I170
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtended(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Extended I020 and I170 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MT"]=1; it.fields["RG"]=0; rec.items["000"]=it; }

    // I020: 1 octet (7 data bits) – MOMU=1, TTAX=2, SCD=1
    { DecodedItem it;
      it.fields["MOMU"]=1; it.fields["TTAX"]=2; it.fields["SCD"]=1;
      rec.items["020"]=it; }

    // I170: 1 octet – BIZ=1, BAZ=0, TUR=0, CSTP=1, CSTH=0, CNF=1
    { DecodedItem it;
      it.fields["BIZ"]=1; it.fields["BAZ"]=0; it.fields["TUR"]=0;
      it.fields["CSTP"]=1; it.fields["CSTH"]=0; it.fields["CNF"]=1;
      rec.items["170"]=it; }

    auto encoded = codec.encode(15, {rec});
    hexdump(encoded, "Extended I020+I170 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;

    CHECK(items.count("020"), "I020 present");
    const auto& f020 = items.at("020").fields;
    CHECK(f020.at("MOMU") == 1, "020/MOMU = 1 (Multi-Static)");
    CHECK(f020.at("TTAX") == 2, "020/TTAX = 2 (Synthetic)");
    CHECK(f020.at("SCD")  == 1, "020/SCD = 1 (Forward)");

    CHECK(items.count("170"), "I170 present");
    const auto& f170 = items.at("170").fields;
    CHECK(f170.at("BIZ")  == 1, "170/BIZ = 1 (Blind Zone)");
    CHECK(f170.at("BAZ")  == 0, "170/BAZ = 0");
    CHECK(f170.at("TUR")  == 0, "170/TUR = 0 (Alive)");
    CHECK(f170.at("CSTP") == 1, "170/CSTP = 1 (Extrapolated)");
    CHECK(f170.at("CSTH") == 0, "170/CSTH = 0");
    CHECK(f170.at("CNF")  == 1, "170/CNF = 1 (Tentative)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip Repetitive FX I030
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripRepetitiveFX(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Repetitive FX I030 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MT"]=1; it.fields["RG"]=0; rec.items["000"]=it; }

    // I030: two 7-bit warning codes
    DecodedItem i030;
    i030.repetitions = {0x10, 0x25};
    rec.items["030"] = i030;

    auto encoded = codec.encode(15, {rec});
    hexdump(encoded, "Repetitive FX I030 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.count("030"), "I030 present");
    const auto& reps = items.at("030").repetitions;
    CHECK(reps.size() == 2,  "030 has 2 repetitions");
    CHECK(reps[0] == 0x10,   "030 rep[0] = 0x10");
    CHECK(reps[1] == 0x25,   "030 rep[1] = 0x25");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip Compound I270 (Target Size & Orientation, 1 PSF byte)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCompoundI270(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Compound I270 (Target Size & Orientation) ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MT"]=1; it.fields["RG"]=0; rec.items["000"]=it; }

    // I270: LEN=5000 (50m), WDT=2000 (20m), HGT=1000 (10m), ORT=8192 (~45°)
    DecodedItem i270;
    i270.compound_sub_fields["LEN"]["LEN"] = 5000;
    i270.compound_sub_fields["WDT"]["WDT"] = 2000;
    i270.compound_sub_fields["HGT"]["HGT"] = 1000;
    i270.compound_sub_fields["ORT"]["ORT"] = 8192;
    rec.items["270"] = i270;

    auto encoded = codec.encode(15, {rec});
    hexdump(encoded, "Compound I270 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& sf = blk.records[0].items.at("270").compound_sub_fields;
    CHECK(sf.count("LEN"),                "270/LEN present");
    CHECK(sf.count("WDT"),                "270/WDT present");
    CHECK(sf.count("HGT"),                "270/HGT present");
    CHECK(sf.count("ORT"),                "270/ORT present");
    CHECK(sf.at("LEN").at("LEN") == 5000, "270/LEN = 5000 (50m)");
    CHECK(sf.at("WDT").at("WDT") == 2000, "270/WDT = 2000 (20m)");
    CHECK(sf.at("HGT").at("HGT") == 1000, "270/HGT = 1000 (10m)");
    CHECK(sf.at("ORT").at("ORT") == 8192, "270/ORT = 8192 (~45°)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip RepetitiveGroup I300, I480, I631
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripRepGroups(Codec& codec) {
    std::cout << "\n=== Test: Round-trip RepetitiveGroup I300, I480, I631 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MT"]=1; it.fields["RG"]=0; rec.items["000"]=it; }

    // I300: 2 object classifications
    DecodedItem i300;
    i300.group_repetitions.push_back({{"CLS", 5},  {"PRB", 90}});
    i300.group_repetitions.push_back({{"CLS", 12}, {"PRB", 50}});
    rec.items["300"] = i300;

    // I480: 1 association (40-bit raw)
    DecodedItem i480;
    i480.group_repetitions.push_back({{"ASSOC", 0x1234567890ULL}});
    rec.items["480"] = i480;

    // I631: 2 contour points (AZCON 16u, ELCON 16s, RGCONSTOP 16u, RGCONSTART 16u)
    DecodedItem i631;
    i631.group_repetitions.push_back({
        {"AZCON",      32768},
        {"ELCON",      s16(-3600)},
        {"RGCONSTOP",  1000},
        {"RGCONSTART", 500}
    });
    i631.group_repetitions.push_back({
        {"AZCON",      0},
        {"ELCON",      0},
        {"RGCONSTOP",  2000},
        {"RGCONSTART", 800}
    });
    rec.items["631"] = i631;

    auto encoded = codec.encode(15, {rec});
    hexdump(encoded, "RepGroups encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;

    CHECK(items.count("300"), "I300 present");
    const auto& gr300 = items.at("300").group_repetitions;
    CHECK(gr300.size() == 2,           "300 has 2 entries");
    CHECK(gr300[0].at("CLS") == 5,     "300 rep[0] CLS=5");
    CHECK(gr300[0].at("PRB") == 90,    "300 rep[0] PRB=90");
    CHECK(gr300[1].at("CLS") == 12,    "300 rep[1] CLS=12");
    CHECK(gr300[1].at("PRB") == 50,    "300 rep[1] PRB=50");

    CHECK(items.count("480"), "I480 present");
    const auto& gr480 = items.at("480").group_repetitions;
    CHECK(gr480.size() == 1,                              "480 has 1 entry");
    CHECK(gr480[0].at("ASSOC") == 0x1234567890ULL,        "480 rep[0] ASSOC round-trip");

    CHECK(items.count("631"), "I631 present");
    const auto& gr631 = items.at("631").group_repetitions;
    CHECK(gr631.size() == 2,                              "631 has 2 points");
    CHECK(gr631[0].at("AZCON")      == 32768,             "631 rep[0] AZCON=32768");
    CHECK(gr631[0].at("ELCON")      == s16(-3600),        "631 rep[0] ELCON=-3600");
    CHECK(gr631[0].at("RGCONSTOP")  == 1000,              "631 rep[0] RGCONSTOP=1000");
    CHECK(gr631[0].at("RGCONSTART") == 500,               "631 rep[0] RGCONSTART=500");
    CHECK(gr631[1].at("AZCON")      == 0,                 "631 rep[1] AZCON=0");
    CHECK(gr631[1].at("RGCONSTOP")  == 2000,              "631 rep[1] RGCONSTOP=2000");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Round-trip Compound I600 (Horizontal Position, 1 PSF byte)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCompoundI600(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Compound I600 (Horizontal Position) ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=2; it.fields["SIC"]=3; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MT"]=2; it.fields["RG"]=0; rec.items["000"]=it; }
    { DecodedItem it; it.fields["TPN"]=100; rec.items["161"]=it; }

    // I600: P84 (8B) + HPR (5B) + HPP (5B)
    DecodedItem i600;
    i600.compound_sub_fields["P84"]["LATITUDE"]  = s32(580000000);
    i600.compound_sub_fields["P84"]["LONGITUDE"] = s32(27000000);
    i600.compound_sub_fields["HPR"]["RSHPX"]    = 200;
    i600.compound_sub_fields["HPR"]["RSHPY"]    = 200;
    i600.compound_sub_fields["HPR"]["CORSHPXY"] = s8(10);
    i600.compound_sub_fields["HPP"]["SDHPX"]    = 100;
    i600.compound_sub_fields["HPP"]["SDHPY"]    = 100;
    i600.compound_sub_fields["HPP"]["COSDHPXY"] = s8(-5);
    rec.items["600"] = i600;

    auto encoded = codec.encode(15, {rec});
    hexdump(encoded, "Compound I600 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& sf = blk.records[0].items.at("600").compound_sub_fields;
    CHECK(sf.count("P84"),                                  "600/P84 present");
    CHECK(sf.count("HPR"),                                  "600/HPR present");
    CHECK(sf.count("HPP"),                                  "600/HPP present");
    CHECK(sf.at("P84").at("LATITUDE")  == s32(580000000),  "600/P84/LATITUDE round-trip");
    CHECK(sf.at("P84").at("LONGITUDE") == s32(27000000),   "600/P84/LONGITUDE round-trip");
    CHECK(sf.at("HPR").at("RSHPX")    == 200,              "600/HPR/RSHPX = 200");
    CHECK(sf.at("HPR").at("RSHPY")    == 200,              "600/HPR/RSHPY = 200");
    CHECK(sf.at("HPR").at("CORSHPXY") == s8(10),           "600/HPR/CORSHPXY = 10");
    CHECK(sf.at("HPP").at("SDHPX")    == 100,              "600/HPP/SDHPX = 100");
    CHECK(sf.at("HPP").at("SDHPY")    == 100,              "600/HPP/SDHPY = 100");
    CHECK(sf.at("HPP").at("COSDHPXY") == s8(-5),           "600/HPP/COSDHPXY = -5");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 9: Round-trip Compound I625 + I626 (Range and Doppler, 2 PSF bytes)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCompoundI625andI626(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Compound I625 and I626 (2 PSF bytes) ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MT"]=1; it.fields["RG"]=0; rec.items["000"]=it; }

    // I625: R(slot0) + SDR(slot2) + RR(slot3) + SDRR(slot5) + RA(slot6)
    DecodedItem i625;
    i625.compound_sub_fields["R"]["R"]        = s24(50000);   // 5000m
    i625.compound_sub_fields["SDR"]["SDR"]    = 100;          // 10m precision
    i625.compound_sub_fields["RR"]["RR"]      = s24(-200);    // -20m/s range rate
    i625.compound_sub_fields["SDRR"]["SDRR"]  = 50;           // 5m/s
    i625.compound_sub_fields["SDRR"]["CORRR"] = s8(-32);      // -0.25 correlation
    i625.compound_sub_fields["RA"]["RA"]      = s16(-128);    // -2m/s² range accel
    rec.items["625"] = i625;

    // I626: DV(slot0) + SDDV(slot1) + CODVR(slot4)
    DecodedItem i626;
    i626.compound_sub_fields["DV"]["DV"]        = s24(-1500); // -15m/s Doppler
    i626.compound_sub_fields["SDDV"]["SDDV"]    = 200;
    i626.compound_sub_fields["CODVR"]["CODVR"]  = s8(-64);    // -0.5 correlation
    rec.items["626"] = i626;

    auto encoded = codec.encode(15, {rec});
    hexdump(encoded, "Compound I625+I626 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;

    CHECK(items.count("625"),                               "I625 present");
    const auto& sf625 = items.at("625").compound_sub_fields;
    CHECK(sf625.count("R"),                                 "625/R present");
    CHECK(sf625.count("SDR"),                               "625/SDR present");
    CHECK(sf625.count("RR"),                                "625/RR present");
    CHECK(sf625.count("SDRR"),                              "625/SDRR present");
    CHECK(sf625.count("RA"),                                "625/RA present");
    CHECK(!sf625.count("RSR"),                              "625/RSR absent");
    CHECK(!sf625.count("RSRR"),                             "625/RSRR absent");
    CHECK(sf625.at("R").at("R")        == s24(50000),       "625/R = 50000 (5000m)");
    CHECK(sf625.at("SDR").at("SDR")    == 100,              "625/SDR = 100 (10m)");
    CHECK(sf625.at("RR").at("RR")      == s24(-200),        "625/RR = -200 (-20m/s)");
    CHECK(sf625.at("SDRR").at("SDRR")  == 50,              "625/SDRR = 50 (5m/s)");
    CHECK(sf625.at("SDRR").at("CORRR") == s8(-32),         "625/SDRR/CORRR = -32");
    CHECK(sf625.at("RA").at("RA")      == s16(-128),        "625/RA = -128 (-2m/s²)");

    CHECK(items.count("626"),                               "I626 present");
    const auto& sf626 = items.at("626").compound_sub_fields;
    CHECK(sf626.count("DV"),                                "626/DV present");
    CHECK(sf626.count("SDDV"),                              "626/SDDV present");
    CHECK(sf626.count("CODVR"),                             "626/CODVR present");
    CHECK(!sf626.count("DA"),                               "626/DA absent");
    CHECK(sf626.at("DV").at("DV")        == s24(-1500),     "626/DV = -1500 (-15m/s)");
    CHECK(sf626.at("SDDV").at("SDDV")   == 200,             "626/SDDV = 200");
    CHECK(sf626.at("CODVR").at("CODVR") == s8(-64),         "626/CODVR = -64 (-0.5)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 10: Full round-trip – complete INCSS target report
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip INCSS target report ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=0; it.fields["SIC"]=7; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MT"]=2; it.fields["RG"]=0; rec.items["000"]=it; }
    { DecodedItem it; it.fields["SID"]=1; rec.items["015"]=it; }
    { DecodedItem it; it.fields["TOA"]=36000u*128u; rec.items["145"]=it; }
    { DecodedItem it; it.fields["TPN"]=0x1234; rec.items["161"]=it; }
    { DecodedItem it; it.fields["PID"]=1; it.fields["ON"]=42; rec.items["400"]=it; }

    // I020: 1 octet
    { DecodedItem it;
      it.fields["MOMU"]=0; it.fields["TTAX"]=0; it.fields["SCD"]=1;
      rec.items["020"]=it; }

    // I170: 1 octet (confirmed, alive)
    { DecodedItem it;
      it.fields["BIZ"]=0; it.fields["BAZ"]=0; it.fields["TUR"]=0;
      it.fields["CSTP"]=0; it.fields["CSTH"]=0; it.fields["CNF"]=0;
      rec.items["170"]=it; }

    // I600: P84 + HPR
    DecodedItem i600;
    i600.compound_sub_fields["P84"]["LATITUDE"]  = s32(485000000);
    i600.compound_sub_fields["P84"]["LONGITUDE"] = s32(23000000);
    i600.compound_sub_fields["HPR"]["RSHPX"]    = 50;
    i600.compound_sub_fields["HPR"]["RSHPY"]    = 50;
    i600.compound_sub_fields["HPR"]["CORSHPXY"] = s8(0);
    rec.items["600"] = i600;

    // I625: R + RR
    DecodedItem i625;
    i625.compound_sub_fields["R"]["R"]   = s24(120000);  // 12000m
    i625.compound_sub_fields["RR"]["RR"] = s24(-500);    // -50m/s
    rec.items["625"] = i625;

    // I627: AZ + RSAZ
    DecodedItem i627;
    i627.compound_sub_fields["AZ"]["AZ"]     = 16384; // 90°
    i627.compound_sub_fields["RSAZ"]["RSAZ"] = 100;
    rec.items["627"] = i627;

    // I300: one classification
    DecodedItem i300;
    i300.group_repetitions.push_back({{"CLS", 3}, {"PRB", 75}});
    rec.items["300"] = i300;

    auto encoded = codec.encode(15, {rec});
    hexdump(encoded, "Full round-trip encoded");
    CHECK(!encoded.empty(), "encode produced output");
    CHECK(encoded[0] == 0x0F, "CAT byte = 0x0F (15)");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("SAC") == 0,             "010/SAC = 0");
    CHECK(items.at("010").fields.at("SIC") == 7,             "010/SIC = 7");
    CHECK(items.at("000").fields.at("MT")  == 2,             "000/MT = 2 (Track)");
    CHECK(items.at("015").fields.at("SID") == 1,             "015/SID = 1");
    CHECK(items.at("145").fields.at("TOA") == 36000u*128u,   "145/TOA round-trip");
    CHECK(items.at("161").fields.at("TPN") == 0x1234,        "161/TPN = 0x1234");
    CHECK(items.at("020").fields.at("SCD") == 1,             "020/SCD = 1 (Forward)");
    CHECK(items.at("170").fields.at("CNF") == 0,             "170/CNF = 0 (Confirmed)");
    CHECK(items.at("400").fields.at("PID") == 1,             "400/PID = 1");
    CHECK(items.at("400").fields.at("ON")  == 42,            "400/ON = 42");

    const auto& sf600 = items.at("600").compound_sub_fields;
    CHECK(sf600.at("P84").at("LATITUDE")  == s32(485000000), "600/P84/LAT round-trip");
    CHECK(sf600.at("P84").at("LONGITUDE") == s32(23000000),  "600/P84/LON round-trip");
    CHECK(sf600.at("HPR").at("RSHPX")    == 50,              "600/HPR/RSHPX = 50");

    const auto& sf625 = items.at("625").compound_sub_fields;
    CHECK(sf625.at("R").at("R")   == s24(120000),            "625/R = 120000 (12km)");
    CHECK(sf625.at("RR").at("RR") == s24(-500),              "625/RR = -500 (-50m/s)");

    const auto& sf627 = items.at("627").compound_sub_fields;
    CHECK(sf627.at("AZ").at("AZ")     == 16384,              "627/AZ = 16384 (90°)");
    CHECK(sf627.at("RSAZ").at("RSAZ") == 100,                "627/RSAZ = 100");

    const auto& gr300 = items.at("300").group_repetitions;
    CHECK(gr300.size() == 1,           "300 has 1 entry");
    CHECK(gr300[0].at("CLS") == 3,     "300 rep[0] CLS=3");
    CHECK(gr300[0].at("PRB") == 75,    "300 rep[0] PRB=75");
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = (argc > 1) ? argv[1] : "specs/CAT15.xml";

    Codec codec;
    testSpecLoad(codec, spec_path);
    testDecodeBasicTargetReport(codec);
    testRoundTripFixed(codec);
    testRoundTripExtended(codec);
    testRoundTripRepetitiveFX(codec);
    testRoundTripCompoundI270(codec);
    testRoundTripRepGroups(codec);
    testRoundTripCompoundI600(codec);
    testRoundTripCompoundI625andI626(codec);
    testFullRoundTrip(codec);

    std::cout << "\n=== Results: "
              << (failures == 0 ? "ALL PASSED" : std::to_string(failures) + " FAILED")
              << " ===\n";
    return failures == 0 ? 0 : 1;
}
