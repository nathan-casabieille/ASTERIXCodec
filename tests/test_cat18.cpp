// test_cat18.cpp – Tests for CAT18 Mode S Datalink Function Messages, Ed. 1.8.

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

// ─────────────────────────────────────────────────────────────────────────────
//  Test 1: Spec load
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT18 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 18,        "cat number = 18");
    CHECK(cat.edition == "1.8", "edition = 1.8");

    for (auto id : {"000","001","002","004","005","006","007","008","009","010",
                    "011","012","013","014","015","016","017","018","019","020",
                    "021","022","023","025","027","028","029","030","031","032",
                    "033","034","035","036","037"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 35, "35 items total");

    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    const auto& uap = cat.uap_variations.at("default");
    CHECK(uap.size() == 35, "UAP has 35 slots");

    CHECK(uap[0]  == "036", "UAP slot  1 = 036");
    CHECK(uap[1]  == "037", "UAP slot  2 = 037");
    CHECK(uap[2]  == "000", "UAP slot  3 = 000");
    CHECK(uap[6]  == "017", "UAP slot  7 = 017");
    CHECK(uap[7]  == "018", "UAP slot  8 = 018");
    CHECK(uap[13] == "029", "UAP slot 14 = 029");
    CHECK(uap[14] == "002", "UAP slot 15 = 002");
    CHECK(uap[27] == "004", "UAP slot 28 = 004");
    CHECK(uap[28] == "031", "UAP slot 29 = 031");
    CHECK(uap[34] == "013", "UAP slot 35 = 013");

    for (auto id : {"000","001","002","004","005","007","010","011","012","013",
                    "014","015","016","018","020","021","022","023","025","027",
                    "028","029","030","031","032","033","034","035","036","037"}) {
        CHECK(cat.items.at(id).type == ItemType::Fixed,
              std::string(id) + " is Fixed");
    }
    CHECK(cat.items.at("008").type == ItemType::Extended,        "008 is Extended");
    CHECK(cat.items.at("009").type == ItemType::Extended,        "009 is Extended");
    CHECK(cat.items.at("006").type == ItemType::RepetitiveGroup, "006 is RepetitiveGroup");
    CHECK(cat.items.at("017").type == ItemType::RepetitiveGroup, "017 is RepetitiveGroup");
    CHECK(cat.items.at("019").type == ItemType::SP,              "019 is SP/Explicit");

    CHECK(cat.items.at("000").fixed_bytes == 1, "000 = 1 byte");
    CHECK(cat.items.at("002").fixed_bytes == 3, "002 = 3 bytes");
    CHECK(cat.items.at("005").fixed_bytes == 3, "005 = 3 bytes");
    CHECK(cat.items.at("011").fixed_bytes == 7, "011 = 7 bytes");
    CHECK(cat.items.at("014").fixed_bytes == 4, "014 = 4 bytes");
    CHECK(cat.items.at("015").fixed_bytes == 4, "015 = 4 bytes");
    CHECK(cat.items.at("021").fixed_bytes == 6, "021 = 6 bytes");
    CHECK(cat.items.at("023").fixed_bytes == 7, "023 = 7 bytes");
    CHECK(cat.items.at("029").fixed_bytes == 7, "029 = 7 bytes");
    CHECK(cat.items.at("031").fixed_bytes == 6, "031 = 6 bytes");
    CHECK(cat.items.at("036").fixed_bytes == 2, "036 = 2 bytes");
    CHECK(cat.items.at("037").fixed_bytes == 2, "037 = 2 bytes");

    CHECK(cat.items.at("008").octets.size() == 2, "008 has 2 octets");
    CHECK(cat.items.at("009").octets.size() == 2, "009 has 2 octets");

    CHECK(cat.items.at("006").rep_group_bits == 24, "006 rep_group_bits = 24");
    CHECK(cat.items.at("017").rep_group_bits == 32, "017 rep_group_bits = 32");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode a minimal message from raw bytes
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicMessage(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT18 message ===\n";

    // FSPEC B1: I036(b7)+I037(b6)+I000(b5) = 0xE0
    // Total: CAT(1)+LEN(2)+FSPEC(1)+I036(2)+I037(2)+I000(1) = 9 bytes
    std::vector<uint8_t> frame = {
        0x12,             // CAT = 18
        0x00, 0x09,       // LEN = 9
        0xE0,             // FSPEC: 036+037+000, FX=0
        0x01, 0x02,       // I036: SAC=1, SIC=2
        0x03, 0x04,       // I037: SAC=3, SIC=4
        0x05              // I000: MT=5 (Keep_alive)
    };

    hexdump(frame, "Basic message input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid,                "block is valid");
    CHECK(blk.records.size() == 1,  "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("036"),  "I036 present");
    CHECK(rec.items.count("037"),  "I037 present");
    CHECK(rec.items.count("000"),  "I000 present");
    CHECK(!rec.items.count("002"), "I002 absent");

    CHECK(rec.items.at("036").fields.at("SAC") == 1, "I036.SAC=1");
    CHECK(rec.items.at("036").fields.at("SIC") == 2, "I036.SIC=2");
    CHECK(rec.items.at("037").fields.at("SAC") == 3, "I037.SAC=3");
    CHECK(rec.items.at("037").fields.at("SIC") == 4, "I037.SIC=4");
    CHECK(rec.items.at("000").fields.at("MT")  == 5, "I000.MT=5 (Keep_alive)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip core Fixed items (FSPEC bytes 1–4)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCoreFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip core Fixed items ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=10; it.fields["SIC"]=20; rec.items["036"]=it; }
    { DecodedItem it; it.fields["SAC"]=11; it.fields["SIC"]=21; rec.items["037"]=it; }
    { DecodedItem it; it.fields["MT"]=5; rec.items["000"]=it; }
    { DecodedItem it; it.fields["CAUSE"]=3; it.fields["DIAG"]=2; rec.items["001"]=it; }
    { DecodedItem it; it.fields["TOD"]=43200u*128u; rec.items["002"]=it; }
    { DecodedItem it; it.fields["PREVIOUSII"]=3; it.fields["CURRENTII"]=5; rec.items["004"]=it; }
    { DecodedItem it; it.fields["ADR"]=0xABCDEFu; rec.items["005"]=it; }
    { DecodedItem it; it.fields["UM"]=1; it.fields["DM"]=0; it.fields["UC"]=0; it.fields["DC"]=0; rec.items["007"]=it; }
    { DecodedItem it; it.fields["COM"]=1; rec.items["010"]=it; }

    auto encoded = codec.encode(18, {rec});
    hexdump(encoded, "CoreFixed encoded");
    CHECK(!encoded.empty(), "encode produced output");
    CHECK(encoded[0] == 0x12, "CAT byte = 0x12 (18)");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("036").fields.at("SAC")        == 10,           "036/SAC = 10");
    CHECK(items.at("036").fields.at("SIC")        == 20,           "036/SIC = 20");
    CHECK(items.at("037").fields.at("SAC")        == 11,           "037/SAC = 11");
    CHECK(items.at("037").fields.at("SIC")        == 21,           "037/SIC = 21");
    CHECK(items.at("000").fields.at("MT")         == 5,            "000/MT = 5 (Keep_alive)");
    CHECK(items.at("001").fields.at("CAUSE")      == 3,            "001/CAUSE = 3");
    CHECK(items.at("001").fields.at("DIAG")       == 2,            "001/DIAG = 2");
    CHECK(items.at("002").fields.at("TOD")        == 43200u*128u,  "002/TOD = 12h");
    CHECK(items.at("004").fields.at("PREVIOUSII") == 3,            "004/PREVIOUSII = 3");
    CHECK(items.at("004").fields.at("CURRENTII")  == 5,            "004/CURRENTII = 5");
    CHECK(items.at("005").fields.at("ADR")        == 0xABCDEFu,    "005/ADR = 0xABCDEF");
    CHECK(items.at("007").fields.at("UM")         == 1,            "007/UM = 1");
    CHECK(items.at("007").fields.at("UC")         == 0,            "007/UC = 0 (enabled)");
    CHECK(items.at("010").fields.at("COM")        == 1,            "010/COM = 1");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip more Fixed items (FSPEC bytes 2, 4, 5)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripMoreFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip more Fixed items ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["036"]=it; }
    { DecodedItem it; it.fields["SAC"]=2; it.fields["SIC"]=2; rec.items["037"]=it; }
    { DecodedItem it; it.fields["MT"]=16; rec.items["000"]=it; }

    // FSPEC byte 1 extra
    { DecodedItem it; it.fields["PN"]=0x12345678u; rec.items["016"]=it; }
    // FSPEC byte 2
    { DecodedItem it; it.fields["PR"]=7; it.fields["PT"]=1; rec.items["018"]=it; }
    // FSPEC byte 4
    { DecodedItem it; it.fields["BN"]=0x0A0B0C0Du; rec.items["020"]=it; }
    { DecodedItem it; it.fields["PRIORITY"]=3; it.fields["POWER"]=7; it.fields["DURATION"]=30; it.fields["COVERAGE"]=0x01020304u; rec.items["021"]=it; }
    { DecodedItem it; it.fields["PREFIX"]=0x1234567u; rec.items["022"]=it; }
    { DecodedItem it; it.fields["MSG"]=0x01020304050607ULL; rec.items["023"]=it; }
    // FSPEC byte 5
    { DecodedItem it; it.fields["V"]=0; it.fields["G"]=0; it.fields["L"]=0; it.fields["MOD3A"]=0x123u; rec.items["032"]=it; }
    { DecodedItem it; it.fields["V"]=0; it.fields["G"]=0; it.fields["FL"]=400; rec.items["033"]=it; }
    { DecodedItem it; it.fields["GS"]=1000; rec.items["034"]=it; }
    { DecodedItem it; it.fields["HDG"]=8192; rec.items["035"]=it; }
    { DecodedItem it; it.fields["FS"]=0; it.fields["CQF"]=100; rec.items["012"]=it; }
    { DecodedItem it; it.fields["METHOD"]=5; rec.items["013"]=it; }

    auto encoded = codec.encode(18, {rec});
    hexdump(encoded, "MoreFixed encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("016").fields.at("PN")       == 0x12345678u,          "016/PN = 0x12345678");
    CHECK(items.at("018").fields.at("PR")       == 7,                    "018/PR = 7");
    CHECK(items.at("018").fields.at("PT")       == 1,                    "018/PT = 1");
    CHECK(items.at("020").fields.at("BN")       == 0x0A0B0C0Du,          "020/BN = 0x0A0B0C0D");
    CHECK(items.at("021").fields.at("PRIORITY") == 3,                    "021/PRIORITY = 3");
    CHECK(items.at("021").fields.at("POWER")    == 7,                    "021/POWER = 7");
    CHECK(items.at("021").fields.at("DURATION") == 30,                   "021/DURATION = 30s");
    CHECK(items.at("021").fields.at("COVERAGE") == 0x01020304u,          "021/COVERAGE = 0x01020304");
    CHECK(items.at("022").fields.at("PREFIX")   == 0x1234567u,           "022/PREFIX = 0x1234567");
    CHECK(items.at("023").fields.at("MSG")      == 0x01020304050607ULL,  "023/MSG round-trip");
    CHECK(items.at("032").fields.at("MOD3A")    == 0x123u,               "032/MOD3A = 0x123");
    CHECK(items.at("033").fields.at("FL")       == 400,                  "033/FL = 400 (FL100)");
    CHECK(items.at("034").fields.at("GS")       == 1000,                 "034/GS = 1000");
    CHECK(items.at("035").fields.at("HDG")      == 8192,                 "035/HDG = 8192");
    CHECK(items.at("012").fields.at("FS")       == 0,                    "012/FS = 0 (airborne)");
    CHECK(items.at("012").fields.at("CQF")      == 100,                  "012/CQF = 100");
    CHECK(items.at("013").fields.at("METHOD")   == 5,                    "013/METHOD = 5");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip Extended I008 (2 octets)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtendedI008(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Extended I008 (Aircraft Data Link Status) ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=5; it.fields["SIC"]=6; rec.items["036"]=it; }
    { DecodedItem it; it.fields["SAC"]=7; it.fields["SIC"]=8; rec.items["037"]=it; }
    { DecodedItem it; it.fields["MT"]=16; rec.items["000"]=it; }

    // I008: Octet1 UDS=1 DDS=0 UCS=1 DCS=0 EI=1 | Octet2 IC=1
    { DecodedItem it;
      it.fields["UDS"]=1; it.fields["DDS"]=0; it.fields["UCS"]=1;
      it.fields["DCS"]=0; it.fields["EI"]=1;
      it.fields["IC"]=1;
      rec.items["008"]=it; }

    auto encoded = codec.encode(18, {rec});
    hexdump(encoded, "Extended I008 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.count("008"), "I008 present");
    const auto& f = items.at("008").fields;
    CHECK(f.at("UDS") == 1, "008/UDS = 1 (disabled uplink)");
    CHECK(f.at("DDS") == 0, "008/DDS = 0 (enabled extract)");
    CHECK(f.at("UCS") == 1, "008/UCS = 1");
    CHECK(f.at("DCS") == 0, "008/DCS = 0");
    CHECK(f.at("EI")  == 1, "008/EI = 1 (not in coverage)");
    CHECK(f.at("IC")  == 1, "008/IC = 1 (cannot change)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip Extended I009 (2 octets)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtendedI009(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Extended I009 (Aircraft Data Link Report Request) ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; rec.items["036"]=it; }
    { DecodedItem it; it.fields["SAC"]=3; it.fields["SIC"]=4; rec.items["037"]=it; }
    { DecodedItem it; it.fields["MT"]=17; rec.items["000"]=it; }

    // I009: Octet1 SR=1 AR=1 ER=0 FR=1 MR=0 PR=0 CR=1 | Octet2 ID=1 MA=0 SP=1 HG=0 HD=1
    { DecodedItem it;
      it.fields["SR"]=1; it.fields["AR"]=1; it.fields["ER"]=0;
      it.fields["FR"]=1; it.fields["MR"]=0; it.fields["PR"]=0; it.fields["CR"]=1;
      it.fields["ID"]=1; it.fields["MA"]=0; it.fields["SP"]=1;
      it.fields["HG"]=0; it.fields["HD"]=1;
      rec.items["009"]=it; }

    auto encoded = codec.encode(18, {rec});
    hexdump(encoded, "Extended I009 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.count("009"), "I009 present");
    const auto& f = items.at("009").fields;
    CHECK(f.at("SR") == 1, "009/SR = 1 (status required)");
    CHECK(f.at("AR") == 1, "009/AR = 1 (COM required)");
    CHECK(f.at("ER") == 0, "009/ER = 0 (ECA not required)");
    CHECK(f.at("FR") == 1, "009/FR = 1 (CQF required)");
    CHECK(f.at("CR") == 1, "009/CR = 1 (Cartesian pos required)");
    CHECK(f.at("ID") == 1, "009/ID = 1 (aircraft ID required)");
    CHECK(f.at("SP") == 1, "009/SP = 1 (speed required)");
    CHECK(f.at("HG") == 0, "009/HG = 0 (height not required)");
    CHECK(f.at("HD") == 1, "009/HD = 1 (heading required)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip RepetitiveGroup I006 and I017
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripRepGroups(Codec& codec) {
    std::cout << "\n=== Test: Round-trip RepetitiveGroup I006 and I017 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; rec.items["036"]=it; }
    { DecodedItem it; it.fields["SAC"]=3; it.fields["SIC"]=4; rec.items["037"]=it; }
    { DecodedItem it; it.fields["MT"]=32; rec.items["000"]=it; }

    // I006: address list – 2 entries (ADR 24-bit each)
    DecodedItem i006;
    i006.group_repetitions.push_back({{"ADR", 0x3C1234u}});
    i006.group_repetitions.push_back({{"ADR", 0xABCDEFu}});
    rec.items["006"] = i006;

    // I017: packet number list – 3 entries (PN 32-bit each)
    DecodedItem i017;
    i017.group_repetitions.push_back({{"PN", 100u}});
    i017.group_repetitions.push_back({{"PN", 200u}});
    i017.group_repetitions.push_back({{"PN", 0xDEADBEEFu}});
    rec.items["017"] = i017;

    auto encoded = codec.encode(18, {rec});
    hexdump(encoded, "RepGroups encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;

    CHECK(items.count("006"), "I006 present");
    const auto& gr006 = items.at("006").group_repetitions;
    CHECK(gr006.size() == 2,               "006 has 2 entries");
    CHECK(gr006[0].at("ADR") == 0x3C1234u, "006 rep[0] ADR = 0x3C1234");
    CHECK(gr006[1].at("ADR") == 0xABCDEFu, "006 rep[1] ADR = 0xABCDEF");

    CHECK(items.count("017"), "I017 present");
    const auto& gr017 = items.at("017").group_repetitions;
    CHECK(gr017.size() == 3,                   "017 has 3 entries");
    CHECK(gr017[0].at("PN") == 100u,           "017 rep[0] PN = 100");
    CHECK(gr017[1].at("PN") == 200u,           "017 rep[1] PN = 200");
    CHECK(gr017[2].at("PN") == 0xDEADBEEFu,    "017 rep[2] PN = 0xDEADBEEF");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Round-trip position and capability items
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripPositionItems(Codec& codec) {
    std::cout << "\n=== Test: Round-trip I014, I015, I011 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["036"]=it; }
    { DecodedItem it; it.fields["SAC"]=2; it.fields["SIC"]=2; rec.items["037"]=it; }
    { DecodedItem it; it.fields["MT"]=67; rec.items["000"]=it; }

    // I014: polar position
    { DecodedItem it; it.fields["RHO"]=10000; it.fields["THETA"]=8000; rec.items["014"]=it; }
    // I015: Cartesian position (Y negative)
    { DecodedItem it; it.fields["X"]=1024; it.fields["Y"]=s16(-512); rec.items["015"]=it; }
    // I011: 7-byte capability report
    { DecodedItem it; it.fields["CAP"]=0xAABBCCDDEEFF11ULL; rec.items["011"]=it; }

    auto encoded = codec.encode(18, {rec});
    hexdump(encoded, "PositionItems encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.count("014"),                                             "I014 present");
    CHECK(items.at("014").fields.at("RHO")   == 10000,                   "014/RHO = 10000");
    CHECK(items.at("014").fields.at("THETA") == 8000,                    "014/THETA = 8000");
    CHECK(items.count("015"),                                             "I015 present");
    CHECK(items.at("015").fields.at("X")     == 1024,                    "015/X = 1024");
    CHECK(items.at("015").fields.at("Y")     == s16(-512),               "015/Y = -512");
    CHECK(items.count("011"),                                             "I011 present");
    CHECK(items.at("011").fields.at("CAP")   == 0xAABBCCDDEEFF11ULL,     "011/CAP round-trip");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 9: Round-trip GICB items
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripGICB(Codec& codec) {
    std::cout << "\n=== Test: Round-trip GICB items I025, I027, I028, I029, I030, I031 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; rec.items["036"]=it; }
    { DecodedItem it; it.fields["SAC"]=3; it.fields["SIC"]=4; rec.items["037"]=it; }
    { DecodedItem it; it.fields["MT"]=64; rec.items["000"]=it; }

    { DecodedItem it; it.fields["GN"]=42u; rec.items["025"]=it; }
    { DecodedItem it; it.fields["BDS"]=0x50u; rec.items["027"]=it; }
    { DecodedItem it; it.fields["PERIOD"]=10u; rec.items["028"]=it; }
    { DecodedItem it; it.fields["GICB"]=0x01020304050607ULL; rec.items["029"]=it; }
    { DecodedItem it;
      it.fields["PRIORITY"]=7; it.fields["PC"]=1;
      it.fields["AU"]=0; it.fields["NE"]=0; it.fields["RD"]=2;
      rec.items["030"]=it; }
    { DecodedItem it; it.fields["ID"]=0x303132333435ULL; rec.items["031"]=it; }

    auto encoded = codec.encode(18, {rec});
    hexdump(encoded, "GICB encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("025").fields.at("GN")       == 42u,                    "025/GN = 42");
    CHECK(items.at("027").fields.at("BDS")      == 0x50u,                  "027/BDS = 0x50");
    CHECK(items.at("028").fields.at("PERIOD")   == 10u,                    "028/PERIOD = 10s");
    CHECK(items.at("029").fields.at("GICB")     == 0x01020304050607ULL,    "029/GICB round-trip");
    CHECK(items.at("030").fields.at("PRIORITY") == 7,                      "030/PRIORITY = 7");
    CHECK(items.at("030").fields.at("PC")       == 1,                      "030/PC = 1");
    CHECK(items.at("030").fields.at("AU")       == 0,                      "030/AU = 0");
    CHECK(items.at("030").fields.at("NE")       == 0,                      "030/NE = 0");
    CHECK(items.at("030").fields.at("RD")       == 2,                      "030/RD = 2");
    CHECK(items.at("031").fields.at("ID")       == 0x303132333435ULL,      "031/ID round-trip");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 10: Full round-trip – complete Mode S Datalink message
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip Mode S Datalink message ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=0; it.fields["SIC"]=7; rec.items["036"]=it; }
    { DecodedItem it; it.fields["SAC"]=0; it.fields["SIC"]=8; rec.items["037"]=it; }
    { DecodedItem it; it.fields["MT"]=35; rec.items["000"]=it; }
    { DecodedItem it; it.fields["TOD"]=36000u*128u; rec.items["002"]=it; }
    { DecodedItem it; it.fields["ADR"]=0x3C4567u; rec.items["005"]=it; }
    { DecodedItem it; it.fields["COM"]=3; rec.items["010"]=it; }

    { DecodedItem it;
      it.fields["UDS"]=0; it.fields["DDS"]=0; it.fields["UCS"]=0;
      it.fields["DCS"]=0; it.fields["EI"]=1; it.fields["IC"]=1;
      rec.items["008"]=it; }

    { DecodedItem it;
      it.fields["SR"]=1; it.fields["AR"]=0; it.fields["ER"]=0;
      it.fields["FR"]=1; it.fields["MR"]=0; it.fields["PR"]=1; it.fields["CR"]=1;
      it.fields["ID"]=0; it.fields["MA"]=1; it.fields["SP"]=0;
      it.fields["HG"]=1; it.fields["HD"]=0;
      rec.items["009"]=it; }

    DecodedItem i006;
    i006.group_repetitions.push_back({{"ADR", 0x3C4567u}});
    rec.items["006"] = i006;

    { DecodedItem it; it.fields["RHO"]=5000; it.fields["THETA"]=16384; rec.items["014"]=it; }
    { DecodedItem it; it.fields["X"]=512; it.fields["Y"]=256; rec.items["015"]=it; }
    { DecodedItem it; it.fields["PN"]=0xCAFEBABEu; rec.items["016"]=it; }

    DecodedItem i017;
    i017.group_repetitions.push_back({{"PN", 0xCAFEBABEu}});
    rec.items["017"] = i017;

    { DecodedItem it; it.fields["V"]=0; it.fields["G"]=0; it.fields["L"]=0; it.fields["MOD3A"]=0x777u; rec.items["032"]=it; }
    { DecodedItem it; it.fields["GS"]=4096; rec.items["034"]=it; }

    auto encoded = codec.encode(18, {rec});
    hexdump(encoded, "Full round-trip encoded");
    CHECK(!encoded.empty(), "encode produced output");
    CHECK(encoded[0] == 0x12, "CAT byte = 0x12 (18)");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("036").fields.at("SAC") == 0,               "036/SAC = 0");
    CHECK(items.at("036").fields.at("SIC") == 7,               "036/SIC = 7");
    CHECK(items.at("000").fields.at("MT")  == 35,              "000/MT = 35 (Downlink_packet)");
    CHECK(items.at("002").fields.at("TOD") == 36000u*128u,     "002/TOD = 10h");
    CHECK(items.at("005").fields.at("ADR") == 0x3C4567u,       "005/ADR = 0x3C4567");
    CHECK(items.at("010").fields.at("COM") == 3,               "010/COM = 3");
    CHECK(items.count("008"),                                   "I008 present");
    CHECK(items.at("008").fields.at("EI") == 1,                "008/EI = 1 (not in coverage)");
    CHECK(items.at("008").fields.at("IC") == 1,                "008/IC = 1 (cannot change)");
    CHECK(items.count("009"),                                   "I009 present");
    CHECK(items.at("009").fields.at("SR") == 1,                "009/SR = 1");
    CHECK(items.at("009").fields.at("FR") == 1,                "009/FR = 1");
    CHECK(items.at("009").fields.at("MA") == 1,                "009/MA = 1");
    CHECK(items.count("006"),                                   "I006 present");
    CHECK(items.at("006").group_repetitions.size() == 1,        "006 has 1 entry");
    CHECK(items.at("006").group_repetitions[0].at("ADR") == 0x3C4567u, "006/ADR round-trip");
    CHECK(items.at("014").fields.at("RHO")   == 5000,          "014/RHO = 5000");
    CHECK(items.at("014").fields.at("THETA") == 16384,         "014/THETA = 16384");
    CHECK(items.at("015").fields.at("X")     == 512,           "015/X = 512");
    CHECK(items.at("015").fields.at("Y")     == 256,           "015/Y = 256");
    CHECK(items.at("016").fields.at("PN")    == 0xCAFEBABEu,   "016/PN = 0xCAFEBABE");
    CHECK(items.count("017"),                                   "I017 present");
    CHECK(items.at("017").group_repetitions[0].at("PN") == 0xCAFEBABEu, "017/PN round-trip");
    CHECK(items.at("032").fields.at("MOD3A") == 0x777u,        "032/MOD3A round-trip");
    CHECK(items.at("034").fields.at("GS")    == 4096,          "034/GS = 4096");
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = (argc > 1) ? argv[1] : "specs/CAT18.xml";

    Codec codec;
    testSpecLoad(codec, spec_path);
    testDecodeBasicMessage(codec);
    testRoundTripCoreFixed(codec);
    testRoundTripMoreFixed(codec);
    testRoundTripExtendedI008(codec);
    testRoundTripExtendedI009(codec);
    testRoundTripRepGroups(codec);
    testRoundTripPositionItems(codec);
    testRoundTripGICB(codec);
    testFullRoundTrip(codec);

    std::cout << "\n=== Results: "
              << (failures == 0 ? "ALL PASSED" : std::to_string(failures) + " FAILED")
              << " ===\n";
    return failures == 0 ? 0 : 1;
}
