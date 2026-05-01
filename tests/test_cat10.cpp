// test_cat10.cpp – Tests for CAT10 Transmission of Monosensor Surface Movement Data.

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
    std::cout << "\n=== Test: CAT10 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 10,         "cat number = 10");
    CHECK(cat.edition == "1.1",  "edition = 1.1");

    for (auto id : {"000","010","020","040","041","042","060","090","091",
                    "131","140","161","170","200","202","210","220","245",
                    "250","270","280","300","310","500","550","SP","RE"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 27, "27 items total");

    // Single UAP, 28 slots
    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    const auto& uap = cat.uap_variations.at("default");
    CHECK(uap.size() == 28, "UAP has 28 slots");

    // Spot-check UAP slot order
    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "000", "UAP slot  2 = 000");
    CHECK(uap[2]  == "020", "UAP slot  3 = 020");
    CHECK(uap[3]  == "140", "UAP slot  4 = 140");
    CHECK(uap[4]  == "041", "UAP slot  5 = 041");
    CHECK(uap[5]  == "040", "UAP slot  6 = 040");
    CHECK(uap[6]  == "042", "UAP slot  7 = 042");
    CHECK(uap[7]  == "200", "UAP slot  8 = 200");
    CHECK(uap[8]  == "202", "UAP slot  9 = 202");
    CHECK(uap[9]  == "161", "UAP slot 10 = 161");
    CHECK(uap[10] == "170", "UAP slot 11 = 170");
    CHECK(uap[11] == "060", "UAP slot 12 = 060");
    CHECK(uap[12] == "220", "UAP slot 13 = 220");
    CHECK(uap[13] == "245", "UAP slot 14 = 245");
    CHECK(uap[14] == "250", "UAP slot 15 = 250");
    CHECK(uap[15] == "300", "UAP slot 16 = 300");
    CHECK(uap[16] == "090", "UAP slot 17 = 090");
    CHECK(uap[17] == "091", "UAP slot 18 = 091");
    CHECK(uap[18] == "270", "UAP slot 19 = 270");
    CHECK(uap[19] == "550", "UAP slot 20 = 550");
    CHECK(uap[20] == "310", "UAP slot 21 = 310");
    CHECK(uap[21] == "500", "UAP slot 22 = 500");
    CHECK(uap[22] == "280", "UAP slot 23 = 280");
    CHECK(uap[23] == "131", "UAP slot 24 = 131");
    CHECK(uap[24] == "210", "UAP slot 25 = 210");
    CHECK(uap[25] == "-",   "UAP slot 26 = - (reserved)");
    CHECK(uap[26] == "SP",  "UAP slot 27 = SP");
    CHECK(uap[27] == "RE",  "UAP slot 28 = RE");

    // Item types
    for (auto id : {"000","010","040","041","042","060","090","091",
                    "131","140","161","200","202","210","220","245",
                    "300","310","500","550"}) {
        CHECK(cat.items.at(id).type == ItemType::Fixed,
              std::string(id) + " is Fixed");
    }
    CHECK(cat.items.at("020").type == ItemType::Extended,       "020 is Extended");
    CHECK(cat.items.at("170").type == ItemType::Extended,       "170 is Extended");
    CHECK(cat.items.at("270").type == ItemType::Extended,       "270 is Extended");
    CHECK(cat.items.at("250").type == ItemType::RepetitiveGroup,"250 is RepetitiveGroup");
    CHECK(cat.items.at("280").type == ItemType::RepetitiveGroup,"280 is RepetitiveGroup");
    CHECK(cat.items.at("SP").type  == ItemType::SP,             "SP is SP/Explicit");
    CHECK(cat.items.at("RE").type  == ItemType::SP,             "RE is SP/Explicit");

    // Fixed byte sizes
    CHECK(cat.items.at("000").fixed_bytes == 1,  "000 = 1 byte");
    CHECK(cat.items.at("010").fixed_bytes == 2,  "010 = 2 bytes");
    CHECK(cat.items.at("040").fixed_bytes == 4,  "040 = 4 bytes");
    CHECK(cat.items.at("041").fixed_bytes == 8,  "041 = 8 bytes");
    CHECK(cat.items.at("042").fixed_bytes == 4,  "042 = 4 bytes");
    CHECK(cat.items.at("060").fixed_bytes == 2,  "060 = 2 bytes");
    CHECK(cat.items.at("090").fixed_bytes == 2,  "090 = 2 bytes");
    CHECK(cat.items.at("091").fixed_bytes == 2,  "091 = 2 bytes");
    CHECK(cat.items.at("131").fixed_bytes == 1,  "131 = 1 byte");
    CHECK(cat.items.at("140").fixed_bytes == 3,  "140 = 3 bytes");
    CHECK(cat.items.at("161").fixed_bytes == 2,  "161 = 2 bytes");
    CHECK(cat.items.at("200").fixed_bytes == 4,  "200 = 4 bytes");
    CHECK(cat.items.at("202").fixed_bytes == 4,  "202 = 4 bytes");
    CHECK(cat.items.at("210").fixed_bytes == 2,  "210 = 2 bytes");
    CHECK(cat.items.at("220").fixed_bytes == 3,  "220 = 3 bytes");
    CHECK(cat.items.at("245").fixed_bytes == 7,  "245 = 7 bytes");
    CHECK(cat.items.at("300").fixed_bytes == 1,  "300 = 1 byte");
    CHECK(cat.items.at("310").fixed_bytes == 1,  "310 = 1 byte");
    CHECK(cat.items.at("500").fixed_bytes == 4,  "500 = 4 bytes");
    CHECK(cat.items.at("550").fixed_bytes == 1,  "550 = 1 byte");

    // I020 Extended octets
    CHECK(cat.items.at("020").octets.size() == 3, "020 has 3 octets");
    CHECK(cat.items.at("170").octets.size() == 3, "170 has 3 octets");
    CHECK(cat.items.at("270").octets.size() == 3, "270 has 3 octets");

    // I250 RepetitiveGroup: MBDATA(56)+BDS1(4)+BDS2(4) = 64 bits
    CHECK(cat.items.at("250").rep_group_bits == 64, "250 rep_group_bits = 64");
    // I280 RepetitiveGroup: DRHO(8)+DTHETA(8) = 16 bits
    CHECK(cat.items.at("280").rep_group_bits == 16, "280 rep_group_bits = 16");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode a minimal target report from raw bytes
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicTargetReport(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT10 target report ===\n";

    // UAP slots: 010(b7) 000(b6) → FSPEC 0xC0
    // Items: 010=SAC1,SIC2  000=MSGTYP1(Target Report)
    // Total: CAT(1)+LEN(2)+FSPEC(1)+010(2)+000(1) = 7 bytes
    std::vector<uint8_t> frame = {
        0x0A,             // CAT = 10
        0x00, 0x07,       // LEN = 7
        0xC0,             // FSPEC: 010+000 present, FX=0
        0x01, 0x02,       // I010: SAC=1, SIC=2
        0x01              // I000: MSGTYP=1 (Target Report)
    };

    hexdump(frame, "Basic target report input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid, "block is valid");
    CHECK(blk.records.size() == 1, "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"),  "I010 present");
    CHECK(rec.items.count("000"),  "I000 present");
    CHECK(!rec.items.count("020"), "I020 absent");

    CHECK(rec.items.at("010").fields.at("SAC")    == 1, "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC")    == 2, "I010.SIC=2");
    CHECK(rec.items.at("000").fields.at("MSGTYP") == 1, "I000.MSGTYP=1 (Target Report)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip simple Fixed items
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripSimpleFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip simple Fixed items ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=5; it.fields["SIC"]=10; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["000"]=it; }
    { DecodedItem it; it.fields["ToD"]=43200u*128u; rec.items["140"]=it; }
    { DecodedItem it; it.fields["TRK"]=0xABC; rec.items["161"]=it; }
    { DecodedItem it; it.fields["AA"]=0xABCDEF; rec.items["220"]=it; }
    { DecodedItem it; it.fields["AMPL"]=0xB4; rec.items["131"]=it; }

    auto encoded = codec.encode(10, {rec});
    hexdump(encoded, "SimpleFixed encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("SAC") == 5,          "010/SAC = 5");
    CHECK(items.at("010").fields.at("SIC") == 10,         "010/SIC = 10");
    CHECK(items.at("000").fields.at("MSGTYP") == 1,       "000/MSGTYP = 1");
    CHECK(items.at("140").fields.at("ToD") == 43200u*128u,"140/ToD round-trip");
    CHECK(items.at("161").fields.at("TRK") == 0xABC,      "161/TRK = 0xABC");
    CHECK(items.at("220").fields.at("AA")  == 0xABCDEFu,  "220/AA = 0xABCDEF");
    CHECK(items.at("131").fields.at("AMPL") == 0xB4,      "131/AMPL = 0xB4");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip Extended I020 (Target Report Descriptor, 3 octets)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtendedI020(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Extended I020 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["000"]=it; }

    // I020: all 3 octets
    DecodedItem i020;
    // Octet 1
    i020.fields["TYP"]=3; i020.fields["DCR"]=1; i020.fields["CHN"]=0;
    i020.fields["GBS"]=1; i020.fields["CRT"]=0;
    // Octet 2
    i020.fields["SIM"]=0; i020.fields["TST"]=0; i020.fields["RAB"]=0;
    i020.fields["LOP"]=1; i020.fields["TOT"]=2;
    // Octet 3
    i020.fields["SPI"]=1;
    rec.items["020"] = i020;

    auto encoded = codec.encode(10, {rec});
    hexdump(encoded, "Extended I020 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& f = blk.records[0].items.at("020").fields;
    CHECK(f.at("TYP") == 3,  "020/TYP = 3 (PSR)");
    CHECK(f.at("DCR") == 1,  "020/DCR = 1");
    CHECK(f.at("GBS") == 1,  "020/GBS = 1");
    CHECK(f.at("LOP") == 1,  "020/LOP = 1");
    CHECK(f.at("TOT") == 2,  "020/TOT = 2 (Ground vehicle)");
    CHECK(f.at("SPI") == 1,  "020/SPI = 1");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip Extended I170 (Track Status, 3 octets)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtendedI170(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Extended I170 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["000"]=it; }

    // I170: all 3 octets
    DecodedItem i170;
    // Octet 1
    i170.fields["CNF"]=0; i170.fields["TRE"]=0; i170.fields["CST"]=1;
    i170.fields["MAH"]=1; i170.fields["TCC"]=0; i170.fields["STH"]=1;
    // Octet 2
    i170.fields["TOM"]=2; i170.fields["DOU"]=3; i170.fields["MRS"]=1;
    // Octet 3
    i170.fields["GHO"]=1;
    rec.items["170"] = i170;

    auto encoded = codec.encode(10, {rec});
    hexdump(encoded, "Extended I170 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& f = blk.records[0].items.at("170").fields;
    CHECK(f.at("CNF") == 0, "170/CNF = 0 (Confirmed)");
    CHECK(f.at("CST") == 1, "170/CST = 1");
    CHECK(f.at("MAH") == 1, "170/MAH = 1");
    CHECK(f.at("STH") == 1, "170/STH = 1");
    CHECK(f.at("TOM") == 2, "170/TOM = 2 (Landing)");
    CHECK(f.at("DOU") == 3, "170/DOU = 3");
    CHECK(f.at("MRS") == 1, "170/MRS = 1");
    CHECK(f.at("GHO") == 1, "170/GHO = 1 (Ghost)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip WGS-84 position (I041) and signed Cartesian (I042, I202)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripPositions(Codec& codec) {
    std::cout << "\n=== Test: Round-trip position items ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["000"]=it; }

    // I041: WGS-84, LAT≈48.8°, LON≈2.3° (encoded as raw 32-bit signed)
    { DecodedItem it;
      it.fields["LAT"] = s32(static_cast<int32_t>(48.8 / (180.0/2147483648.0)));
      it.fields["LON"] = s32(static_cast<int32_t>(2.3  / (180.0/2147483648.0)));
      rec.items["041"]=it; }

    // I042: Cartesian X=-500m, Y=1000m
    { DecodedItem it; it.fields["X"]=s16(-500); it.fields["Y"]=1000; rec.items["042"]=it; }

    // I202: Cartesian velocity VX=-16 LSB = -1 m/s, VY=32 LSB = 2 m/s
    { DecodedItem it; it.fields["VX"]=s16(-16); it.fields["VY"]=32; rec.items["202"]=it; }

    // I210: acceleration AX=-4 = -0.25 m/s², AY=8 = 0.5 m/s²
    { DecodedItem it; it.fields["AX"]=s8(-4); it.fields["AY"]=8; rec.items["210"]=it; }

    auto encoded = codec.encode(10, {rec});
    hexdump(encoded, "Positions encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.count("041"),                            "I041 present");
    CHECK(items.at("042").fields.at("X") == s16(-500),  "042/X = -500 m");
    CHECK(items.at("042").fields.at("Y") == 1000,       "042/Y = 1000 m");
    CHECK(items.at("202").fields.at("VX") == s16(-16),  "202/VX = -16 LSB");
    CHECK(items.at("202").fields.at("VY") == 32,        "202/VY = 32 LSB");
    CHECK(items.at("210").fields.at("AX") == s8(-4),    "210/AX = -4 LSB");
    CHECK(items.at("210").fields.at("AY") == 8,         "210/AY = 8 LSB");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip RepetitiveGroup I250 and I280
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripRepetitive(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Repetitive I250 and I280 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["000"]=it; }

    // I250: two MB entries
    DecodedItem i250;
    i250.group_repetitions.push_back({{"MBDATA", 0x01020304050607ULL}, {"BDS1", 4}, {"BDS2", 0}});
    i250.group_repetitions.push_back({{"MBDATA", 0xAABBCCDDEEFF11ULL & 0x00FFFFFFFFFFFFFFULL}, {"BDS1", 5}, {"BDS2", 2}});
    rec.items["250"] = i250;

    // I280: two presence entries
    DecodedItem i280;
    i280.group_repetitions.push_back({{"DRHO", 10}, {"DTHETA", s8(-5)}});
    i280.group_repetitions.push_back({{"DRHO", s8(-20)}, {"DTHETA", 15}});
    rec.items["280"] = i280;

    auto encoded = codec.encode(10, {rec});
    hexdump(encoded, "Repetitive I250+I280 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;

    // I250
    CHECK(items.count("250"), "I250 present");
    const auto& gr250 = items.at("250").group_repetitions;
    CHECK(gr250.size() == 2,        "250 has 2 repetitions");
    CHECK(gr250[0].at("BDS1") == 4,"250 rep[0] BDS1=4");
    CHECK(gr250[0].at("BDS2") == 0,"250 rep[0] BDS2=0");
    CHECK(gr250[1].at("BDS1") == 5,"250 rep[1] BDS1=5");
    CHECK(gr250[1].at("BDS2") == 2,"250 rep[1] BDS2=2");

    // I280
    CHECK(items.count("280"), "I280 present");
    const auto& gr280 = items.at("280").group_repetitions;
    CHECK(gr280.size() == 2,              "280 has 2 repetitions");
    CHECK(gr280[0].at("DRHO")   == 10,   "280 rep[0] DRHO=10");
    CHECK(gr280[0].at("DTHETA") == s8(-5),"280 rep[0] DTHETA=-5");
    CHECK(gr280[1].at("DRHO")   == s8(-20),"280 rep[1] DRHO=-20");
    CHECK(gr280[1].at("DTHETA") == 15,   "280 rep[1] DTHETA=15");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Round-trip Extended I270 (Target Size and Orientation)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtendedI270(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Extended I270 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["000"]=it; }

    // I270: LENGTH=45m, ORIENTATION=60 LSBs, WIDTH=12m
    DecodedItem i270;
    i270.fields["LENGTH"]      = 45;
    i270.fields["ORIENTATION"] = 60;
    i270.fields["WIDTH"]       = 12;
    rec.items["270"] = i270;

    auto encoded = codec.encode(10, {rec});
    hexdump(encoded, "Extended I270 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& f = blk.records[0].items.at("270").fields;
    CHECK(f.at("LENGTH")      == 45, "270/LENGTH = 45");
    CHECK(f.at("ORIENTATION") == 60, "270/ORIENTATION = 60");
    CHECK(f.at("WIDTH")       == 12, "270/WIDTH = 12");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 9: Round-trip status and identification items
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripStatusAndIdent(Codec& codec) {
    std::cout << "\n=== Test: Round-trip status and identification items ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=3; rec.items["000"]=it; }  // Periodic status

    // I550: NOGO=1, OVL=1, TSV=0, DIV=0, TTF=1
    { DecodedItem it;
      it.fields["NOGO"]=1; it.fields["OVL"]=1; it.fields["TSV"]=0;
      it.fields["DIV"]=0;  it.fields["TTF"]=1;
      rec.items["550"]=it; }

    // I300: VFI=9 (Bus)
    { DecodedItem it; it.fields["VFI"]=9; rec.items["300"]=it; }

    // I310: TRB=1, MSG=2 (Follow me)
    { DecodedItem it; it.fields["TRB"]=1; it.fields["MSG"]=2; rec.items["310"]=it; }

    // I245: STI=0, CHR=arbitrary 48-bit pattern
    { DecodedItem it; it.fields["STI"]=0; it.fields["CHR"]=0x414243444546ULL; rec.items["245"]=it; }

    // I500: DEVX=8, DEVY=12, COVXY=signed -4
    { DecodedItem it;
      it.fields["DEVX"]=8; it.fields["DEVY"]=12;
      it.fields["COVXY"]=s16(-4);
      rec.items["500"]=it; }

    auto encoded = codec.encode(10, {rec});
    hexdump(encoded, "Status+Ident encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("550").fields.at("NOGO") == 1, "550/NOGO = 1 (Degraded)");
    CHECK(items.at("550").fields.at("OVL")  == 1, "550/OVL = 1");
    CHECK(items.at("550").fields.at("TTF")  == 1, "550/TTF = 1");
    CHECK(items.at("300").fields.at("VFI")  == 9, "300/VFI = 9 (Bus)");
    CHECK(items.at("310").fields.at("TRB")  == 1, "310/TRB = 1 (In Trouble)");
    CHECK(items.at("310").fields.at("MSG")  == 2, "310/MSG = 2 (Follow me)");
    CHECK(items.at("245").fields.at("STI")  == 0, "245/STI = 0");
    CHECK(items.at("245").fields.at("CHR")  == 0x414243444546ULL, "245/CHR round-trip");
    CHECK(items.at("500").fields.at("DEVX") == 8,      "500/DEVX = 8");
    CHECK(items.at("500").fields.at("DEVY") == 12,     "500/DEVY = 12");
    CHECK(items.at("500").fields.at("COVXY") == s16(-4),"500/COVXY = -4");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 10: Full round-trip – complete target report
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip target report ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=2; it.fields["SIC"]=5; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["000"]=it; }

    // I020 octet 1+2+3
    { DecodedItem it;
      it.fields["TYP"]=2; it.fields["DCR"]=0; it.fields["CHN"]=0;
      it.fields["GBS"]=0; it.fields["CRT"]=0;
      it.fields["SIM"]=0; it.fields["TST"]=0; it.fields["RAB"]=0;
      it.fields["LOP"]=0; it.fields["TOT"]=1;
      it.fields["SPI"]=0;
      rec.items["020"]=it; }

    // I140: 36000 s × 128
    { DecodedItem it; it.fields["ToD"]=36000u*128u; rec.items["140"]=it; }

    // I041: WGS-84
    { DecodedItem it;
      it.fields["LAT"] = s32(580000000);  // ~48° N
      it.fields["LON"] = s32(27000000);   // ~2.3° E
      rec.items["041"]=it; }

    // I040: polar RHO=1000m, TH=0x4000 (~90°)
    { DecodedItem it; it.fields["RHO"]=1000; it.fields["TH"]=0x4000; rec.items["040"]=it; }

    // I042: X=-200, Y=300
    { DecodedItem it; it.fields["X"]=s16(-200); it.fields["Y"]=300; rec.items["042"]=it; }

    // I200: GSP=0x0100, TRA=0x2000
    { DecodedItem it; it.fields["GSP"]=0x0100; it.fields["TRA"]=0x2000; rec.items["200"]=it; }

    // I161: TRK=42
    { DecodedItem it; it.fields["TRK"]=42; rec.items["161"]=it; }

    // I170: octet 1 only
    { DecodedItem it;
      it.fields["CNF"]=0; it.fields["TRE"]=0; it.fields["CST"]=0;
      it.fields["MAH"]=0; it.fields["TCC"]=0; it.fields["STH"]=0;
      rec.items["170"]=it; }

    // I060: V=0, G=0, L=0, MODE3A=0x1234 & 0xFFF = 0x234
    { DecodedItem it;
      it.fields["V"]=0; it.fields["G"]=0; it.fields["L"]=0;
      it.fields["MODE3A"]=0x234;
      rec.items["060"]=it; }

    auto encoded = codec.encode(10, {rec});
    hexdump(encoded, "Full round-trip encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("SAC") == 2,       "010/SAC = 2");
    CHECK(items.at("010").fields.at("SIC") == 5,       "010/SIC = 5");
    CHECK(items.at("000").fields.at("MSGTYP") == 1,    "000/MSGTYP = 1");
    CHECK(items.at("020").fields.at("TYP") == 2,       "020/TYP = 2 (ADS-B)");
    CHECK(items.at("020").fields.at("TOT") == 1,       "020/TOT = 1 (Aircraft)");
    CHECK(items.at("140").fields.at("ToD") == 36000u*128u, "140/ToD round-trip");
    CHECK(items.at("041").fields.at("LAT") == s32(580000000), "041/LAT round-trip");
    CHECK(items.at("040").fields.at("RHO") == 1000,    "040/RHO = 1000");
    CHECK(items.at("040").fields.at("TH")  == 0x4000,  "040/TH = 0x4000");
    CHECK(items.at("042").fields.at("X") == s16(-200), "042/X = -200");
    CHECK(items.at("042").fields.at("Y") == 300,       "042/Y = 300");
    CHECK(items.at("161").fields.at("TRK") == 42,      "161/TRK = 42");
    CHECK(items.at("060").fields.at("MODE3A") == 0x234,"060/MODE3A = 0x234");
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = (argc > 1) ? argv[1] : "specs/CAT10.xml";

    Codec codec;
    testSpecLoad(codec, spec_path);
    testDecodeBasicTargetReport(codec);
    testRoundTripSimpleFixed(codec);
    testRoundTripExtendedI020(codec);
    testRoundTripExtendedI170(codec);
    testRoundTripPositions(codec);
    testRoundTripRepetitive(codec);
    testRoundTripExtendedI270(codec);
    testRoundTripStatusAndIdent(codec);
    testFullRoundTrip(codec);

    std::cout << "\n=== Results: "
              << (failures == 0 ? "ALL PASSED" : std::to_string(failures) + " FAILED")
              << " ===\n";
    return failures == 0 ? 0 : 1;
}
