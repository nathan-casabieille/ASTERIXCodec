// test_cat150.cpp – Tests for CAT150 MADAP Plan Server - Flight Data Message, Ed. 3.0.

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

static uint64_t s16(int16_t v) { return static_cast<uint64_t>(static_cast<uint16_t>(v)); }
static uint64_t s24(int32_t v) { return static_cast<uint64_t>(static_cast<uint32_t>(v) & 0xFFFFFF); }

// ─────────────────────────────────────────────────────────────────────────────
//  Test 1: Spec load
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT150 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 150,       "cat number = 150");
    CHECK(cat.edition == "3.0", "edition = 3.0");

    for (auto id : {"010","020","030","040","050","060","070","080","090","100",
                    "110","120","130","140","150","151","160","170","171","180",
                    "190","200","210","220","230","240","250","251"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 28, "28 items total");

    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    const auto& uap = cat.uap_variations.at("default");
    CHECK(uap.size() == 28, "UAP has 28 slots");

    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "020", "UAP slot  2 = 020");
    CHECK(uap[2]  == "030", "UAP slot  3 = 030");
    CHECK(uap[6]  == "070", "UAP slot  7 = 070");
    CHECK(uap[13] == "140", "UAP slot 14 = 140");
    CHECK(uap[14] == "150", "UAP slot 15 = 150");
    CHECK(uap[20] == "210", "UAP slot 21 = 210");
    CHECK(uap[25] == "251", "UAP slot 26 = 251");
    CHECK(uap[26] == "171", "UAP slot 27 = 171");
    CHECK(uap[27] == "151", "UAP slot 28 = 151");

    // Item types
    for (auto id : {"010","020","030","040","050","060","070","080","090",
                    "100","110","120","130","190","210","220","230"}) {
        CHECK(cat.items.at(id).type == ItemType::Fixed,
              std::string(id) + " is Fixed");
    }
    for (auto id : {"140","150","151","160","170","171","180","200","240","250","251"}) {
        CHECK(cat.items.at(id).type == ItemType::RepetitiveGroup,
              std::string(id) + " is RepetitiveGroup");
    }

    // Fixed sizes
    CHECK(cat.items.at("010").fixed_bytes == 2,  "010 = 2 bytes");
    CHECK(cat.items.at("020").fixed_bytes == 2,  "020 = 2 bytes");
    CHECK(cat.items.at("030").fixed_bytes == 1,  "030 = 1 byte");
    CHECK(cat.items.at("040").fixed_bytes == 2,  "040 = 2 bytes");
    CHECK(cat.items.at("050").fixed_bytes == 7,  "050 = 7 bytes");
    CHECK(cat.items.at("060").fixed_bytes == 4,  "060 = 4 bytes");
    CHECK(cat.items.at("070").fixed_bytes == 4,  "070 = 4 bytes");
    CHECK(cat.items.at("080").fixed_bytes == 4,  "080 = 4 bytes");
    CHECK(cat.items.at("090").fixed_bytes == 4,  "090 = 4 bytes");
    CHECK(cat.items.at("100").fixed_bytes == 1,  "100 = 1 byte");
    CHECK(cat.items.at("110").fixed_bytes == 1,  "110 = 1 byte");
    CHECK(cat.items.at("120").fixed_bytes == 7,  "120 = 7 bytes");
    CHECK(cat.items.at("130").fixed_bytes == 3,  "130 = 3 bytes");
    CHECK(cat.items.at("190").fixed_bytes == 2,  "190 = 2 bytes");
    CHECK(cat.items.at("210").fixed_bytes == 2,  "210 = 2 bytes");
    CHECK(cat.items.at("220").fixed_bytes == 2,  "220 = 2 bytes");
    CHECK(cat.items.at("230").fixed_bytes == 2,  "230 = 2 bytes");

    // RepetitiveGroup bits per entry
    CHECK(cat.items.at("140").rep_group_bits == 96, "140 rep_group_bits = 96");
    CHECK(cat.items.at("150").rep_group_bits == 32, "150 rep_group_bits = 32");
    CHECK(cat.items.at("151").rep_group_bits == 48, "151 rep_group_bits = 48");
    CHECK(cat.items.at("160").rep_group_bits == 32, "160 rep_group_bits = 32");
    CHECK(cat.items.at("170").rep_group_bits == 24, "170 rep_group_bits = 24");
    CHECK(cat.items.at("171").rep_group_bits == 24, "171 rep_group_bits = 24");
    CHECK(cat.items.at("180").rep_group_bits == 32, "180 rep_group_bits = 32");
    CHECK(cat.items.at("200").rep_group_bits == 8,  "200 rep_group_bits = 8");
    CHECK(cat.items.at("240").rep_group_bits == 32, "240 rep_group_bits = 32");
    CHECK(cat.items.at("250").rep_group_bits == 16, "250 rep_group_bits = 16");
    CHECK(cat.items.at("251").rep_group_bits == 32, "251 rep_group_bits = 32");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode a minimal message from raw bytes
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicMessage(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT150 message ===\n";

    // FSPEC byte 1: I010(bit7)+I020(bit6)+I030(bit5) → 1110_0000 = 0xE0, FX=0
    // Frame: CAT(1)+LEN(2)+FSPEC(1)+I010(2)+I020(2)+I030(1) = 9 bytes
    std::vector<uint8_t> frame = {
        0x96,             // CAT = 150
        0x00, 0x09,       // LEN = 9
        0xE0,             // FSPEC: I010+I020+I030, FX=0
        0x01, 0x02,       // I010: CEN=1, POS=2
        0x03, 0x04,       // I020: CEN=3, POS=4
        0x01              // I030: MT=1 (flight plan creation)
    };

    hexdump(frame, "Basic message input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid,                "block is valid");
    CHECK(blk.records.size() == 1,  "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"),  "I010 present");
    CHECK(rec.items.count("020"),  "I020 present");
    CHECK(rec.items.count("030"),  "I030 present");
    CHECK(!rec.items.count("040"), "I040 absent");
    CHECK(!rec.items.count("050"), "I050 absent");

    CHECK(rec.items.at("010").fields.at("CEN") == 1, "I010.CEN=1");
    CHECK(rec.items.at("010").fields.at("POS") == 2, "I010.POS=2");
    CHECK(rec.items.at("020").fields.at("CEN") == 3, "I020.CEN=3");
    CHECK(rec.items.at("020").fields.at("POS") == 4, "I020.POS=4");
    CHECK(rec.items.at("030").fields.at("MT")  == 1, "I030.MT=1 (flight plan creation)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip core Fixed items
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCoreFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip core Fixed items ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["CEN"]=1; it.fields["POS"]=0; rec.items["010"]=it; }
    { DecodedItem it; it.fields["CEN"]=3; it.fields["POS"]=1; rec.items["020"]=it; }
    { DecodedItem it; it.fields["MT"]=1; rec.items["030"]=it; }    // flight plan creation
    { DecodedItem it; it.fields["PRN"]=1234; rec.items["040"]=it; }
    { DecodedItem it; it.fields["GAT"]=1; it.fields["OAT"]=0; it.fields["CPL"]=1; it.fields["SPN"]=0; rec.items["100"]=it; }
    { DecodedItem it; it.fields["HLD"]=0; it.fields["RVQ"]=1; it.fields["RVC"]=1; it.fields["RVX"]=0; rec.items["110"]=it; }
    { DecodedItem it; it.fields["CTN"]=500; rec.items["210"]=it; }
    { DecodedItem it; it.fields["MPC"]=302; rec.items["220"]=it; }
    { DecodedItem it; it.fields["NOP"]=150; rec.items["230"]=it; }

    auto encoded = codec.encode(150, {rec});
    hexdump(encoded, "CoreFixed encoded");
    CHECK(!encoded.empty(), "encode produced output");
    CHECK(encoded[0] == 0x96, "CAT byte = 0x96 (150)");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("CEN") == 1,    "010/CEN = 1");
    CHECK(items.at("010").fields.at("POS") == 0,    "010/POS = 0 (broadcast)");
    CHECK(items.at("020").fields.at("CEN") == 3,    "020/CEN = 3");
    CHECK(items.at("030").fields.at("MT")  == 1,    "030/MT = 1 (creation)");
    CHECK(items.at("040").fields.at("PRN") == 1234, "040/PRN = 1234");
    CHECK(items.at("100").fields.at("GAT") == 1,    "100/GAT = 1");
    CHECK(items.at("100").fields.at("CPL") == 1,    "100/CPL = 1 (complete plan)");
    CHECK(items.at("100").fields.at("SPN") == 0,    "100/SPN = 0");
    CHECK(items.at("110").fields.at("HLD") == 0,    "110/HLD = 0");
    CHECK(items.at("110").fields.at("RVQ") == 1,    "110/RVQ = 1 (RVSM equipped)");
    CHECK(items.at("110").fields.at("RVC") == 1,    "110/RVC = 1 (RVSM capable)");
    CHECK(items.at("210").fields.at("CTN") == 500,  "210/CTN = 500");
    CHECK(items.at("220").fields.at("MPC") == 302,  "220/MPC = 302");
    CHECK(items.at("230").fields.at("NOP") == 150,  "230/NOP = 150");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip Fixed ASCII-string items
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripAsciiFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Fixed ASCII-string items ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["CEN"]=1; it.fields["POS"]=0; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MT"]=1; rec.items["030"]=it; }

    // I050: Callsign "AFR123 " (7 ASCII bytes = 56 bits)
    // A=0x41 F=0x46 R=0x52 1=0x31 2=0x32 3=0x33 ' '=0x20
    { DecodedItem it; it.fields["CS"]=0x41465231323320ULL; rec.items["050"]=it; }

    // I060: Present Mode 3A "7070" (4 ASCII bytes = 32 bits)
    // 7=0x37 0=0x30 7=0x37 0=0x30
    { DecodedItem it; it.fields["M3A"]=0x37303730ULL; rec.items["060"]=it; }

    // I080: Departure "LFPG" (Paris CDG)
    // L=0x4C F=0x46 P=0x50 G=0x47
    { DecodedItem it; it.fields["DEP"]=0x4C465047ULL; rec.items["080"]=it; }

    // I090: Destination "EHAM" (Amsterdam Schiphol)
    // E=0x45 H=0x48 A=0x41 M=0x4D
    { DecodedItem it; it.fields["DEST"]=0x4548414DULL; rec.items["090"]=it; }

    // I120: Aircraft Type – NOA="01"(16b), TOA="B738"(32b), WT="M"(8b)
    { DecodedItem it;
      it.fields["NOA"]=0x3031ULL;       // "01"
      it.fields["TOA"]=0x42373338ULL;   // "B738"
      it.fields["WT"] =0x4DULL;         // "M"
      rec.items["120"]=it; }

    // I130: Cleared FL "350" (3 ASCII bytes = 24 bits)
    // 3=0x33 5=0x35 0=0x30
    { DecodedItem it; it.fields["CFL"]=0x333530ULL; rec.items["130"]=it; }

    // I190: Controller ID "AB" (2 ASCII bytes = 16 bits)
    // A=0x41 B=0x42
    { DecodedItem it; it.fields["CID"]=0x4142ULL; rec.items["190"]=it; }

    auto encoded = codec.encode(150, {rec});
    hexdump(encoded, "AsciiFixed encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("050").fields.at("CS")   == 0x41465231323320ULL, "050/CS = 'AFR123 '");
    CHECK(items.at("060").fields.at("M3A")  == 0x37303730ULL,       "060/M3A = '7070'");
    CHECK(items.at("080").fields.at("DEP")  == 0x4C465047ULL,       "080/DEP = 'LFPG'");
    CHECK(items.at("090").fields.at("DEST") == 0x4548414DULL,       "090/DEST = 'EHAM'");
    CHECK(items.at("120").fields.at("NOA")  == 0x3031ULL,           "120/NOA = '01'");
    CHECK(items.at("120").fields.at("TOA")  == 0x42373338ULL,       "120/TOA = 'B738'");
    CHECK(items.at("120").fields.at("WT")   == 0x4DULL,             "120/WT = 'M'");
    CHECK(items.at("130").fields.at("CFL")  == 0x333530ULL,         "130/CFL = '350'");
    CHECK(items.at("190").fields.at("CID")  == 0x4142ULL,           "190/CID = 'AB'");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip route coordinate RepetitiveGroups (I150, I151, I160)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripRouteCoords(Codec& codec) {
    std::cout << "\n=== Test: Round-trip route coordinate RepetitiveGroups ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["CEN"]=1; it.fields["POS"]=0; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MT"]=1; rec.items["030"]=it; }

    // I150: 2 Cartesian route points (X/Y signed 16-bit, 1/64 NM/LSB)
    DecodedItem i150;
    i150.group_repetitions.push_back({{"X", s16(100)},  {"Y", s16(-200)}});
    i150.group_repetitions.push_back({{"X", s16(-50)},  {"Y", s16(75)}});
    rec.items["150"] = i150;

    // I151: 2 WGS-84 route points (LAT/LON signed 24-bit, 180/2^23 deg/LSB)
    // s24(2097152) → LAT = 2097152 × 180/2^23 = 45.0°
    // s24(1048576) → LON = 1048576 × 180/2^23 = 22.5°
    DecodedItem i151;
    i151.group_repetitions.push_back({{"LAT", s24(2097152)}, {"LON", s24(1048576)}});
    i151.group_repetitions.push_back({{"LAT", s24(1048576)}, {"LON", s24(524288)}});
    rec.items["151"] = i151;

    // I160: 2 time-over-waypoint entries (HH/MM as 16-bit ASCII each)
    // "13"=0x3133 "30"=0x3330
    DecodedItem i160;
    i160.group_repetitions.push_back({{"HH", 0x3133u}, {"MM", 0x3330u}});  // 13:30
    i160.group_repetitions.push_back({{"HH", 0x3134u}, {"MM", 0x3030u}});  // 14:00
    rec.items["160"] = i160;

    auto encoded = codec.encode(150, {rec});
    hexdump(encoded, "RouteCoords encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;

    // I150
    CHECK(items.count("150"), "I150 present");
    const auto& gr150 = items.at("150").group_repetitions;
    CHECK(gr150.size() == 2,              "150 has 2 entries");
    CHECK(gr150[0].at("X") == s16(100),   "150 rep[0] X = 100");
    CHECK(gr150[0].at("Y") == s16(-200),  "150 rep[0] Y = -200");
    CHECK(gr150[1].at("X") == s16(-50),   "150 rep[1] X = -50");
    CHECK(gr150[1].at("Y") == s16(75),    "150 rep[1] Y = 75");

    // I151
    CHECK(items.count("151"), "I151 present");
    const auto& gr151 = items.at("151").group_repetitions;
    CHECK(gr151.size() == 2,                   "151 has 2 entries");
    CHECK(gr151[0].at("LAT") == s24(2097152),  "151 rep[0] LAT = 45°");
    CHECK(gr151[0].at("LON") == s24(1048576),  "151 rep[0] LON = 22.5°");
    CHECK(gr151[1].at("LAT") == s24(1048576),  "151 rep[1] LAT round-trip");
    CHECK(gr151[1].at("LON") == s24(524288),   "151 rep[1] LON round-trip");

    // I160
    CHECK(items.count("160"), "I160 present");
    const auto& gr160 = items.at("160").group_repetitions;
    CHECK(gr160.size() == 2,               "160 has 2 entries");
    CHECK(gr160[0].at("HH") == 0x3133u,   "160 rep[0] HH = '13'");
    CHECK(gr160[0].at("MM") == 0x3330u,   "160 rep[0] MM = '30'");
    CHECK(gr160[1].at("HH") == 0x3134u,   "160 rep[1] HH = '14'");
    CHECK(gr160[1].at("MM") == 0x3030u,   "160 rep[1] MM = '00'");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip route misc RepetitiveGroups (I170, I171, I180, I200)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripRouteMisc(Codec& codec) {
    std::cout << "\n=== Test: Round-trip route misc RepetitiveGroups ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["CEN"]=1; it.fields["POS"]=0; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MT"]=1; rec.items["030"]=it; }

    // I170: 2 planned FL entries (24-bit ASCII "350", "370")
    // "350" = 0x33 0x35 0x30 = 0x333530
    // "370" = 0x33 0x37 0x30 = 0x333730
    DecodedItem i170;
    i170.group_repetitions.push_back({{"FL", 0x333530u}});
    i170.group_repetitions.push_back({{"FL", 0x333730u}});
    rec.items["170"] = i170;

    // I171: 1 requested FL entry (24-bit ASCII "350")
    DecodedItem i171;
    i171.group_repetitions.push_back({{"RFL", 0x333530u}});
    rec.items["171"] = i171;

    // I180: 2 speed entries (32-bit ASCII "0450", "0470" in knots)
    // "0450" = 0x30 0x34 0x35 0x30 = 0x30343530
    DecodedItem i180;
    i180.group_repetitions.push_back({{"TAS", 0x30343530u}});
    i180.group_repetitions.push_back({{"TAS", 0x30343730u}});
    rec.items["180"] = i180;

    // I200: Field 18 free text "RMK" (3 ASCII bytes, each 8-bit)
    // R=0x52 M=0x4D K=0x4B
    DecodedItem i200;
    i200.group_repetitions.push_back({{"CH", 0x52u}});
    i200.group_repetitions.push_back({{"CH", 0x4Du}});
    i200.group_repetitions.push_back({{"CH", 0x4Bu}});
    rec.items["200"] = i200;

    auto encoded = codec.encode(150, {rec});
    hexdump(encoded, "RouteMisc encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;

    // I170
    CHECK(items.count("170"), "I170 present");
    const auto& gr170 = items.at("170").group_repetitions;
    CHECK(gr170.size() == 2,               "170 has 2 entries");
    CHECK(gr170[0].at("FL") == 0x333530u,  "170 rep[0] FL = '350'");
    CHECK(gr170[1].at("FL") == 0x333730u,  "170 rep[1] FL = '370'");

    // I171
    CHECK(items.count("171"), "I171 present");
    const auto& gr171 = items.at("171").group_repetitions;
    CHECK(gr171.size() == 1,                "171 has 1 entry");
    CHECK(gr171[0].at("RFL") == 0x333530u,  "171 rep[0] RFL = '350'");

    // I180
    CHECK(items.count("180"), "I180 present");
    const auto& gr180 = items.at("180").group_repetitions;
    CHECK(gr180.size() == 2,                "180 has 2 entries");
    CHECK(gr180[0].at("TAS") == 0x30343530u, "180 rep[0] TAS = '0450'");
    CHECK(gr180[1].at("TAS") == 0x30343730u, "180 rep[1] TAS = '0470'");

    // I200
    CHECK(items.count("200"), "I200 present");
    const auto& gr200 = items.at("200").group_repetitions;
    CHECK(gr200.size() == 3,           "200 has 3 entries");
    CHECK(gr200[0].at("CH") == 0x52u,  "200 rep[0] CH = 'R'");
    CHECK(gr200[1].at("CH") == 0x4Du,  "200 rep[1] CH = 'M'");
    CHECK(gr200[2].at("CH") == 0x4Bu,  "200 rep[2] CH = 'K'");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip correlation RepetitiveGroups (I240, I250, I251)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCorrelation(Codec& codec) {
    std::cout << "\n=== Test: Round-trip correlation RepetitiveGroups ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["CEN"]=1; it.fields["POS"]=0; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MT"]=252; rec.items["030"]=it; }  // Correlations

    // I240: 2 newly correlated plan/track pairs
    DecodedItem i240;
    i240.group_repetitions.push_back({{"PLAN", 100u}, {"TRACK", 500u}});
    i240.group_repetitions.push_back({{"PLAN", 200u}, {"TRACK", 600u}});
    rec.items["240"] = i240;

    // I250: 2 newly de-correlated plans
    DecodedItem i250;
    i250.group_repetitions.push_back({{"PLAN", 150u}});
    i250.group_repetitions.push_back({{"PLAN", 300u}});
    rec.items["250"] = i250;

    // I251: 1 pair of conflicting tracks
    DecodedItem i251;
    i251.group_repetitions.push_back({{"TRACK1", 100u}, {"TRACK2", 200u}});
    rec.items["251"] = i251;

    auto encoded = codec.encode(150, {rec});
    hexdump(encoded, "Correlation encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;

    CHECK(items.at("030").fields.at("MT") == 252, "030/MT = 252 (Correlations)");

    // I240
    CHECK(items.count("240"), "I240 present");
    const auto& gr240 = items.at("240").group_repetitions;
    CHECK(gr240.size() == 2,                "240 has 2 entries");
    CHECK(gr240[0].at("PLAN")  == 100u,     "240 rep[0] PLAN = 100");
    CHECK(gr240[0].at("TRACK") == 500u,     "240 rep[0] TRACK = 500");
    CHECK(gr240[1].at("PLAN")  == 200u,     "240 rep[1] PLAN = 200");
    CHECK(gr240[1].at("TRACK") == 600u,     "240 rep[1] TRACK = 600");

    // I250
    CHECK(items.count("250"), "I250 present");
    const auto& gr250 = items.at("250").group_repetitions;
    CHECK(gr250.size() == 2,            "250 has 2 entries");
    CHECK(gr250[0].at("PLAN") == 150u,  "250 rep[0] PLAN = 150");
    CHECK(gr250[1].at("PLAN") == 300u,  "250 rep[1] PLAN = 300");

    // I251
    CHECK(items.count("251"), "I251 present");
    const auto& gr251 = items.at("251").group_repetitions;
    CHECK(gr251.size() == 1,                 "251 has 1 entry");
    CHECK(gr251[0].at("TRACK1") == 100u,     "251 rep[0] TRACK1 = 100");
    CHECK(gr251[0].at("TRACK2") == 200u,     "251 rep[0] TRACK2 = 200");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Full round-trip – complete flight plan message
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip flight plan message ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    // Header
    { DecodedItem it; it.fields["CEN"]=0xFF; it.fields["POS"]=0; rec.items["010"]=it; }  // broadcast
    { DecodedItem it; it.fields["CEN"]=3;    it.fields["POS"]=1; rec.items["020"]=it; }
    { DecodedItem it; it.fields["MT"]=1; rec.items["030"]=it; }  // flight plan creation
    { DecodedItem it; it.fields["PRN"]=999; rec.items["040"]=it; }

    // Flight identification
    // I050: "BAW456 " = B=0x42 A=0x41 W=0x57 4=0x34 5=0x35 6=0x36 ' '=0x20
    { DecodedItem it; it.fields["CS"]=0x42415734353620ULL; rec.items["050"]=it; }
    // I060: Present Mode 3A "1234"
    { DecodedItem it; it.fields["M3A"]=0x31323334ULL; rec.items["060"]=it; }
    // I080: Departure "EGLL" (London Heathrow)
    { DecodedItem it; it.fields["DEP"]=0x45474C4CULL; rec.items["080"]=it; }
    // I090: Destination "KJFK" (New York JFK)
    { DecodedItem it; it.fields["DEST"]=0x4B4A464BULL; rec.items["090"]=it; }

    // Type / status
    { DecodedItem it; it.fields["GAT"]=1; it.fields["OAT"]=0; it.fields["CPL"]=1; it.fields["SPN"]=0; rec.items["100"]=it; }
    { DecodedItem it; it.fields["HLD"]=0; it.fields["RVQ"]=1; it.fields["RVC"]=1; it.fields["RVX"]=0; rec.items["110"]=it; }

    // I120: Aircraft type – NOA="01", TOA="B77W", WT="H"
    { DecodedItem it;
      it.fields["NOA"]=0x3031ULL;       // "01"
      it.fields["TOA"]=0x42373757ULL;   // "B77W"
      it.fields["WT"] =0x48ULL;         // "H" (heavy)
      rec.items["120"]=it; }

    // I130: Cleared FL "350"
    { DecodedItem it; it.fields["CFL"]=0x333530ULL; rec.items["130"]=it; }

    // Route points
    DecodedItem i150;
    i150.group_repetitions.push_back({{"X", s16(200)}, {"Y", s16(100)}});
    rec.items["150"] = i150;

    DecodedItem i170;
    i170.group_repetitions.push_back({{"FL", 0x333530u}});  // "350"
    rec.items["170"] = i170;

    // Stats
    { DecodedItem it; it.fields["MPC"]=302; rec.items["220"]=it; }
    { DecodedItem it; it.fields["NOP"]=87;  rec.items["230"]=it; }

    auto encoded = codec.encode(150, {rec});
    hexdump(encoded, "Full round-trip encoded");
    CHECK(!encoded.empty(), "encode produced output");
    CHECK(encoded[0] == 0x96, "CAT byte = 0x96 (150)");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("CEN")  == 0xFF,               "010/CEN = 0xFF (broadcast)");
    CHECK(items.at("020").fields.at("CEN")  == 3,                  "020/CEN = 3");
    CHECK(items.at("030").fields.at("MT")   == 1,                  "030/MT = 1 (creation)");
    CHECK(items.at("040").fields.at("PRN")  == 999,                "040/PRN = 999");
    CHECK(items.at("050").fields.at("CS")   == 0x42415734353620ULL,"050/CS = 'BAW456 '");
    CHECK(items.at("060").fields.at("M3A")  == 0x31323334ULL,      "060/M3A = '1234'");
    CHECK(items.at("080").fields.at("DEP")  == 0x45474C4CULL,      "080/DEP = 'EGLL'");
    CHECK(items.at("090").fields.at("DEST") == 0x4B4A464BULL,      "090/DEST = 'KJFK'");
    CHECK(items.at("100").fields.at("GAT")  == 1,                  "100/GAT = 1");
    CHECK(items.at("100").fields.at("CPL")  == 1,                  "100/CPL = 1");
    CHECK(items.at("110").fields.at("RVQ")  == 1,                  "110/RVQ = 1");
    CHECK(items.at("120").fields.at("NOA")  == 0x3031ULL,          "120/NOA = '01'");
    CHECK(items.at("120").fields.at("TOA")  == 0x42373757ULL,      "120/TOA = 'B77W'");
    CHECK(items.at("120").fields.at("WT")   == 0x48ULL,            "120/WT = 'H' (heavy)");
    CHECK(items.at("130").fields.at("CFL")  == 0x333530ULL,        "130/CFL = '350'");
    CHECK(items.count("150"),                                       "I150 present");
    CHECK(items.at("150").group_repetitions[0].at("X") == s16(200),"150/X = 200");
    CHECK(items.at("150").group_repetitions[0].at("Y") == s16(100),"150/Y = 100");
    CHECK(items.count("170"),                                       "I170 present");
    CHECK(items.at("170").group_repetitions[0].at("FL") == 0x333530u, "170/FL = '350'");
    CHECK(items.at("220").fields.at("MPC")  == 302,                "220/MPC = 302");
    CHECK(items.at("230").fields.at("NOP")  == 87,                 "230/NOP = 87");
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = (argc > 1) ? argv[1] : "specs/CAT150.xml";

    Codec codec;
    testSpecLoad(codec, spec_path);
    testDecodeBasicMessage(codec);
    testRoundTripCoreFixed(codec);
    testRoundTripAsciiFixed(codec);
    testRoundTripRouteCoords(codec);
    testRoundTripRouteMisc(codec);
    testRoundTripCorrelation(codec);
    testFullRoundTrip(codec);

    std::cout << "\n=== Results: "
              << (failures == 0 ? "ALL PASSED" : std::to_string(failures) + " FAILED")
              << " ===\n";
    return failures == 0 ? 0 : 1;
}
