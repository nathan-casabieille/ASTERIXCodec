// test_cat16.cpp – Tests for CAT16 Data Link Flight Messages decode/encode round-trip.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat16

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
            std::cerr << "FAIL [RT] " << p << " group_repetitions count mismatch ("
                      << src.group_repetitions.size() << " vs "
                      << dst.group_repetitions.size() << ")\n";
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
//  Test 1: XML spec loads, item types and UAP are correct
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT16 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 16,         "cat number = 16");
    CHECK(cat.edition == "1.0",  "edition = 1.0");

    for (auto id : {"000","010","020","030","040","050","060","070","080","090","100",
                    "110","120","130","140","150","160","170","180","190","200",
                    "210","220","230","240","250","260","RE","SP"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 29, "29 items total");

    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    CHECK(cat.uap_variations.at("default").size() == 35, "UAP has 35 slots");

    const auto& uap = cat.uap_variations.at("default");
    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "000", "UAP slot  2 = 000");
    CHECK(uap[6]  == "060", "UAP slot  7 = 060");
    CHECK(uap[7]  == "070", "UAP slot  8 = 070");
    CHECK(uap[14] == "140", "UAP slot 15 = 140");
    CHECK(uap[21] == "210", "UAP slot 22 = 210");
    CHECK(uap[24] == "240", "UAP slot 25 = 240");
    CHECK(uap[27] == "-",   "UAP slot 28 = - (unused)");
    CHECK(uap[31] == "RE",  "UAP slot 32 = RE");
    CHECK(uap[32] == "SP",  "UAP slot 33 = SP");
    CHECK(uap[34] == "-",   "UAP slot 35 = - (unused)");

    // Item types
    CHECK(cat.items.at("000").type == ItemType::Fixed,           "000 is Fixed");
    CHECK(cat.items.at("010").type == ItemType::Fixed,           "010 is Fixed");
    CHECK(cat.items.at("240").type == ItemType::Compound,        "240 is Compound");
    CHECK(cat.items.at("250").type == ItemType::RepetitiveGroup, "250 is RepetitiveGroup");
    CHECK(cat.items.at("RE").type  == ItemType::SP,              "RE is SP/Explicit");
    CHECK(cat.items.at("SP").type  == ItemType::SP,              "SP is SP/Explicit");

    // Fixed byte sizes
    CHECK(cat.items.at("000").fixed_bytes == 1,  "000 = 1 byte");
    CHECK(cat.items.at("010").fixed_bytes == 2,  "010 = 2 bytes");
    CHECK(cat.items.at("020").fixed_bytes == 3,  "020 = 3 bytes");
    CHECK(cat.items.at("030").fixed_bytes == 3,  "030 = 3 bytes");
    CHECK(cat.items.at("040").fixed_bytes == 2,  "040 = 2 bytes");
    CHECK(cat.items.at("050").fixed_bytes == 5,  "050 = 5 bytes");
    CHECK(cat.items.at("060").fixed_bytes == 6,  "060 = 6 bytes");
    CHECK(cat.items.at("070").fixed_bytes == 4,  "070 = 4 bytes");
    CHECK(cat.items.at("080").fixed_bytes == 1,  "080 = 1 byte");
    CHECK(cat.items.at("090").fixed_bytes == 4,  "090 = 4 bytes");
    CHECK(cat.items.at("110").fixed_bytes == 2,  "110 = 2 bytes");
    CHECK(cat.items.at("140").fixed_bytes == 6,  "140 = 6 bytes");
    CHECK(cat.items.at("160").fixed_bytes == 2,  "160 = 2 bytes");
    CHECK(cat.items.at("170").fixed_bytes == 2,  "170 = 2 bytes");
    CHECK(cat.items.at("180").fixed_bytes == 2,  "180 = 2 bytes");
    CHECK(cat.items.at("190").fixed_bytes == 1,  "190 = 1 byte");
    CHECK(cat.items.at("200").fixed_bytes == 1,  "200 = 1 byte");
    CHECK(cat.items.at("210").fixed_bytes == 7,  "210 = 7 bytes");
    CHECK(cat.items.at("220").fixed_bytes == 3,  "220 = 3 bytes");
    CHECK(cat.items.at("230").fixed_bytes == 2,  "230 = 2 bytes");
    CHECK(cat.items.at("260").fixed_bytes == 7,  "260 = 7 bytes");

    // I240 Compound sub-item count
    CHECK(cat.items.at("240").compound_sub_items.size() == 14, "240 has 14 sub-items");

    // I250 RepetitiveGroup
    const auto& i250 = cat.items.at("250");
    CHECK(i250.rep_group_bits == 64,             "250 rep_group_bits = 64 (8 bytes)");
    CHECK(i250.rep_group_elements.size() == 3,   "250 has 3 group elements");
    CHECK(i250.rep_group_elements[0].name == "MBDATA", "250 element[0] = MBDATA");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode basic flight message
