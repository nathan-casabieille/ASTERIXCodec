// test_cat32.cpp – Tests for CAT32 Miniplan Reports to an SDPS.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat32

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
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 1: XML spec loads correctly
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT32 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 32,         "cat number = 32");
    CHECK(cat.edition == "1.2",  "edition = 1.2");

    for (auto id : {"010","015","018","020","035","040","050","060",
                    "400","410","420","430","435","440","450","460",
                    "480","490","500","RE"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 20, "20 items total");

    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    CHECK(cat.uap_variations.at("default").size() == 21, "UAP has 21 slots");

    const auto& uap = cat.uap_variations.at("default");
    // Byte 1: 010 015 018 035 020 040 050
    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "015", "UAP slot  2 = 015");
    CHECK(uap[2]  == "018", "UAP slot  3 = 018");
    CHECK(uap[3]  == "035", "UAP slot  4 = 035");
    CHECK(uap[4]  == "020", "UAP slot  5 = 020");
    CHECK(uap[5]  == "040", "UAP slot  6 = 040");
    CHECK(uap[6]  == "050", "UAP slot  7 = 050");
    // Byte 2: 060 400 410 420 440 450 480
    CHECK(uap[7]  == "060", "UAP slot  8 = 060");
    CHECK(uap[8]  == "400", "UAP slot  9 = 400");
    CHECK(uap[9]  == "410", "UAP slot 10 = 410");
    CHECK(uap[10] == "420", "UAP slot 11 = 420");
    CHECK(uap[11] == "440", "UAP slot 12 = 440");
    CHECK(uap[12] == "450", "UAP slot 13 = 450");
    CHECK(uap[13] == "480", "UAP slot 14 = 480");
    // Byte 3: 490 430 435 460 500 - RE
    CHECK(uap[14] == "490", "UAP slot 15 = 490");
    CHECK(uap[15] == "430", "UAP slot 16 = 430");
    CHECK(uap[16] == "435", "UAP slot 17 = 435");
    CHECK(uap[17] == "460", "UAP slot 18 = 460");
    CHECK(uap[18] == "500", "UAP slot 19 = 500");
    CHECK(uap[19] == "-",   "UAP slot 20 = - (unused)");
    CHECK(uap[20] == "RE",  "UAP slot 21 = RE");

    // Item types
    CHECK(cat.items.at("010").type == ItemType::Fixed,           "010 is Fixed");
    CHECK(cat.items.at("050").type == ItemType::Fixed,           "050 is Fixed (3B model)");
    CHECK(cat.items.at("460").type == ItemType::RepetitiveGroup, "460 is RepetitiveGroup");
    CHECK(cat.items.at("500").type == ItemType::Compound,        "500 is Compound");
    CHECK(cat.items.at("RE").type  == ItemType::SP,              "RE is SP/Explicit");

    // Fixed byte sizes
    CHECK(cat.items.at("010").fixed_bytes == 2,  "010 = 2 bytes");
    CHECK(cat.items.at("015").fixed_bytes == 2,  "015 = 2 bytes");
    CHECK(cat.items.at("018").fixed_bytes == 2,  "018 = 2 bytes");
    CHECK(cat.items.at("020").fixed_bytes == 3,  "020 = 3 bytes");
    CHECK(cat.items.at("035").fixed_bytes == 1,  "035 = 1 byte");
    CHECK(cat.items.at("040").fixed_bytes == 2,  "040 = 2 bytes");
    CHECK(cat.items.at("050").fixed_bytes == 3,  "050 = 3 bytes");
    CHECK(cat.items.at("060").fixed_bytes == 2,  "060 = 2 bytes");
    CHECK(cat.items.at("400").fixed_bytes == 7,  "400 = 7 bytes");
    CHECK(cat.items.at("430").fixed_bytes == 4,  "430 = 4 bytes");
    CHECK(cat.items.at("435").fixed_bytes == 1,  "435 = 1 byte");
    CHECK(cat.items.at("440").fixed_bytes == 4,  "440 = 4 bytes");
    CHECK(cat.items.at("450").fixed_bytes == 4,  "450 = 4 bytes");
    CHECK(cat.items.at("480").fixed_bytes == 2,  "480 = 2 bytes");
    CHECK(cat.items.at("490").fixed_bytes == 2,  "490 = 2 bytes");

    // I460 RepetitiveGroup: spare(4)+OCT1(3)+OCT2(3)+OCT3(3)+OCT4(3) = 16 bits = 2B per entry
    // Spares included in rep_group_elements: spare, OCT1, OCT2, OCT3, OCT4 = 5 entries
    const auto& i460 = cat.items.at("460");
    CHECK(i460.rep_group_bits == 16,             "460 rep_group_bits = 16 (2 bytes)");
    CHECK(i460.rep_group_elements.size() == 5,   "460 has 5 group elements (spare+OCT1..OCT4)");
    CHECK(i460.rep_group_elements[1].name == "OCT1", "460 element[1] = OCT1");
    CHECK(i460.rep_group_elements[4].name == "OCT4", "460 element[4] = OCT4");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode basic miniplan message
