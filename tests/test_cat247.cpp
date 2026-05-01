// test_cat247.cpp – Tests for CAT247 Version Number Exchange.
//
// Compile with CMake:
//   cmake -B build && cmake --build build
//   ./build/test_cat247

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

// ─────────────────────────────────────────────────────────────────────────────
//  Test 1: XML spec loads correctly
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT247 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 247,        "cat number = 247");
    CHECK(cat.edition == "1.3",  "edition = 1.3");

    for (auto id : {"010","015","140","550","RE","SP"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 6, "6 items total");

    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    CHECK(cat.uap_variations.at("default").size() == 7, "UAP has 7 slots");

    const auto& uap = cat.uap_variations.at("default");
    CHECK(uap[0] == "010", "UAP slot 1 = 010");
    CHECK(uap[1] == "015", "UAP slot 2 = 015");
    CHECK(uap[2] == "140", "UAP slot 3 = 140");
    CHECK(uap[3] == "550", "UAP slot 4 = 550");
    CHECK(uap[4] == "-",   "UAP slot 5 = - (unused)");
    CHECK(uap[5] == "SP",  "UAP slot 6 = SP");
    CHECK(uap[6] == "RE",  "UAP slot 7 = RE");

    // Item types
    for (auto id : {"010","015","140"}) {
        CHECK(cat.items.at(id).type == ItemType::Fixed,
              std::string(id) + " is Fixed");
    }
    CHECK(cat.items.at("550").type == ItemType::RepetitiveGroup, "550 is RepetitiveGroup");
    CHECK(cat.items.at("RE").type  == ItemType::SP,              "RE is SP/Explicit");

    // Fixed byte sizes
    CHECK(cat.items.at("010").fixed_bytes == 2, "010 = 2 bytes");
    CHECK(cat.items.at("015").fixed_bytes == 1, "015 = 1 byte");
    CHECK(cat.items.at("140").fixed_bytes == 3, "140 = 3 bytes");

    // I550 repetitive structure
    CHECK(cat.items.at("550").rep_group_bits == 24, "550 rep_group_bits = 24 (3 bytes/entry)");
    CHECK(cat.items.at("550").rep_group_elements.size() == 3, "550 has 3 elements");
    CHECK(cat.items.at("550").rep_group_elements[0].name == "CAT",  "550 element[0] = CAT");
    CHECK(cat.items.at("550").rep_group_elements[1].name == "MAIN", "550 element[1] = MAIN");
    CHECK(cat.items.at("550").rep_group_elements[2].name == "SUB",  "550 element[2] = SUB");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode a basic message with I010 only
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicMessage(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT247 message ===\n";

    // FSPEC byte 1: I010(b7) = 0b10000000 = 0x80, FX=0
    // LEN = 3(header) + 1(FSPEC) + 2(I010) = 6
    std::vector<uint8_t> frame = {
        0xF7,             // CAT = 247
        0x00, 0x06,       // LEN = 6
        0x80,             // FSPEC: I010 only
        0x01, 0x02        // I010 SAC=1, SIC=2
    };

    hexdump(frame, "Basic CAT247 input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid, "block is valid");
    CHECK(blk.records.size() == 1, "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"),  "I010 present");
    CHECK(!rec.items.count("015"), "I015 absent");
    CHECK(!rec.items.count("550"), "I550 absent");

    CHECK(rec.items.at("010").fields.at("SAC") == 1, "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC") == 2, "I010.SIC=2");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip simple Fixed items (I010 + I015 + I140)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripSimpleFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip simple Fixed items ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=3; it.fields["SIC"]=7; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["SI"]=5;                       enc_rec.items["015"]=it; }
    { DecodedItem it; it.fields["TOD"]=0x600000;               enc_rec.items["140"]=it; }

    std::vector<uint8_t> encoded = codec.encode(247, {enc_rec});
    hexdump(encoded, "Simple Fixed encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("SAC") == 3,        "I010.SAC=3");
    CHECK(items.at("010").fields.at("SIC") == 7,        "I010.SIC=7");
    CHECK(items.at("015").fields.at("SI")  == 5,        "I015.SI=5");
    CHECK(items.at("140").fields.at("TOD") == 0x600000, "I140.TOD=0x600000");

    // Boundary: SI=0 and SI=255
    for (uint64_t si : {0u, 255u}) {
        DecodedRecord r; r.uap_variation = "default";
        { DecodedItem it; it.fields["SAC"]=0; it.fields["SIC"]=0; r.items["010"]=it; }
        { DecodedItem it; it.fields["SI"]=si;                      r.items["015"]=it; }
        auto enc = codec.encode(247, {r});
        auto b   = codec.decode(enc);
        if (!b.records.empty())
            CHECK(b.records[0].items.at("015").fields.at("SI") == si,
                  "I015.SI=" + std::to_string(si));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip I550 Category Version Number Report
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripVersionReport(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I550 Version Report ===\n";

    // 4a: single entry
    {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; enc_rec.items["010"]=it; }

        DecodedItem i550;
        std::map<std::string, uint64_t> entry;
        entry["CAT"]  = 48;
        entry["MAIN"] = 1;
        entry["SUB"]  = 32;
        i550.group_repetitions.push_back(entry);
        enc_rec.items["550"] = i550;

        auto encoded = codec.encode(247, {enc_rec});
        hexdump(encoded, "I550 single entry encoded");

        auto blk = codec.decode(encoded);
        CHECK(blk.valid, "I550-a: block valid");
        if (!blk.records.empty()) {
            const auto& reps = blk.records[0].items.at("550").group_repetitions;
            CHECK(reps.size() == 1, "I550: 1 entry");
            CHECK(reps[0].at("CAT")  == 48, "I550[0].CAT=48");
            CHECK(reps[0].at("MAIN") == 1,  "I550[0].MAIN=1");
            CHECK(reps[0].at("SUB")  == 32, "I550[0].SUB=32");
        }
    }

    // 4b: multiple entries (several ASTERIX categories)
    {
        DecodedRecord enc_rec;
        enc_rec.uap_variation = "default";
        { DecodedItem it; it.fields["SAC"]=2; it.fields["SIC"]=4; enc_rec.items["010"]=it; }

        struct VerEntry { uint64_t cat, main, sub; };
        std::vector<VerEntry> versions = {
            {1,  4, 0},
            {34, 1, 29},
            {48, 1, 32},
            {62, 1, 21},
            {65, 1, 6},
        };

        DecodedItem i550;
        for (auto& v : versions) {
            std::map<std::string, uint64_t> e;
            e["CAT"]  = v.cat;
            e["MAIN"] = v.main;
            e["SUB"]  = v.sub;
            i550.group_repetitions.push_back(e);
        }
        enc_rec.items["550"] = i550;

        auto encoded = codec.encode(247, {enc_rec});
        hexdump(encoded, "I550 multi-entry encoded");

        auto blk = codec.decode(encoded);
        CHECK(blk.valid, "I550-b: block valid");
        if (!blk.records.empty()) {
            const auto& reps = blk.records[0].items.at("550").group_repetitions;
            CHECK(reps.size() == 5, "I550: 5 entries");
            for (size_t i = 0; i < versions.size() && i < reps.size(); ++i) {
                CHECK(reps[i].at("CAT")  == versions[i].cat,
                      "I550[" + std::to_string(i) + "].CAT=" + std::to_string(versions[i].cat));
                CHECK(reps[i].at("MAIN") == versions[i].main,
                      "I550[" + std::to_string(i) + "].MAIN=" + std::to_string(versions[i].main));
                CHECK(reps[i].at("SUB")  == versions[i].sub,
                      "I550[" + std::to_string(i) + "].SUB=" + std::to_string(versions[i].sub));
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: I550 boundary values (CAT=0/255, MAIN=0/255, SUB=0/255)
// ─────────────────────────────────────────────────────────────────────────────
static void testVersionBoundary(Codec& codec) {
    std::cout << "\n=== Test: I550 boundary values ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";
    { DecodedItem it; it.fields["SAC"]=0; it.fields["SIC"]=0; enc_rec.items["010"]=it; }

    DecodedItem i550;
    std::map<std::string, uint64_t> e0; e0["CAT"]=0;   e0["MAIN"]=0;   e0["SUB"]=0;
    std::map<std::string, uint64_t> e1; e1["CAT"]=255; e1["MAIN"]=255; e1["SUB"]=255;
    i550.group_repetitions.push_back(e0);
    i550.group_repetitions.push_back(e1);
    enc_rec.items["550"] = i550;

    auto encoded = codec.encode(247, {enc_rec});
    hexdump(encoded, "I550 boundary encoded");

    auto blk = codec.decode(encoded);
    CHECK(blk.valid, "I550-boundary: block valid");
    if (!blk.records.empty()) {
        const auto& reps = blk.records[0].items.at("550").group_repetitions;
        CHECK(reps.size() == 2, "I550: 2 boundary entries");
        if (reps.size() >= 2) {
            CHECK(reps[0].at("CAT")  == 0,   "I550[0].CAT=0 (min)");
            CHECK(reps[0].at("MAIN") == 0,   "I550[0].MAIN=0 (min)");
            CHECK(reps[0].at("SUB")  == 0,   "I550[0].SUB=0 (min)");
            CHECK(reps[1].at("CAT")  == 255, "I550[1].CAT=255 (max)");
            CHECK(reps[1].at("MAIN") == 255, "I550[1].MAIN=255 (max)");
            CHECK(reps[1].at("SUB")  == 255, "I550[1].SUB=255 (max)");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Full round-trip with all items present
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip CAT247 ===\n";

    DecodedRecord enc_rec;
    enc_rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=10; it.fields["SIC"]=20; enc_rec.items["010"]=it; }
    { DecodedItem it; it.fields["SI"]=3;                         enc_rec.items["015"]=it; }
    { DecodedItem it; it.fields["TOD"]=0x800000;                 enc_rec.items["140"]=it; }

    DecodedItem i550;
    std::map<std::string, uint64_t> e1; e1["CAT"]=21; e1["MAIN"]=2; e1["SUB"]=7;
    std::map<std::string, uint64_t> e2; e2["CAT"]=62; e2["MAIN"]=1; e2["SUB"]=21;
    i550.group_repetitions.push_back(e1);
    i550.group_repetitions.push_back(e2);
    enc_rec.items["550"] = i550;

    std::vector<uint8_t> encoded = codec.encode(247, {enc_rec});
    hexdump(encoded, "Full RT encoded");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.count("010"), "I010 present");
    CHECK(items.count("015"), "I015 present");
    CHECK(items.count("140"), "I140 present");
    CHECK(items.count("550"), "I550 present");

    CHECK(items.at("010").fields.at("SAC") == 10,       "I010.SAC=10");
    CHECK(items.at("010").fields.at("SIC") == 20,       "I010.SIC=20");
    CHECK(items.at("015").fields.at("SI")  == 3,        "I015.SI=3");
    CHECK(items.at("140").fields.at("TOD") == 0x800000, "I140.TOD=0x800000");

    const auto& reps = items.at("550").group_repetitions;
    CHECK(reps.size() == 2, "I550: 2 version entries");
    if (reps.size() >= 2) {
        CHECK(reps[0].at("CAT")  == 21, "I550[0].CAT=21 (CAT021)");
        CHECK(reps[0].at("MAIN") == 2,  "I550[0].MAIN=2");
        CHECK(reps[0].at("SUB")  == 7,  "I550[0].SUB=7");
        CHECK(reps[1].at("CAT")  == 62, "I550[1].CAT=62 (CAT062)");
        CHECK(reps[1].at("MAIN") == 1,  "I550[1].MAIN=1");
        CHECK(reps[1].at("SUB")  == 21, "I550[1].SUB=21");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = "specs/CAT247.xml";
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
    testRoundTripVersionReport(codec);
    testVersionBoundary(codec);
    testFullRoundTrip(codec);

    std::cout << "\n============================\n";
    if (failures == 0)
        std::cout << "ALL TESTS PASSED\n";
    else
        std::cout << failures << " TEST(S) FAILED\n";
    std::cout << "============================\n";

    return failures ? 1 : 0;
}