//          I010(SAC=1,SIC=2) + I000(MSGTYP=1) + I040(TN=1234)
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicMessage(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT16 flight message ===\n";

    // FSPEC byte 1: bit7=1(I010), bit6=1(I000), bit5=0, bit4=0, bit3=1(I040), bit2=0, bit1=0, FX=0
    //              = 1100_1000 = 0xC8
    // I010: SAC=0x01, SIC=0x02
    // I000: MSGTYP=1
    // I040: TN=1234=0x04D2

    std::vector<uint8_t> frame = {
        0x10,             // CAT = 16
        0x00, 0x09,       // LEN = 9
        0xC8,             // FSPEC byte 1 (FX=0)
        0x01, 0x02,       // I010 SAC=1, SIC=2
        0x01,             // I000 MSGTYP=1
        0x04, 0xD2        // I040 TN=1234
    };

    hexdump(frame, "Basic flight msg input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid, "block is valid");
    CHECK(blk.records.size() == 1, "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"), "I010 present");
    CHECK(rec.items.count("000"), "I000 present");
    CHECK(rec.items.count("040"), "I040 present");

    CHECK(rec.items.at("010").fields.at("SAC") == 1,    "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC") == 2,    "I010.SIC=2");
    CHECK(rec.items.at("000").fields.at("MSGTYP") == 1, "I000.MSGTYP=1");
    CHECK(rec.items.at("040").fields.at("TN") == 1234,  "I040.TN=1234");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip simple fixed items (I000, I020, I030, I040, I080, I150)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripSimpleFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip simple Fixed items ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=5; it.fields["SIC"]=10; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=3; enc_rec.items["000"]=it; }
    { DecodedItem it; it.fields["FMN"]=0xABCDE; enc_rec.items["020"]=it; }
    { DecodedItem it; it.fields["TOD"]=0x200000; enc_rec.items["030"]=it; }
    { DecodedItem it; it.fields["TN"]=4096; enc_rec.items["040"]=it; }
    { DecodedItem it; it.fields["WTC"]=77; enc_rec.items["080"]=it; } // Medium
    { DecodedItem it; it.fields["CPT"]=1; enc_rec.items["150"]=it; }  // Waypoint

    std::vector<uint8_t> encoded = codec.encode(16, {enc_rec});
    hexdump(encoded, "Simple Fixed encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "RT-simple");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip I050 (GFIDC + spare + ID group)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripIFPSFlightID(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I050 IFPS Flight ID ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i050;
    i050.fields["GFIDC"] = 2;          // FI created by IFPS
    i050.fields["ID"]    = 0xDEADBEEF; // 32-bit flight ID
    enc_rec.items["050"] = i050;

    std::vector<uint8_t> encoded = codec.encode(16, {enc_rec});
    hexdump(encoded, "I050 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "RT-050");
    CHECK(blk.records[0].items.at("050").fields.at("GFIDC") == 2,           "I050.GFIDC=2");
    CHECK(blk.records[0].items.at("050").fields.at("ID")    == 0xDEADBEEFu, "I050.ID=0xDEADBEEF");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip raw string items (I060, I070, I090, I100, I140)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripRawStrings(Codec& codec) {
    std::cout << "\n=== Test: Round-trip raw icao6str items ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    // I060: 48-bit aircraft identification (e.g. "AFR001 " in 6-bit ICAO = some raw value)
    { DecodedItem it; it.fields["AI"]  = 0x001122334455ULL; enc_rec.items["060"]=it; }
    // I070: 32-bit aircraft type
    { DecodedItem it; it.fields["TYP"] = 0xA1B2C3D4; enc_rec.items["070"]=it; }
    // I090: 32-bit departure airport
    { DecodedItem it; it.fields["DEP"] = 0x11223344; enc_rec.items["090"]=it; }
    // I100: 32-bit destination airport
    { DecodedItem it; it.fields["DES"] = 0x55667788; enc_rec.items["100"]=it; }
    // I140: 48-bit coordination point
    { DecodedItem it; it.fields["CP"]  = 0xAABBCCDDEEFFULL; enc_rec.items["140"]=it; }

    std::vector<uint8_t> encoded = codec.encode(16, {enc_rec});
    hexdump(encoded, "Raw strings encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "RT-strings");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip flight level and signed speed items
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripFlightLevels(Codec& codec) {
    std::cout << "\n=== Test: Round-trip flight level items ===\n";

    // I110/120/130: FL100 = 100 * 4 = 400 raw (LSB=0.25 FL)
    // I160: IM=0 (IAS), SPEED=250 kt raw
    // I170: +5 FL/min raw = 5; -3 FL/min raw = uint16(-3) = 65533

    const uint64_t cflr_neg = static_cast<uint64_t>(static_cast<uint16_t>(static_cast<int16_t>(-3)));

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["CFL"]=400; enc_rec.items["110"]=it; } // FL100
    { DecodedItem it; it.fields["OFL"]=360; enc_rec.items["120"]=it; } // FL90
    { DecodedItem it; it.fields["ICL"]=320; enc_rec.items["130"]=it; } // FL80
    { DecodedItem it; it.fields["IM"]=0; it.fields["SPEED"]=250; enc_rec.items["160"]=it; }
    { DecodedItem it; it.fields["CFLR"]=cflr_neg; enc_rec.items["170"]=it; }

    std::vector<uint8_t> encoded = codec.encode(16, {enc_rec});
    hexdump(encoded, "FL items encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "RT-FL");

    CHECK(blk.records[0].items.at("110").fields.at("CFL") == 400,      "I110.CFL=400 raw");
    CHECK(blk.records[0].items.at("170").fields.at("CFLR") == cflr_neg,"I170.CFLR=-3 raw");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip transponder/comm items (I180, I190, I200, I210, I220, I230, I260)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripTransponder(Codec& codec) {
    std::cout << "\n=== Test: Round-trip transponder items ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    // I180: V=0, G=0, L=1, spare=0, MODE3A=0o1234=668
    { DecodedItem it; it.fields["V"]=0; it.fields["G"]=0; it.fields["L"]=1; it.fields["MODE3A"]=668; enc_rec.items["180"]=it; }
    // I190: COM=1
    { DecodedItem it; it.fields["COM"]=1; enc_rec.items["190"]=it; }
    // I200: COM=2, STAT=1
    { DecodedItem it; it.fields["COM"]=2; it.fields["STAT"]=1; enc_rec.items["200"]=it; }
    // I210: 56-bit RA raw
    { DecodedItem it; it.fields["RA"]=0x00AABBCCDDEE11ULL; enc_rec.items["210"]=it; }
    // I220: 24-bit aircraft address
    { DecodedItem it; it.fields["ADR"]=0x4840D6; enc_rec.items["220"]=it; }
    // I230: 16-bit capabilities
    { DecodedItem it; it.fields["CAP"]=0xBEEF; enc_rec.items["230"]=it; }
    // I260: 56-bit long RA
    { DecodedItem it; it.fields["RA"]=0x00112233445566ULL; enc_rec.items["260"]=it; }

    std::vector<uint8_t> encoded = codec.encode(16, {enc_rec});
    hexdump(encoded, "Transponder encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "RT-xpdr");

    CHECK(blk.records[0].items.at("180").fields.at("MODE3A") == 668,          "I180.MODE3A=668");
    CHECK(blk.records[0].items.at("220").fields.at("ADR")    == 0x4840D6u,    "I220.ADR=0x4840D6");
    CHECK(blk.records[0].items.at("260").fields.at("RA")     == 0x00112233445566ULL, "I260.RA round-trip");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Round-trip I250 Mode S BDS Register Data (2 entries)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripBDSData(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I250 Mode S BDS Register Data ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i250;
    // Entry 0: MBDATA=56-bit value, BDS1=3, BDS2=7
    i250.group_repetitions.push_back({
        {"MBDATA", 0x00112233445566ULL},
        {"BDS1", 3},
        {"BDS2", 7}
    });
    // Entry 1: MBDATA=different value, BDS1=5, BDS2=8 (max 4-bit = 15)
    i250.group_repetitions.push_back({
        {"MBDATA", 0xAABBCCDDEEFF00ULL},
        {"BDS1", 5},
        {"BDS2", 15}
    });
    enc_rec.items["250"] = i250;

    std::vector<uint8_t> encoded = codec.encode(16, {enc_rec});
    hexdump(encoded, "I250 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("250"), "I250 present");
    const auto& groups = blk.records[0].items.at("250").group_repetitions;
    CHECK(groups.size() == 2, "I250: 2 groups");

    if (groups.size() == 2) {
        CHECK(groups[0].at("MBDATA") == 0x00112233445566ULL, "I250[0].MBDATA");
        CHECK(groups[0].at("BDS1")   == 3,                   "I250[0].BDS1=3");
        CHECK(groups[0].at("BDS2")   == 7,                   "I250[0].BDS2=7");
        CHECK(groups[1].at("MBDATA") == 0xAABBCCDDEEFF00ULL, "I250[1].MBDATA");
        CHECK(groups[1].at("BDS1")   == 5,                   "I250[1].BDS1=5");
        CHECK(groups[1].at("BDS2")   == 15,                  "I250[1].BDS2=15");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 9: Round-trip I240 Aircraft Derived Data (Compound, 2 PSF bytes)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripAircraftDerivedData(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I240 Aircraft Derived Data ===\n";

    // BPS = 1013.0 mb → raw = 1013 / 0.1 = 10130 = 0x278A
    // VPR = +1000 ft/min → raw = 1000 / 6.25 = 160 = 0x00A0
    // TAS = 450 kt → raw = 450
    // SAL: SAS=1, SOURCE=2 (MCP/FCU), ALT=FL350 → 35000ft / 25 = 1400 raw (13-bit)
    // TIS: NAC=10, NIC=9
    // MES (PSF byte 2): SUM=0xAA, POM=0x55, GAO=0x0F, SGV=0x1234, STA=0x01, TNH=0x5678, MES2=0xBE

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i240;
    i240.compound_sub_fields["BPS"]["BPS"] = 10130;
    i240.compound_sub_fields["VPR"]["VPR"] = 160;
    i240.compound_sub_fields["TAS"]["TAS"] = 450;
    i240.compound_sub_fields["SAL"]["SAS"]    = 1;
    i240.compound_sub_fields["SAL"]["SOURCE"] = 2;
    i240.compound_sub_fields["SAL"]["ALT"]    = 1400; // +35000ft raw (13-bit)
    i240.compound_sub_fields["TIS"]["NAC"] = 10;
    i240.compound_sub_fields["TIS"]["NIC"] = 9;
    // MES is in PSF byte 2 — encoder must set FX=1 in PSF byte 1
    i240.compound_sub_fields["MES"]["SUM"]  = 0xAA;
    i240.compound_sub_fields["MES"]["POM"]  = 0x55;
    i240.compound_sub_fields["MES"]["GAO"]  = 0x0F;
    i240.compound_sub_fields["MES"]["SGV"]  = 0x1234;
    i240.compound_sub_fields["MES"]["STA"]  = 0x01;
    i240.compound_sub_fields["MES"]["TNH"]  = 0x5678;
    i240.compound_sub_fields["MES"]["MES2"] = 0xBE;
    enc_rec.items["240"] = i240;

    std::vector<uint8_t> encoded = codec.encode(16, {enc_rec});
    hexdump(encoded, "I240 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("240"), "I240 present");
    checkItemsMatch(blk.records[0].items, enc_rec.items, "RT-240");

    // Spot-check specific decoded values
    const auto& csf = blk.records[0].items.at("240").compound_sub_fields;
    CHECK(csf.count("BPS"),             "I240/BPS present");
    CHECK(csf.count("VPR"),             "I240/VPR present");
    CHECK(csf.count("TIS"),             "I240/TIS present");
    CHECK(csf.count("MES"),             "I240/MES present (PSF byte 2)");
    if (csf.count("BPS")) CHECK(csf.at("BPS").at("BPS") == 10130,       "I240/BPS=10130");
    if (csf.count("VPR")) CHECK(csf.at("VPR").at("VPR") == 160,         "I240/VPR=160");
    if (csf.count("TIS")) CHECK(csf.at("TIS").at("NAC") == 10,          "I240/TIS.NAC=10");
    if (csf.count("MES")) CHECK(csf.at("MES").at("SUM") == 0xAAu,       "I240/MES.SUM=0xAA");
    if (csf.count("MES")) CHECK(csf.at("MES").at("SGV") == 0x1234u,     "I240/MES.SGV=0x1234");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 10: Round-trip I240 with only PSF byte 1 sub-items (no MES)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCompound240PSF1Only(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I240 (PSF byte 1 only, no MES) ===\n";

    // FSS: MV=1, AH=0, AM=1, ALT=-1000ft → raw = -1000/25 = -40 → 13-bit = 8192-40 = 8152
    const uint64_t fss_alt = static_cast<uint64_t>(static_cast<uint16_t>(static_cast<int16_t>(-40)) & 0x1FFF);

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i240;
    i240.compound_sub_fields["FSS"]["MV"]  = 1;
    i240.compound_sub_fields["FSS"]["AH"]  = 0;
    i240.compound_sub_fields["FSS"]["AM"]  = 1;
    i240.compound_sub_fields["FSS"]["ALT"] = fss_alt;
    enc_rec.items["240"] = i240;

    std::vector<uint8_t> encoded = codec.encode(16, {enc_rec});
    hexdump(encoded, "I240-PSF1 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& csf = blk.records[0].items.at("240").compound_sub_fields;
    CHECK(csf.count("FSS"),                                 "I240/FSS present");
    CHECK(!csf.count("MES"),                                "I240/MES absent (no second PSF byte)");
    if (csf.count("FSS")) {
        CHECK(csf.at("FSS").at("MV")  == 1,       "I240/FSS.MV=1");
        CHECK(csf.at("FSS").at("AM")  == 1,       "I240/FSS.AM=1");
        CHECK(csf.at("FSS").at("ALT") == fss_alt, "I240/FSS.ALT=-40 raw");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 11: Full round-trip with many items
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip CAT16 ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; enc_rec.items["000"]=it; }
    { DecodedItem it; it.fields["FMN"]=42; enc_rec.items["020"]=it; }
    { DecodedItem it; it.fields["TOD"]=0x400000; enc_rec.items["030"]=it; }
    { DecodedItem it; it.fields["TN"]=9999; enc_rec.items["040"]=it; }
    { DecodedItem it; it.fields["GFIDC"]=1; it.fields["ID"]=0x12345678; enc_rec.items["050"]=it; }
    { DecodedItem it; it.fields["AI"]=0xAABBCCDDEEFFULL; enc_rec.items["060"]=it; }
    { DecodedItem it; it.fields["WTC"]=72; enc_rec.items["080"]=it; } // Heavy
    { DecodedItem it; it.fields["DEP"]=0x11223344; enc_rec.items["090"]=it; }
    { DecodedItem it; it.fields["DES"]=0x55667788; enc_rec.items["100"]=it; }
    { DecodedItem it; it.fields["CFL"]=400; enc_rec.items["110"]=it; }
    { DecodedItem it; it.fields["V"]=0; it.fields["G"]=0; it.fields["L"]=0; it.fields["MODE3A"]=07700; enc_rec.items["180"]=it; }
    { DecodedItem it; it.fields["ADR"]=0x4840D6; enc_rec.items["220"]=it; }
    {
        DecodedItem it;
        it.group_repetitions.push_back({{"MBDATA", 0xCAFEBABEDEADULL}, {"BDS1", 4}, {"BDS2", 8}});
        enc_rec.items["250"] = it;
    }

    std::vector<uint8_t> encoded = codec.encode(16, {enc_rec});
    hexdump(encoded, "Full RT encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "full-RT");
    CHECK(blk.records[0].items.count("010"), "I010 (mandatory) present");
}

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = "specs/CAT16.xml";
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
    testRoundTripIFPSFlightID(codec);
    testRoundTripRawStrings(codec);
    testRoundTripFlightLevels(codec);
    testRoundTripTransponder(codec);
    testRoundTripBDSData(codec);
    testRoundTripAircraftDerivedData(codec);
    testRoundTripCompound240PSF1Only(codec);
    testFullRoundTrip(codec);

    std::cout << "\n============================\n";
    if (failures == 0)
        std::cout << "ALL TESTS PASSED\n";
    else
        std::cout << failures << " TEST(S) FAILED\n";
    std::cout << "============================\n";

    return failures ? 1 : 0;
}