//          I010(SAC=1,SIC=2) + I015(UN=42)
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicMessage(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT32 message ===\n";

    // FSPEC byte 1: I010(b7)+I015(b6) = 0b11000000 = 0xC0, FX=0
    // LEN = 3(header) + 1(FSPEC) + 2(I010) + 2(I015) = 8
    std::vector<uint8_t> frame = {
        0x20,             // CAT = 32
        0x00, 0x08,       // LEN = 8
        0xC0,             // FSPEC byte 1 (FX=0): I010+I015
        0x01, 0x02,       // I010 SAC=1, SIC=2
        0x00, 0x2A        // I015 UN=42
    };

    hexdump(frame, "Basic miniplan input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid, "block is valid");
    CHECK(blk.records.size() == 1, "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"), "I010 present");
    CHECK(rec.items.count("015"), "I015 present");
    CHECK(!rec.items.count("500"), "I500 absent");

    CHECK(rec.items.at("010").fields.at("SAC") == 1,  "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC") == 2,  "I010.SIC=2");
    CHECK(rec.items.at("015").fields.at("UN")  == 42, "I015.UN=42");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip basic Fixed items (I010, I015, I018, I020, I035, I040)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripBasicFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip basic Fixed items ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=5; it.fields["SIC"]=10; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["UN"]=100; enc_rec.items["015"]=it; }
    { DecodedItem it; it.fields["SAC"]=2; it.fields["SIC"]=3; enc_rec.items["018"]=it; }
    { DecodedItem it; it.fields["TOD"]=0x200000; enc_rec.items["020"]=it; }
    { DecodedItem it; it.fields["FAMILY"]=2; it.fields["NATURE"]=3; enc_rec.items["035"]=it; }
    { DecodedItem it; it.fields["TN"]=12345; enc_rec.items["040"]=it; }

    std::vector<uint8_t> encoded = codec.encode(32, {enc_rec});
    hexdump(encoded, "Basic Fixed encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    checkItemsMatch(blk.records[0].items, enc_rec.items, "RT-basic");

    CHECK(blk.records[0].items.at("035").fields.at("FAMILY") == 2,  "I035.FAMILY=2");
    CHECK(blk.records[0].items.at("035").fields.at("NATURE") == 3,  "I035.NATURE=3");
    CHECK(blk.records[0].items.at("040").fields.at("TN")     == 12345, "I040.TN=12345");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip I050 (Fixed 3B: SUI+STN+spare)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCurrentPlanNumber(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I050 Current Plan Number ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i050;
    i050.fields["SUI"] = 7;
    i050.fields["STN"] = 1234;
    enc_rec.items["050"] = i050;

    std::vector<uint8_t> encoded = codec.encode(32, {enc_rec});
    hexdump(encoded, "I050 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("050"), "I050 present");
    const auto& f = blk.records[0].items.at("050").fields;
    CHECK(f.at("SUI") == 7,    "I050.SUI=7");
    CHECK(f.at("STN") == 1234, "I050.STN=1234");

    // Boundary: max SUI=255, max STN=32767
    DecodedRecord r2;
    r2.uap_variation = "default";
    DecodedItem i2; i2.fields["SUI"]=255; i2.fields["STN"]=32767;
    r2.items["050"] = i2;
    auto enc2 = codec.encode(32, {r2});
    auto blk2 = codec.decode(enc2);
    if (!blk2.records.empty()) {
        const auto& f2 = blk2.records[0].items.at("050").fields;
        CHECK(f2.at("SUI") == 255,   "I050.SUI=255 (max)");
        CHECK(f2.at("STN") == 32767, "I050.STN=32767 (max)");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip I060 (Mode 3/A, string_octal) and ASCII items
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripMode3AAndASCII(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I060 Mode 3/A and ASCII items ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    // MODE3A octal 1234 → 1*512+2*64+3*8+4 = 668
    DecodedItem i060; i060.fields["MODE3A"] = 668; enc_rec.items["060"] = i060;

    // I400 callsign: "RYANAIR" = 0x52 59 41 4E 41 49 52
    uint64_t cs = (uint64_t)0x52 << 48 | (uint64_t)0x59 << 40 | (uint64_t)0x41 << 32 |
                  (uint64_t)0x4E << 24 | (uint64_t)0x41 << 16 | (uint64_t)0x49 << 8 | 0x52;
    DecodedItem i400; i400.fields["CS"] = cs; enc_rec.items["400"] = i400;

    // I430 type of aircraft: "B738" = 0x42373338
    DecodedItem i430; i430.fields["TYAC"] = 0x42373338u; enc_rec.items["430"] = i430;

    // I440 departure: "LFPG" = 0x4C465047
    DecodedItem i440; i440.fields["DEP"] = 0x4C465047u; enc_rec.items["440"] = i440;

    // I450 destination: "EGLL" = 0x45474C4C
    DecodedItem i450; i450.fields["DEST"] = 0x45474C4Cu; enc_rec.items["450"] = i450;

    std::vector<uint8_t> encoded = codec.encode(32, {enc_rec});
    hexdump(encoded, "I060+ASCII encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.at("060").fields.at("MODE3A") == 668,         "I060.MODE3A=668 (octal 1234)");
    CHECK(blk.records[0].items.at("400").fields.at("CS")     == cs,           "I400.CS='RYANAIR'");
    CHECK(blk.records[0].items.at("430").fields.at("TYAC")   == 0x42373338u,  "I430.TYAC='B738'");
    CHECK(blk.records[0].items.at("440").fields.at("DEP")    == 0x4C465047u,  "I440.DEP='LFPG'");
    CHECK(blk.records[0].items.at("450").fields.at("DEST")   == 0x45474C4Cu,  "I450.DEST='EGLL'");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip I460 Allocated SSR Codes (RepetitiveGroup, 2 entries)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripSSRCodes(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I460 Allocated SSR Codes ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i460;
    // Entry 0: 1234 in octal → OCT1=1,OCT2=2,OCT3=3,OCT4=4
    i460.group_repetitions.push_back({{"OCT1", 1}, {"OCT2", 2}, {"OCT3", 3}, {"OCT4", 4}});
    // Entry 1: 0007 in octal → OCT1=0,OCT2=0,OCT3=0,OCT4=7
    i460.group_repetitions.push_back({{"OCT1", 0}, {"OCT2", 0}, {"OCT3", 0}, {"OCT4", 7}});
    enc_rec.items["460"] = i460;

    std::vector<uint8_t> encoded = codec.encode(32, {enc_rec});
    hexdump(encoded, "I460 encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("460"), "I460 present");
    const auto& groups = blk.records[0].items.at("460").group_repetitions;
    CHECK(groups.size() == 2, "I460: 2 entries");

    if (groups.size() == 2) {
        CHECK(groups[0].at("OCT1") == 1, "I460[0].OCT1=1");
        CHECK(groups[0].at("OCT2") == 2, "I460[0].OCT2=2");
        CHECK(groups[0].at("OCT3") == 3, "I460[0].OCT3=3");
        CHECK(groups[0].at("OCT4") == 4, "I460[0].OCT4=4");
        CHECK(groups[1].at("OCT1") == 0, "I460[1].OCT1=0");
        CHECK(groups[1].at("OCT4") == 7, "I460[1].OCT4=7");
    }

    // Single entry
    DecodedRecord r2;
    r2.uap_variation = "default";
    DecodedItem i2;
    i2.group_repetitions.push_back({{"OCT1", 7}, {"OCT2", 7}, {"OCT3", 7}, {"OCT4", 7}});
    r2.items["460"] = i2;
    auto enc2 = codec.encode(32, {r2});
    auto blk2 = codec.decode(enc2);
    if (!blk2.records.empty()) {
        const auto& g2 = blk2.records[0].items.at("460").group_repetitions;
        CHECK(g2.size() == 1, "I460: 1 entry");
        if (!g2.empty()) {
            CHECK(g2[0].at("OCT1") == 7, "I460[0].OCT1=7 (max octal digit)");
            CHECK(g2[0].at("OCT4") == 7, "I460[0].OCT4=7 (max octal digit)");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip I500 Supplementary Flight Data (Compound)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripSupplementaryData(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I500 Supplementary Flight Data (Compound) ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    DecodedItem i500;
    // IFI: TYP=0 (Plan Number), NBR=12345
    i500.compound_sub_fields["IFI"]["TYP"] = 0;
    i500.compound_sub_fields["IFI"]["NBR"] = 12345;
    // RVP: RVSM=1 (Approved), HPR=0
    i500.compound_sub_fields["RVP"]["RVSM"] = 1;
    i500.compound_sub_fields["RVP"]["HPR"]  = 0;
    // STS: EMP=1 (Occupied), AVL=0 (Available)
    i500.compound_sub_fields["STS"]["EMP"] = 1;
    i500.compound_sub_fields["STS"]["AVL"] = 0;
    enc_rec.items["500"] = i500;

    std::vector<uint8_t> encoded = codec.encode(32, {enc_rec});
    hexdump(encoded, "I500 Compound encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    CHECK(blk.records[0].items.count("500"), "I500 present");
    const auto& csf = blk.records[0].items.at("500").compound_sub_fields;

    CHECK(csf.count("IFI"),                 "I500/IFI present");
    CHECK(csf.count("RVP"),                 "I500/RVP present");
    CHECK(csf.count("STS"),                 "I500/STS present");

    if (csf.count("IFI")) {
        CHECK(csf.at("IFI").at("TYP") == 0,     "I500/IFI.TYP=0 (Plan Number)");
        CHECK(csf.at("IFI").at("NBR") == 12345,  "I500/IFI.NBR=12345");
    }
    if (csf.count("RVP")) {
        CHECK(csf.at("RVP").at("RVSM") == 1, "I500/RVP.RVSM=1 (Approved)");
        CHECK(csf.at("RVP").at("HPR")  == 0, "I500/RVP.HPR=0 (Normal)");
    }
    if (csf.count("STS")) {
        CHECK(csf.at("STS").at("EMP") == 1, "I500/STS.EMP=1 (Occupied)");
        CHECK(csf.at("STS").at("AVL") == 0, "I500/STS.AVL=0 (Available)");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Full round-trip with all item types
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip CAT32 ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["UN"]=7; enc_rec.items["015"]=it; }
    { DecodedItem it; it.fields["SAC"]=3; it.fields["SIC"]=4; enc_rec.items["018"]=it; }
    { DecodedItem it; it.fields["TOD"]=0x400000; enc_rec.items["020"]=it; }
    { DecodedItem it; it.fields["FAMILY"]=4; it.fields["NATURE"]=1; enc_rec.items["035"]=it; }
    { DecodedItem it; it.fields["TN"]=9999; enc_rec.items["040"]=it; }
    { DecodedItem it; it.fields["SUI"]=3; it.fields["STN"]=500; enc_rec.items["050"]=it; }
    { DecodedItem it; it.fields["MODE3A"]=0; enc_rec.items["060"]=it; }  // 0000 octal
    { DecodedItem it; it.fields["UN"]=42; enc_rec.items["015"]=it; }  // overwrite UN
    { DecodedItem it; it.fields["PN"]=88; enc_rec.items["410"]=it; }
    { DecodedItem it; it.fields["GATOAT"]=1; it.fields["FR1FR2"]=0; it.fields["SP3"]=0; it.fields["SP2"]=0; it.fields["SP1"]=0; enc_rec.items["420"]=it; }
    { DecodedItem it; it.fields["WTC"]=77; enc_rec.items["435"]=it; }  // Medium
    { DecodedItem it; it.fields["CFL"]=240; enc_rec.items["480"]=it; }  // FL 60 (240 * 0.25)
    { DecodedItem it; it.fields["CEN"]=1; it.fields["POS"]=3; enc_rec.items["490"]=it; }
    {
        DecodedItem it;
        it.group_repetitions.push_back({{"OCT1", 1}, {"OCT2", 3}, {"OCT3", 4}, {"OCT4", 0}});
        enc_rec.items["460"] = it;
    }
    {
        DecodedItem it;
        it.compound_sub_fields["IFI"]["TYP"] = 1;
        it.compound_sub_fields["IFI"]["NBR"] = 999;
        it.compound_sub_fields["RVP"]["RVSM"] = 0;
        it.compound_sub_fields["RVP"]["HPR"]  = 1;
        enc_rec.items["500"] = it;
    }

    std::vector<uint8_t> encoded = codec.encode(32, {enc_rec});
    hexdump(encoded, "Full RT encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"), "I010 (mandatory) present");
    CHECK(rec.items.count("050"), "I050 present");
    CHECK(rec.items.count("460"), "I460 present");
    CHECK(rec.items.count("500"), "I500 present");

    CHECK(rec.items.at("050").fields.at("SUI") == 3,   "I050.SUI=3");
    CHECK(rec.items.at("050").fields.at("STN") == 500, "I050.STN=500");
    CHECK(rec.items.at("480").fields.at("CFL") == 240, "I480.CFL=240 (FL60)");
    CHECK(rec.items.at("490").fields.at("CEN") == 1,   "I490.CEN=1");
    CHECK(rec.items.at("490").fields.at("POS") == 3,   "I490.POS=3");
    CHECK(rec.items.at("435").fields.at("WTC") == 77,  "I435.WTC=77 (Medium)");

    const auto& groups = rec.items.at("460").group_repetitions;
    CHECK(groups.size() == 1, "I460: 1 entry");
    if (!groups.empty()) {
        CHECK(groups[0].at("OCT1") == 1, "I460[0].OCT1=1");
        CHECK(groups[0].at("OCT2") == 3, "I460[0].OCT2=3");
    }

    const auto& csf = rec.items.at("500").compound_sub_fields;
    if (csf.count("IFI")) {
        CHECK(csf.at("IFI").at("TYP") == 1,  "I500/IFI.TYP=1 (unit 1 internal)");
        CHECK(csf.at("IFI").at("NBR") == 999, "I500/IFI.NBR=999");
    }
    if (csf.count("RVP")) {
        CHECK(csf.at("RVP").at("HPR") == 1, "I500/RVP.HPR=1 (High Priority)");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = "specs/CAT32.xml";
    if (argc >= 2)
        spec_path = argv[1];

    if (!fs::exists(spec_path)) {
        std::cerr << "Spec not found: " << spec_path << '\n';
        return 1;
    }

    Codec codec;

    testSpecLoad(codec, spec_path);
    testDecodeBasicMessage(codec);
    testRoundTripBasicFixed(codec);
    testRoundTripCurrentPlanNumber(codec);
    testRoundTripMode3AAndASCII(codec);
    testRoundTripSSRCodes(codec);
    testRoundTripSupplementaryData(codec);
    testFullRoundTrip(codec);

    std::cout << "\n============================\n";
    if (failures == 0)
        std::cout << "ALL TESTS PASSED\n";
    else
        std::cout << failures << " TEST(S) FAILED\n";
    std::cout << "============================\n";

    return failures ? 1 : 0;
}
