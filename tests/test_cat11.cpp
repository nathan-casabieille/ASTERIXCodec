// test_cat11.cpp – Tests for CAT11 Transmission of A-SMGCS Data, Ed. 1.3.

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
static uint64_t s32(int32_t v) { return static_cast<uint64_t>(static_cast<uint32_t>(v)); }

// ─────────────────────────────────────────────────────────────────────────────
//  Test 1: Spec load
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT11 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 11,         "cat number = 11");
    CHECK(cat.edition == "1.3",  "edition = 1.3");

    for (auto id : {"000","010","015","041","042","060","090","092","093",
                    "140","161","170","202","210","215","245","270","290",
                    "300","310","380","390","430","500","600","605","610",
                    "SP","RE"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 29, "29 items total");

    // Single UAP, 29 slots
    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    const auto& uap = cat.uap_variations.at("default");
    CHECK(uap.size() == 29, "UAP has 29 slots");

    // Spot-check UAP slot order
    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "000", "UAP slot  2 = 000");
    CHECK(uap[2]  == "015", "UAP slot  3 = 015");
    CHECK(uap[3]  == "140", "UAP slot  4 = 140");
    CHECK(uap[4]  == "041", "UAP slot  5 = 041");
    CHECK(uap[5]  == "042", "UAP slot  6 = 042");
    CHECK(uap[6]  == "202", "UAP slot  7 = 202");
    CHECK(uap[7]  == "210", "UAP slot  8 = 210");
    CHECK(uap[8]  == "060", "UAP slot  9 = 060");
    CHECK(uap[9]  == "245", "UAP slot 10 = 245");
    CHECK(uap[10] == "380", "UAP slot 11 = 380");
    CHECK(uap[11] == "161", "UAP slot 12 = 161");
    CHECK(uap[12] == "170", "UAP slot 13 = 170");
    CHECK(uap[13] == "290", "UAP slot 14 = 290");
    CHECK(uap[14] == "430", "UAP slot 15 = 430");
    CHECK(uap[15] == "090", "UAP slot 16 = 090");
    CHECK(uap[16] == "093", "UAP slot 17 = 093");
    CHECK(uap[17] == "092", "UAP slot 18 = 092");
    CHECK(uap[18] == "215", "UAP slot 19 = 215");
    CHECK(uap[19] == "270", "UAP slot 20 = 270");
    CHECK(uap[20] == "390", "UAP slot 21 = 390");
    CHECK(uap[21] == "300", "UAP slot 22 = 300");
    CHECK(uap[22] == "310", "UAP slot 23 = 310");
    CHECK(uap[23] == "500", "UAP slot 24 = 500");
    CHECK(uap[24] == "600", "UAP slot 25 = 600");
    CHECK(uap[25] == "605", "UAP slot 26 = 605");
    CHECK(uap[26] == "610", "UAP slot 27 = 610");
    CHECK(uap[27] == "SP",  "UAP slot 28 = SP");
    CHECK(uap[28] == "RE",  "UAP slot 29 = RE");

    // Item types
    for (auto id : {"000","010","015","041","042","060","090","092","093",
                    "140","161","202","210","215","245","300","310","430","600"}) {
        CHECK(cat.items.at(id).type == ItemType::Fixed,
              std::string(id) + " is Fixed");
    }
    CHECK(cat.items.at("170").type == ItemType::Extended,        "170 is Extended");
    CHECK(cat.items.at("270").type == ItemType::Extended,        "270 is Extended");
    CHECK(cat.items.at("290").type == ItemType::Compound,        "290 is Compound");
    CHECK(cat.items.at("380").type == ItemType::Compound,        "380 is Compound");
    CHECK(cat.items.at("390").type == ItemType::Compound,        "390 is Compound");
    CHECK(cat.items.at("500").type == ItemType::Compound,        "500 is Compound");
    CHECK(cat.items.at("605").type == ItemType::RepetitiveGroup, "605 is RepetitiveGroup");
    CHECK(cat.items.at("610").type == ItemType::RepetitiveGroup, "610 is RepetitiveGroup");
    CHECK(cat.items.at("SP").type  == ItemType::SP,              "SP is SP/Explicit");
    CHECK(cat.items.at("RE").type  == ItemType::SP,              "RE is SP/Explicit");

    // Fixed byte sizes
    CHECK(cat.items.at("000").fixed_bytes == 1,  "000 = 1 byte");
    CHECK(cat.items.at("010").fixed_bytes == 2,  "010 = 2 bytes");
    CHECK(cat.items.at("015").fixed_bytes == 1,  "015 = 1 byte");
    CHECK(cat.items.at("041").fixed_bytes == 8,  "041 = 8 bytes");
    CHECK(cat.items.at("042").fixed_bytes == 4,  "042 = 4 bytes");
    CHECK(cat.items.at("060").fixed_bytes == 2,  "060 = 2 bytes");
    CHECK(cat.items.at("090").fixed_bytes == 2,  "090 = 2 bytes");
    CHECK(cat.items.at("092").fixed_bytes == 2,  "092 = 2 bytes");
    CHECK(cat.items.at("093").fixed_bytes == 2,  "093 = 2 bytes");
    CHECK(cat.items.at("140").fixed_bytes == 3,  "140 = 3 bytes");
    CHECK(cat.items.at("161").fixed_bytes == 2,  "161 = 2 bytes");
    CHECK(cat.items.at("202").fixed_bytes == 4,  "202 = 4 bytes");
    CHECK(cat.items.at("210").fixed_bytes == 2,  "210 = 2 bytes");
    CHECK(cat.items.at("215").fixed_bytes == 2,  "215 = 2 bytes");
    CHECK(cat.items.at("245").fixed_bytes == 7,  "245 = 7 bytes");
    CHECK(cat.items.at("300").fixed_bytes == 1,  "300 = 1 byte");
    CHECK(cat.items.at("310").fixed_bytes == 1,  "310 = 1 byte");
    CHECK(cat.items.at("430").fixed_bytes == 1,  "430 = 1 byte");
    CHECK(cat.items.at("600").fixed_bytes == 3,  "600 = 3 bytes");

    // Extended octets
    CHECK(cat.items.at("170").octets.size() == 4, "170 has 4 octets");
    CHECK(cat.items.at("270").octets.size() == 3, "270 has 3 octets");

    // RepetitiveGroup bits
    CHECK(cat.items.at("605").rep_group_bits == 16, "605 rep_group_bits = 16");
    CHECK(cat.items.at("610").rep_group_bits == 16, "610 rep_group_bits = 16");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode a minimal target report from raw bytes
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicTargetReport(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT11 target report ===\n";

    // UAP slots: 010(b7) 000(b6) → FSPEC 0xC0
    // Items: 010=SAC1,SIC2  000=MSGTYP1
    // Total: CAT(1)+LEN(2)+FSPEC(1)+010(2)+000(1) = 7 bytes
    std::vector<uint8_t> frame = {
        0x0B,             // CAT = 11
        0x00, 0x07,       // LEN = 7
        0xC0,             // FSPEC: 010+000 present, FX=0
        0x01, 0x02,       // I010: SAC=1, SIC=2
        0x01              // I000: MSGTYP=1
    };

    hexdump(frame, "Basic target report input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid, "block is valid");
    CHECK(blk.records.size() == 1, "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"),  "I010 present");
    CHECK(rec.items.count("000"),  "I000 present");
    CHECK(!rec.items.count("041"), "I041 absent");

    CHECK(rec.items.at("010").fields.at("SAC")    == 1, "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC")    == 2, "I010.SIC=2");
    CHECK(rec.items.at("000").fields.at("MSGTYP") == 1, "I000.MSGTYP=1");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip simple Fixed items
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip simple Fixed items ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=3; it.fields["SIC"]=7; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["000"]=it; }
    { DecodedItem it; it.fields["SID"]=0x42; rec.items["015"]=it; }
    { DecodedItem it; it.fields["TOT"]=36000u*128u; rec.items["140"]=it; }
    { DecodedItem it; it.fields["FTN"]=0x7FFF; rec.items["161"]=it; }
    // I042: X=-100m, Y=200m
    { DecodedItem it; it.fields["X"]=s16(-100); it.fields["Y"]=200; rec.items["042"]=it; }
    // I202: VX=-8 LSB = -2 m/s, VY=12 LSB = 3 m/s
    { DecodedItem it; it.fields["VX"]=s16(-8); it.fields["VY"]=12; rec.items["202"]=it; }
    // I210: AX=-4 LSB = -1 m/s², AY=8 = 2 m/s²
    { DecodedItem it; it.fields["AX"]=s8(-4); it.fields["AY"]=8; rec.items["210"]=it; }
    // I215: ROCD=-80 LSB = -500 ft/min
    { DecodedItem it; it.fields["ROCD"]=s16(-80); rec.items["215"]=it; }
    // I060: spare(4)+MOD3A(12)
    { DecodedItem it; it.fields["MOD3A"]=0x5A3; rec.items["060"]=it; }
    // I090: FL=400 LSB = 100 FL
    { DecodedItem it; it.fields["FL"]=400; rec.items["090"]=it; }
    // I430: POF=2 (Taxiing for departure)
    { DecodedItem it; it.fields["POF"]=2; rec.items["430"]=it; }
    // I300: VFI=9 (Bus)
    { DecodedItem it; it.fields["VFI"]=9; rec.items["300"]=it; }
    // I310: TRB=1, MSG=2
    { DecodedItem it; it.fields["TRB"]=1; it.fields["MSG"]=2; rec.items["310"]=it; }

    auto encoded = codec.encode(11, {rec});
    hexdump(encoded, "SimpleFixed encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("SAC") == 3,            "010/SAC = 3");
    CHECK(items.at("010").fields.at("SIC") == 7,            "010/SIC = 7");
    CHECK(items.at("000").fields.at("MSGTYP") == 1,         "000/MSGTYP = 1");
    CHECK(items.at("015").fields.at("SID") == 0x42,         "015/SID = 0x42");
    CHECK(items.at("140").fields.at("TOT") == 36000u*128u,  "140/TOT round-trip");
    CHECK(items.at("161").fields.at("FTN") == 0x7FFF,       "161/FTN = 0x7FFF");
    CHECK(items.at("042").fields.at("X") == s16(-100),      "042/X = -100 m");
    CHECK(items.at("042").fields.at("Y") == 200,            "042/Y = 200 m");
    CHECK(items.at("202").fields.at("VX") == s16(-8),       "202/VX = -8 LSB");
    CHECK(items.at("202").fields.at("VY") == 12,            "202/VY = 12 LSB");
    CHECK(items.at("210").fields.at("AX") == s8(-4),        "210/AX = -4 LSB");
    CHECK(items.at("210").fields.at("AY") == 8,             "210/AY = 8 LSB");
    CHECK(items.at("215").fields.at("ROCD") == s16(-80),    "215/ROCD = -80 LSB");
    CHECK(items.at("060").fields.at("MOD3A") == 0x5A3,      "060/MOD3A = 0x5A3");
    CHECK(items.at("090").fields.at("FL") == 400,           "090/FL = 400 LSB");
    CHECK(items.at("430").fields.at("POF") == 2,            "430/POF = 2");
    CHECK(items.at("300").fields.at("VFI") == 9,            "300/VFI = 9 (Bus)");
    CHECK(items.at("310").fields.at("TRB") == 1,            "310/TRB = 1");
    CHECK(items.at("310").fields.at("MSG") == 2,            "310/MSG = 2");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip Extended I170 (Track Status, 4 octets)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtendedI170(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Extended I170 (4 octets) ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["000"]=it; }

    // I170: all 4 octets
    DecodedItem i170;
    // Octet 1: MON GBS MRH SRC CNF
    i170.fields["MON"]=1; i170.fields["GBS"]=0; i170.fields["MRH"]=1;
    i170.fields["SRC"]=3; i170.fields["CNF"]=0;
    // Octet 2: SIM TSE TSB FRIFOE ME MI
    i170.fields["SIM"]=0; i170.fields["TSE"]=0; i170.fields["TSB"]=1;
    i170.fields["FRIFOE"]=1; i170.fields["ME"]=0; i170.fields["MI"]=0;
    // Octet 3: AMA SPI CST FPC AFF spare
    i170.fields["AMA"]=1; i170.fields["SPI"]=0; i170.fields["CST"]=1;
    i170.fields["FPC"]=1; i170.fields["AFF"]=0;
    // Octet 4: spare PSR SSR MDS ADS SUC AAC
    i170.fields["PSR"]=1; i170.fields["SSR"]=0; i170.fields["MDS"]=1;
    i170.fields["ADS"]=0; i170.fields["SUC"]=0; i170.fields["AAC"]=1;
    rec.items["170"] = i170;

    auto encoded = codec.encode(11, {rec});
    hexdump(encoded, "Extended I170 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& f = blk.records[0].items.at("170").fields;
    CHECK(f.at("MON") == 1,     "170/MON = 1 (Monosensor)");
    CHECK(f.at("MRH") == 1,     "170/MRH = 1");
    CHECK(f.at("SRC") == 3,     "170/SRC = 3 (Triangulation)");
    CHECK(f.at("CNF") == 0,     "170/CNF = 0 (Confirmed)");
    CHECK(f.at("TSB") == 1,     "170/TSB = 1 (Track service begin)");
    CHECK(f.at("FRIFOE") == 1,  "170/FRIFOE = 1 (Friendly)");
    CHECK(f.at("AMA") == 1,     "170/AMA = 1 (Amalgamation)");
    CHECK(f.at("CST") == 1,     "170/CST = 1 (Coasting)");
    CHECK(f.at("FPC") == 1,     "170/FPC = 1 (Flight plan correlated)");
    CHECK(f.at("PSR") == 1,     "170/PSR = 1");
    CHECK(f.at("MDS") == 1,     "170/MDS = 1");
    CHECK(f.at("AAC") == 1,     "170/AAC = 1");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip Extended I270 (Target Size and Orientation, 3 octets)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtendedI270(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Extended I270 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["000"]=it; }

    // I270: LENGTH=30m, ORIENTATION=45 LSBs, WIDTH=7m
    DecodedItem i270;
    i270.fields["LENGTH"]      = 30;
    i270.fields["ORIENTATION"] = 45;
    i270.fields["WIDTH"]       = 7;
    rec.items["270"] = i270;

    auto encoded = codec.encode(11, {rec});
    hexdump(encoded, "Extended I270 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& f = blk.records[0].items.at("270").fields;
    CHECK(f.at("LENGTH")      == 30, "270/LENGTH = 30");
    CHECK(f.at("ORIENTATION") == 45, "270/ORIENTATION = 45");
    CHECK(f.at("WIDTH")       == 7,  "270/WIDTH = 7");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip Compound I290 (System Track Update Ages)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCompoundI290(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Compound I290 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["000"]=it; }

    // I290: PSR=4 (1s), SSR=8 (2s), MDA=12 (3s), ADS=120 (30s), TRK=40 (10s)
    DecodedItem i290;
    i290.compound_sub_fields["PSR"]["PSR"] = 4;
    i290.compound_sub_fields["SSR"]["SSR"] = 8;
    i290.compound_sub_fields["MDA"]["MDA"] = 12;
    i290.compound_sub_fields["ADS"]["ADS"] = 120;
    i290.compound_sub_fields["TRK"]["TRK"] = 40;
    rec.items["290"] = i290;

    auto encoded = codec.encode(11, {rec});
    hexdump(encoded, "Compound I290 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& sf = blk.records[0].items.at("290").compound_sub_fields;
    CHECK(sf.count("PSR"),              "290/PSR present");
    CHECK(sf.count("SSR"),              "290/SSR present");
    CHECK(sf.count("MDA"),              "290/MDA present");
    CHECK(sf.count("ADS"),              "290/ADS present");
    CHECK(sf.count("TRK"),              "290/TRK present");
    CHECK(!sf.count("MFL"),             "290/MFL absent");
    CHECK(sf.at("PSR").at("PSR") == 4,  "290/PSR = 4 LSBs (1s)");
    CHECK(sf.at("SSR").at("SSR") == 8,  "290/SSR = 8 LSBs (2s)");
    CHECK(sf.at("MDA").at("MDA") == 12, "290/MDA = 12 LSBs (3s)");
    CHECK(sf.at("ADS").at("ADS") == 120,"290/ADS = 120 LSBs (30s)");
    CHECK(sf.at("TRK").at("TRK") == 40, "290/TRK = 40 LSBs (10s)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip Compound I380 (Mode-S / ADS-B Related Data)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCompoundI380(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Compound I380 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["000"]=it; }

    // I380: ADR + COMACAS + ECAT + AVTECH
    DecodedItem i380;
    i380.compound_sub_fields["ADR"]["ADR"] = 0xABCDEF;

    // COMACAS: COM=1, STAT=1, SSC=1, ARC=1, AIC=0, B1A=0, B1B=3, AC=1, MN=0, DC=0
    i380.compound_sub_fields["COMACAS"]["COM"]  = 1;
    i380.compound_sub_fields["COMACAS"]["STAT"] = 1;
    i380.compound_sub_fields["COMACAS"]["SSC"]  = 1;
    i380.compound_sub_fields["COMACAS"]["ARC"]  = 1;
    i380.compound_sub_fields["COMACAS"]["AIC"]  = 0;
    i380.compound_sub_fields["COMACAS"]["B1A"]  = 0;
    i380.compound_sub_fields["COMACAS"]["B1B"]  = 3;
    i380.compound_sub_fields["COMACAS"]["AC"]   = 1;
    i380.compound_sub_fields["COMACAS"]["MN"]   = 0;
    i380.compound_sub_fields["COMACAS"]["DC"]   = 0;

    // ECAT=5 (heavy aircraft)
    i380.compound_sub_fields["ECAT"]["ECAT"] = 5;

    // AVTECH: VDL=0, MDS=0, UAT=1
    i380.compound_sub_fields["AVTECH"]["VDL"] = 0;
    i380.compound_sub_fields["AVTECH"]["MDS"] = 0;
    i380.compound_sub_fields["AVTECH"]["UAT"] = 1;

    rec.items["380"] = i380;

    auto encoded = codec.encode(11, {rec});
    hexdump(encoded, "Compound I380 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& sf = blk.records[0].items.at("380").compound_sub_fields;
    CHECK(sf.count("ADR"),                       "380/ADR present");
    CHECK(sf.count("COMACAS"),                   "380/COMACAS present");
    CHECK(sf.count("ECAT"),                      "380/ECAT present");
    CHECK(sf.count("AVTECH"),                    "380/AVTECH present");
    CHECK(!sf.count("ACT"),                      "380/ACT absent");

    CHECK(sf.at("ADR").at("ADR") == 0xABCDEF,   "380/ADR = 0xABCDEF");
    CHECK(sf.at("COMACAS").at("COM")  == 1,      "380/COMACAS/COM = 1");
    CHECK(sf.at("COMACAS").at("STAT") == 1,      "380/COMACAS/STAT = 1");
    CHECK(sf.at("COMACAS").at("SSC")  == 1,      "380/COMACAS/SSC = 1");
    CHECK(sf.at("COMACAS").at("ARC")  == 1,      "380/COMACAS/ARC = 1");
    CHECK(sf.at("COMACAS").at("B1B")  == 3,      "380/COMACAS/B1B = 3");
    CHECK(sf.at("COMACAS").at("AC")   == 1,      "380/COMACAS/AC = 1");
    CHECK(sf.at("ECAT").at("ECAT")    == 5,      "380/ECAT = 5 (heavy)");
    CHECK(sf.at("AVTECH").at("VDL")   == 0,      "380/AVTECH/VDL = 0");
    CHECK(sf.at("AVTECH").at("UAT")   == 1,      "380/AVTECH/UAT = 1");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Round-trip Compound I390 (Flight Plan Related Data)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCompoundI390(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Compound I390 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["000"]=it; }

    // I390: FPPSID + CSN + FLIGHTCAT + WTC + CFL + CCP + STS
    DecodedItem i390;
    i390.compound_sub_fields["FPPSID"]["SAC"] = 10;
    i390.compound_sub_fields["FPPSID"]["SIC"] = 20;

    // CSN: 'AFR1234\0' packed as 7 ASCII bytes = 0x41465231323334 padded
    i390.compound_sub_fields["CSN"]["CSN"] = 0x41465231323334ULL;

    // FLIGHTCAT: GATOAT=1 (GAT), FR1FR2=0 (IFR), RVSM=1, HPR=0
    i390.compound_sub_fields["FLIGHTCAT"]["GATOAT"] = 1;
    i390.compound_sub_fields["FLIGHTCAT"]["FR1FR2"] = 0;
    i390.compound_sub_fields["FLIGHTCAT"]["RVSM"]   = 1;
    i390.compound_sub_fields["FLIGHTCAT"]["HPR"]    = 0;

    // WTC: 'M' = 0x4D (Medium)
    i390.compound_sub_fields["WTC"]["WTC"] = 0x4D;

    // CFL: 360 LSBs = 90 FL
    i390.compound_sub_fields["CFL"]["CFL"] = 360;

    // CCP: CENTRE=5, POSITION=3
    i390.compound_sub_fields["CCP"]["CENTRE"]   = 5;
    i390.compound_sub_fields["CCP"]["POSITION"] = 3;

    // STS: EMP=1 (Occupied), AVL=1 (Not available)
    i390.compound_sub_fields["STS"]["EMP"] = 1;
    i390.compound_sub_fields["STS"]["AVL"] = 1;

    rec.items["390"] = i390;

    auto encoded = codec.encode(11, {rec});
    hexdump(encoded, "Compound I390 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& sf = blk.records[0].items.at("390").compound_sub_fields;
    CHECK(sf.count("FPPSID"),                          "390/FPPSID present");
    CHECK(sf.count("CSN"),                             "390/CSN present");
    CHECK(sf.count("FLIGHTCAT"),                       "390/FLIGHTCAT present");
    CHECK(sf.count("WTC"),                             "390/WTC present");
    CHECK(sf.count("CFL"),                             "390/CFL present");
    CHECK(sf.count("CCP"),                             "390/CCP present");
    CHECK(sf.count("STS"),                             "390/STS present");
    CHECK(!sf.count("TOD"),                            "390/TOD absent (not supported)");

    CHECK(sf.at("FPPSID").at("SAC") == 10,             "390/FPPSID/SAC = 10");
    CHECK(sf.at("FPPSID").at("SIC") == 20,             "390/FPPSID/SIC = 20");
    CHECK(sf.at("CSN").at("CSN") == 0x41465231323334ULL,"390/CSN round-trip");
    CHECK(sf.at("FLIGHTCAT").at("GATOAT") == 1,        "390/FLIGHTCAT/GATOAT = 1 (GAT)");
    CHECK(sf.at("FLIGHTCAT").at("FR1FR2") == 0,        "390/FLIGHTCAT/FR1FR2 = 0 (IFR)");
    CHECK(sf.at("FLIGHTCAT").at("RVSM")   == 1,        "390/FLIGHTCAT/RVSM = 1 (Approved)");
    CHECK(sf.at("WTC").at("WTC") == 0x4D,              "390/WTC = 0x4D (M)");
    CHECK(sf.at("CFL").at("CFL") == 360,               "390/CFL = 360 LSBs (90 FL)");
    CHECK(sf.at("CCP").at("CENTRE")   == 5,            "390/CCP/CENTRE = 5");
    CHECK(sf.at("CCP").at("POSITION") == 3,            "390/CCP/POSITION = 3");
    CHECK(sf.at("STS").at("EMP") == 1,                 "390/STS/EMP = 1 (Occupied)");
    CHECK(sf.at("STS").at("AVL") == 1,                 "390/STS/AVL = 1 (Not available)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 9: Round-trip alert items (I600, I605, I610) and Compound I500
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripAlertsAndAccuracies(Codec& codec) {
    std::cout << "\n=== Test: Round-trip alerts and estimated accuracies ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["000"]=it; }

    // I600: ACK=1, SVR=2 (Severe), AT=0x05, AN=0x12
    { DecodedItem it;
      it.fields["ACK"]=1; it.fields["SVR"]=2;
      it.fields["AT"]=0x05; it.fields["AN"]=0x12;
      rec.items["600"]=it; }

    // I605: two tracks in alert
    DecodedItem i605;
    i605.group_repetitions.push_back({{"FTN", 42}});
    i605.group_repetitions.push_back({{"FTN", 100}});
    rec.items["605"] = i605;

    // I610: one holdbar bank
    DecodedItem i610;
    i610.group_repetitions.push_back({
        {"BKN", 3},
        {"I1",0},{"I2",1},{"I3",0},{"I4",1},{"I5",0},{"I6",0},
        {"I7",1},{"I8",0},{"I9",0},{"I10",1},{"I11",0},{"I12",1}
    });
    rec.items["610"] = i610;

    // I500: APC + ATH + AVC
    DecodedItem i500;
    i500.compound_sub_fields["APC"]["X"] = 8;
    i500.compound_sub_fields["APC"]["Y"] = 12;
    i500.compound_sub_fields["ATH"]["ATH"] = s16(-20);
    i500.compound_sub_fields["AVC"]["X"] = 5;
    i500.compound_sub_fields["AVC"]["Y"] = 5;
    rec.items["500"] = i500;

    auto encoded = codec.encode(11, {rec});
    hexdump(encoded, "Alerts+Accuracies encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;

    // I600
    CHECK(items.count("600"),                          "I600 present");
    CHECK(items.at("600").fields.at("ACK") == 1,       "600/ACK = 1");
    CHECK(items.at("600").fields.at("SVR") == 2,       "600/SVR = 2 (Severe)");
    CHECK(items.at("600").fields.at("AT")  == 0x05,    "600/AT = 0x05");
    CHECK(items.at("600").fields.at("AN")  == 0x12,    "600/AN = 0x12");

    // I605
    CHECK(items.count("605"),                          "I605 present");
    const auto& gr605 = items.at("605").group_repetitions;
    CHECK(gr605.size() == 2,                           "605 has 2 tracks");
    CHECK(gr605[0].at("FTN") == 42,                   "605 rep[0] FTN=42");
    CHECK(gr605[1].at("FTN") == 100,                  "605 rep[1] FTN=100");

    // I610
    CHECK(items.count("610"),                          "I610 present");
    const auto& gr610 = items.at("610").group_repetitions;
    CHECK(gr610.size() == 1,                           "610 has 1 bank");
    CHECK(gr610[0].at("BKN") == 3,                    "610 rep[0] BKN=3");
    CHECK(gr610[0].at("I2")  == 1,                    "610 rep[0] I2=1 (off)");
    CHECK(gr610[0].at("I4")  == 1,                    "610 rep[0] I4=1 (off)");
    CHECK(gr610[0].at("I7")  == 1,                    "610 rep[0] I7=1 (off)");
    CHECK(gr610[0].at("I12") == 1,                    "610 rep[0] I12=1 (off)");

    // I500
    CHECK(items.count("500"),                          "I500 present");
    const auto& sf500 = items.at("500").compound_sub_fields;
    CHECK(sf500.count("APC"),                          "500/APC present");
    CHECK(sf500.count("ATH"),                          "500/ATH present");
    CHECK(sf500.count("AVC"),                          "500/AVC present");
    CHECK(!sf500.count("APW"),                         "500/APW absent");
    CHECK(sf500.at("APC").at("X") == 8,               "500/APC/X = 8");
    CHECK(sf500.at("APC").at("Y") == 12,              "500/APC/Y = 12");
    CHECK(sf500.at("ATH").at("ATH") == s16(-20),      "500/ATH = -20 LSBs");
    CHECK(sf500.at("AVC").at("X")  == 5,              "500/AVC/X = 5");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 10: Full round-trip – complete A-SMGCS target report
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip A-SMGCS target report ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=0; it.fields["SIC"]=5; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["000"]=it; }
    { DecodedItem it; it.fields["SID"]=1; rec.items["015"]=it; }
    { DecodedItem it; it.fields["TOT"]=43200u*128u; rec.items["140"]=it; }

    // I041: WGS-84 position
    { DecodedItem it;
      it.fields["LAT"] = s32(580000000);
      it.fields["LON"] = s32(27000000);
      rec.items["041"]=it; }

    // I042: X=-300m, Y=500m
    { DecodedItem it; it.fields["X"]=s16(-300); it.fields["Y"]=500; rec.items["042"]=it; }

    // I202: VX=20, VY=-10
    { DecodedItem it; it.fields["VX"]=20; it.fields["VY"]=s16(-10); rec.items["202"]=it; }

    // I161: FTN=0x1234
    { DecodedItem it; it.fields["FTN"]=0x1234; rec.items["161"]=it; }

    // I170: octet 1+2 (monosensor, confirmed, track service begin)
    { DecodedItem it;
      it.fields["MON"]=1; it.fields["GBS"]=1; it.fields["MRH"]=0;
      it.fields["SRC"]=1; it.fields["CNF"]=0;
      it.fields["SIM"]=0; it.fields["TSE"]=0; it.fields["TSB"]=1;
      it.fields["FRIFOE"]=0; it.fields["ME"]=0; it.fields["MI"]=0;
      rec.items["170"]=it; }

    // I245: STI=0, TID=arbitrary 48-bit
    { DecodedItem it;
      it.fields["STI"]=0; it.fields["TID"]=0x4142434445464748ULL & 0x0000FFFFFFFFFFFFULL;
      rec.items["245"]=it; }

    // I093: QNH=1, CTBA=1200 LSBs = 300 FL
    { DecodedItem it; it.fields["QNH"]=1; it.fields["CTBA"]=1200; rec.items["093"]=it; }

    // I092: GAlt=1600 LSBs = 10000 ft
    { DecodedItem it; it.fields["GAlt"]=1600; rec.items["092"]=it; }

    // I430: POF=1 (On stand)
    { DecodedItem it; it.fields["POF"]=1; rec.items["430"]=it; }

    auto encoded = codec.encode(11, {rec});
    hexdump(encoded, "Full round-trip encoded");
    CHECK(!encoded.empty(), "encode produced output");
    CHECK(encoded[0] == 0x0B, "CAT byte = 0x0B (11)");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("SAC") == 0,            "010/SAC = 0");
    CHECK(items.at("010").fields.at("SIC") == 5,            "010/SIC = 5");
    CHECK(items.at("000").fields.at("MSGTYP") == 1,         "000/MSGTYP = 1");
    CHECK(items.at("015").fields.at("SID") == 1,            "015/SID = 1");
    CHECK(items.at("140").fields.at("TOT") == 43200u*128u,  "140/TOT round-trip");
    CHECK(items.at("041").fields.at("LAT") == s32(580000000),"041/LAT round-trip");
    CHECK(items.at("041").fields.at("LON") == s32(27000000), "041/LON round-trip");
    CHECK(items.at("042").fields.at("X") == s16(-300),      "042/X = -300");
    CHECK(items.at("042").fields.at("Y") == 500,            "042/Y = 500");
    CHECK(items.at("202").fields.at("VX") == 20,            "202/VX = 20");
    CHECK(items.at("202").fields.at("VY") == s16(-10),      "202/VY = -10");
    CHECK(items.at("161").fields.at("FTN") == 0x1234,       "161/FTN = 0x1234");
    CHECK(items.at("170").fields.at("MON") == 1,            "170/MON = 1 (Monosensor)");
    CHECK(items.at("170").fields.at("GBS") == 1,            "170/GBS = 1");
    CHECK(items.at("170").fields.at("SRC") == 1,            "170/SRC = 1 (GPS)");
    CHECK(items.at("170").fields.at("TSB") == 1,            "170/TSB = 1");
    CHECK(items.at("245").fields.at("STI") == 0,            "245/STI = 0");
    CHECK(items.at("093").fields.at("QNH") == 1,            "093/QNH = 1");
    CHECK(items.at("093").fields.at("CTBA") == 1200,        "093/CTBA = 1200 LSBs");
    CHECK(items.at("092").fields.at("GAlt") == 1600,        "092/GAlt = 1600 LSBs");
    CHECK(items.at("430").fields.at("POF") == 1,            "430/POF = 1 (On stand)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = (argc > 1) ? argv[1] : "specs/CAT11.xml";

    Codec codec;
    testSpecLoad(codec, spec_path);
    testDecodeBasicTargetReport(codec);
    testRoundTripFixed(codec);
    testRoundTripExtendedI170(codec);
    testRoundTripExtendedI270(codec);
    testRoundTripCompoundI290(codec);
    testRoundTripCompoundI380(codec);
    testRoundTripCompoundI390(codec);
    testRoundTripAlertsAndAccuracies(codec);
    testFullRoundTrip(codec);

    std::cout << "\n=== Results: "
              << (failures == 0 ? "ALL PASSED" : std::to_string(failures) + " FAILED")
              << " ===\n";
    return failures == 0 ? 0 : 1;
}
