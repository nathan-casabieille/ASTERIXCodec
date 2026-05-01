// test_cat25.cpp – Tests for CAT25 CNS/ATM Ground System Status Reports, Ed. 1.6.

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
static uint64_t s32(int32_t v) { return static_cast<uint64_t>(static_cast<uint32_t>(v)); }

// ─────────────────────────────────────────────────────────────────────────────
//  Test 1: Spec load
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT25 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 25,        "cat number = 25");
    CHECK(cat.edition == "1.6", "edition = 1.6");

    for (auto id : {"000","010","015","020","070","100","105","120","140","200","600","610","SP"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 13, "13 items total");

    CHECK(cat.uap_variations.count("default"), "UAP 'default' exists");
    CHECK(!cat.uap_case.has_value(),            "no UAP case discriminator");
    const auto& uap = cat.uap_variations.at("default");
    CHECK(uap.size() == 13, "UAP has 13 slots");

    CHECK(uap[0]  == "010", "UAP slot  1 = 010");
    CHECK(uap[1]  == "000", "UAP slot  2 = 000");
    CHECK(uap[2]  == "200", "UAP slot  3 = 200");
    CHECK(uap[3]  == "015", "UAP slot  4 = 015");
    CHECK(uap[4]  == "020", "UAP slot  5 = 020");
    CHECK(uap[5]  == "070", "UAP slot  6 = 070");
    CHECK(uap[6]  == "100", "UAP slot  7 = 100");
    CHECK(uap[7]  == "105", "UAP slot  8 = 105");
    CHECK(uap[8]  == "120", "UAP slot  9 = 120");
    CHECK(uap[9]  == "140", "UAP slot 10 = 140");
    CHECK(uap[10] == "SP",  "UAP slot 11 = SP");
    CHECK(uap[11] == "600", "UAP slot 12 = 600");
    CHECK(uap[12] == "610", "UAP slot 13 = 610");

    // Item types
    for (auto id : {"000","010","015","020","070","200","600","610"}) {
        CHECK(cat.items.at(id).type == ItemType::Fixed,
              std::string(id) + " is Fixed");
    }
    CHECK(cat.items.at("100").type == ItemType::Extended,        "100 is Extended");
    CHECK(cat.items.at("105").type == ItemType::RepetitiveGroup, "105 is RepetitiveGroup");
    CHECK(cat.items.at("120").type == ItemType::RepetitiveGroup, "120 is RepetitiveGroup");
    CHECK(cat.items.at("140").type == ItemType::RepetitiveGroup, "140 is RepetitiveGroup");
    CHECK(cat.items.at("SP").type  == ItemType::SP,              "SP is SP/Explicit");

    // Fixed sizes
    CHECK(cat.items.at("000").fixed_bytes == 1, "000 = 1 byte");
    CHECK(cat.items.at("010").fixed_bytes == 2, "010 = 2 bytes");
    CHECK(cat.items.at("015").fixed_bytes == 1, "015 = 1 byte");
    CHECK(cat.items.at("020").fixed_bytes == 6, "020 = 6 bytes");
    CHECK(cat.items.at("070").fixed_bytes == 3, "070 = 3 bytes");
    CHECK(cat.items.at("200").fixed_bytes == 3, "200 = 3 bytes");
    CHECK(cat.items.at("600").fixed_bytes == 8, "600 = 8 bytes");
    CHECK(cat.items.at("610").fixed_bytes == 2, "610 = 2 bytes");

    // Extended octet count
    CHECK(cat.items.at("100").octets.size() == 2, "100 has 2 octets");

    // RepetitiveGroup bits per entry
    CHECK(cat.items.at("105").rep_group_bits == 8,  "105 rep_group_bits = 8");
    CHECK(cat.items.at("120").rep_group_bits == 24, "120 rep_group_bits = 24");
    CHECK(cat.items.at("140").rep_group_bits == 48, "140 rep_group_bits = 48");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode a minimal message from raw bytes
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeBasicMessage(Codec& codec) {
    std::cout << "\n=== Test: Decode basic CAT25 message ===\n";

    // UAP byte 1: I010(bit7)=1 I020(bit3)=0 I070(bit2)=1 → 0x84, FX=0
    // Frame: CAT(1)+LEN(2)+FSPEC(1)+I010(2)+I070(3) = 9 bytes
    // I070: TOD = 9h = 9*3600 s → raw = 9*3600*128 = 4147200 = 0x3F4800
    std::vector<uint8_t> frame = {
        0x19,             // CAT = 25
        0x00, 0x09,       // LEN = 9
        0x84,             // FSPEC: I010(slot1)+I070(slot6), FX=0
        0x01, 0x02,       // I010: SAC=1, SIC=2
        0x3F, 0x48, 0x00  // I070: TOD = 9h
    };

    hexdump(frame, "Basic message input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid,                "block is valid");
    CHECK(blk.records.size() == 1,  "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"),  "I010 present");
    CHECK(rec.items.count("070"),  "I070 present");
    CHECK(!rec.items.count("000"), "I000 absent");
    CHECK(!rec.items.count("100"), "I100 absent");

    CHECK(rec.items.at("010").fields.at("SAC") == 1,           "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC") == 2,           "I010.SIC=2");
    CHECK(rec.items.at("070").fields.at("TOD") == 9u*3600u*128u, "I070.TOD=9h");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip core Fixed items
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCoreFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip core Fixed items ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    // I010: Data Source Identifier
    { DecodedItem it; it.fields["SAC"]=5; it.fields["SIC"]=10; rec.items["010"]=it; }
    // I000: Report Type – RTYP=1 (Service Status), RG=0 (Periodic)
    { DecodedItem it; it.fields["RTYP"]=1; it.fields["RG"]=0; rec.items["000"]=it; }
    // I200: Message Identification
    { DecodedItem it; it.fields["MID"]=0x123456; rec.items["200"]=it; }
    // I070: Time of Day (12h)
    { DecodedItem it; it.fields["TOD"]=12u*3600u*128u; rec.items["070"]=it; }

    auto encoded = codec.encode(25, {rec});
    hexdump(encoded, "CoreFixed encoded");
    CHECK(!encoded.empty(), "encode produced output");
    CHECK(encoded[0] == 0x19, "CAT byte = 0x19 (25)");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("SAC")  == 5,          "010/SAC = 5");
    CHECK(items.at("010").fields.at("SIC")  == 10,         "010/SIC = 10");
    CHECK(items.at("000").fields.at("RTYP") == 1,          "000/RTYP = 1 (Service Status)");
    CHECK(items.at("000").fields.at("RG")   == 0,          "000/RG = 0 (Periodic)");
    CHECK(items.at("200").fields.at("MID")  == 0x123456,   "200/MID = 0x123456");
    CHECK(items.at("070").fields.at("TOD")  == 12u*3600u*128u, "070/TOD = 12h");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip remaining Fixed items (I015, I020, I600, I610)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripMoreFixed(Codec& codec) {
    std::cout << "\n=== Test: Round-trip more Fixed items ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; rec.items["010"]=it; }
    { DecodedItem it; it.fields["TOD"]=1000; rec.items["070"]=it; }

    // I015: Service Identification
    { DecodedItem it; it.fields["SID"]=0xAB; rec.items["015"]=it; }

    // I020: Service Designator (48-bit raw, e.g. "1090ADS" packed as ICAO-6)
    { DecodedItem it; it.fields["DES"]=0x313233343536ULL; rec.items["020"]=it; }

    // I600: WGS-84 position
    // LAT = 2^30 raw → 1073741824 × 180/2^32 = 45.0°
    // LON = 2^27 raw → 134217728  × 180/2^32 = 5.625°
    { DecodedItem it;
      it.fields["LAT"]=s32(1073741824);
      it.fields["LON"]=s32(134217728);
      rec.items["600"]=it; }

    // I610: Height (signed 16-bit, 0.25 m/LSB)
    // -200 raw → -200 × 0.25 = -50 m
    { DecodedItem it; it.fields["HGT"]=s16(-200); rec.items["610"]=it; }

    auto encoded = codec.encode(25, {rec});
    hexdump(encoded, "MoreFixed encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("015").fields.at("SID")  == 0xAB,               "015/SID = 0xAB");
    CHECK(items.at("020").fields.at("DES")  == 0x313233343536ULL,  "020/DES round-trip");
    CHECK(items.at("600").fields.at("LAT")  == s32(1073741824),    "600/LAT round-trip");
    CHECK(items.at("600").fields.at("LON")  == s32(134217728),     "600/LON round-trip");
    CHECK(items.at("610").fields.at("HGT")  == s16(-200),          "610/HGT = -50 m");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip Extended I100 (2 octets)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtendedI100(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Extended I100 (2 octets) ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; rec.items["010"]=it; }
    { DecodedItem it; it.fields["TOD"]=500; rec.items["070"]=it; }

    // I100: set fields in both octets
    // Octet 1: NOGO=1, OPS=2 (Maintenance), SSTAT=2 (Degraded)
    // Octet 2: SYSTAT=1 (Failed), SESTAT=2 (Degraded) — non-zero forces 2nd octet emission
    { DecodedItem it;
      it.fields["NOGO"]  = 1;
      it.fields["OPS"]   = 2;
      it.fields["SSTAT"] = 2;
      it.fields["SYSTAT"] = 1;
      it.fields["SESTAT"] = 2;
      rec.items["100"] = it; }

    auto encoded = codec.encode(25, {rec});
    hexdump(encoded, "Extended I100 (2 octets) encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.count("100"), "I100 present");
    const auto& f = items.at("100").fields;
    CHECK(f.at("NOGO")   == 1, "100/NOGO = 1 (no-go)");
    CHECK(f.at("OPS")    == 2, "100/OPS = 2 (Maintenance)");
    CHECK(f.at("SSTAT")  == 2, "100/SSTAT = 2 (Degraded)");
    CHECK(f.at("SYSTAT") == 1, "100/SYSTAT = 1 (Failed)");
    CHECK(f.at("SESTAT") == 2, "100/SESTAT = 2 (Degraded)");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip RepetitiveGroup I105, I120, I140
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripRepGroups(Codec& codec) {
    std::cout << "\n=== Test: Round-trip RepetitiveGroup I105, I120, I140 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=2; rec.items["010"]=it; }
    { DecodedItem it; it.fields["TOD"]=200; rec.items["070"]=it; }

    // I105: 2 error codes (8-bit each)
    DecodedItem i105;
    i105.group_repetitions.push_back({{"EC", 2}});  // Time Source Invalid
    i105.group_repetitions.push_back({{"EC", 5}});  // Data Processor Overload
    rec.items["105"] = i105;

    // I120: 2 component status entries (24-bit each: CID(16)+ERRC(6)+CS(2))
    DecodedItem i120;
    i120.group_repetitions.push_back({{"CID", 0x0101u}, {"ERRC", 2}, {"CS", 1}});  // Alert, Failed
    i120.group_repetitions.push_back({{"CID", 0x0202u}, {"ERRC", 3}, {"CS", 0}});  // Alarm, Running
    rec.items["120"] = i120;

    // I140: 2 service statistics entries (48-bit each: TYPE(8)+REF(1)+spare(7)+COUNT(32))
    DecodedItem i140;
    i140.group_repetitions.push_back({{"TYPE", 3}, {"REF", 0}, {"COUNT", 1000u}});  // Total rx, UTC
    i140.group_repetitions.push_back({{"TYPE", 4}, {"REF", 1}, {"COUNT",  500u}});  // Total tx, prev
    rec.items["140"] = i140;

    auto encoded = codec.encode(25, {rec});
    hexdump(encoded, "RepGroups encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;

    // I105
    CHECK(items.count("105"), "I105 present");
    const auto& gr105 = items.at("105").group_repetitions;
    CHECK(gr105.size() == 2,           "105 has 2 entries");
    CHECK(gr105[0].at("EC") == 2,      "105 rep[0] EC = 2 (Time Source Invalid)");
    CHECK(gr105[1].at("EC") == 5,      "105 rep[1] EC = 5 (Data Processor Overload)");

    // I120
    CHECK(items.count("120"), "I120 present");
    const auto& gr120 = items.at("120").group_repetitions;
    CHECK(gr120.size() == 2,               "120 has 2 entries");
    CHECK(gr120[0].at("CID")  == 0x0101u,  "120 rep[0] CID = 0x0101");
    CHECK(gr120[0].at("ERRC") == 2,        "120 rep[0] ERRC = 2 (Alert)");
    CHECK(gr120[0].at("CS")   == 1,        "120 rep[0] CS = 1 (Failed)");
    CHECK(gr120[1].at("CID")  == 0x0202u,  "120 rep[1] CID = 0x0202");
    CHECK(gr120[1].at("ERRC") == 3,        "120 rep[1] ERRC = 3 (Alarm)");
    CHECK(gr120[1].at("CS")   == 0,        "120 rep[1] CS = 0 (Running)");

    // I140
    CHECK(items.count("140"), "I140 present");
    const auto& gr140 = items.at("140").group_repetitions;
    CHECK(gr140.size() == 2,             "140 has 2 entries");
    CHECK(gr140[0].at("TYPE")  == 3,     "140 rep[0] TYPE = 3 (Total rx)");
    CHECK(gr140[0].at("REF")   == 0,     "140 rep[0] REF = 0 (UTC midnight)");
    CHECK(gr140[0].at("COUNT") == 1000u, "140 rep[0] COUNT = 1000");
    CHECK(gr140[1].at("TYPE")  == 4,     "140 rep[1] TYPE = 4 (Total tx)");
    CHECK(gr140[1].at("REF")   == 1,     "140 rep[1] REF = 1 (previous report)");
    CHECK(gr140[1].at("COUNT") == 500u,  "140 rep[1] COUNT = 500");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Full round-trip – CNS/ATM ground system status report
// ─────────────────────────────────────────────────────────────────────────────
static void testFullRoundTrip(Codec& codec) {
    std::cout << "\n=== Test: Full round-trip CNS/ATM ground system status report ===\n";

    DecodedRecord rec;
    rec.uap_variation = "default";

    { DecodedItem it; it.fields["SAC"]=0; it.fields["SIC"]=7; rec.items["010"]=it; }
    // I000: Report Type 2 (Component Status), Event Driven
    { DecodedItem it; it.fields["RTYP"]=2; it.fields["RG"]=1; rec.items["000"]=it; }
    // I200: Message Identification
    { DecodedItem it; it.fields["MID"]=0xABCDEF; rec.items["200"]=it; }
    // I015: Service Identification
    { DecodedItem it; it.fields["SID"]=0x55; rec.items["015"]=it; }
    // I070: Time of Day (10h)
    { DecodedItem it; it.fields["TOD"]=10u*3600u*128u; rec.items["070"]=it; }
    // I100: NOGO=0, OPS=1 (Standby), SSTAT=0; SYSTAT=2 (Degraded) forces 2nd octet
    { DecodedItem it;
      it.fields["NOGO"]  = 0;
      it.fields["OPS"]   = 1;
      it.fields["SSTAT"] = 0;
      it.fields["SYSTAT"] = 2;
      it.fields["SESTAT"] = 0;
      rec.items["100"] = it; }
    // I105: 1 error code
    DecodedItem i105;
    i105.group_repetitions.push_back({{"EC", 3}});  // Time Source Coasting
    rec.items["105"] = i105;
    // I120: 1 component status entry
    DecodedItem i120;
    i120.group_repetitions.push_back({{"CID", 0xFF00u}, {"ERRC", 1}, {"CS", 2}});
    rec.items["120"] = i120;
    // I600: WGS-84 reference point position
    { DecodedItem it;
      it.fields["LAT"]=s32(1073741824);  // 45.0°
      it.fields["LON"]=s32(268435456);   // 11.25°
      rec.items["600"]=it; }
    // I610: Height above sea level (400 raw × 0.25 = 100 m)
    { DecodedItem it; it.fields["HGT"]=400; rec.items["610"]=it; }

    auto encoded = codec.encode(25, {rec});
    hexdump(encoded, "Full round-trip encoded");
    CHECK(!encoded.empty(), "encode produced output");
    CHECK(encoded[0] == 0x19, "CAT byte = 0x19 (25)");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("SAC")   == 0,              "010/SAC = 0");
    CHECK(items.at("010").fields.at("SIC")   == 7,              "010/SIC = 7");
    CHECK(items.at("000").fields.at("RTYP")  == 2,              "000/RTYP = 2 (Component Status)");
    CHECK(items.at("000").fields.at("RG")    == 1,              "000/RG = 1 (Event Driven)");
    CHECK(items.at("200").fields.at("MID")   == 0xABCDEFu,      "200/MID = 0xABCDEF");
    CHECK(items.at("015").fields.at("SID")   == 0x55,           "015/SID = 0x55");
    CHECK(items.at("070").fields.at("TOD")   == 10u*3600u*128u, "070/TOD = 10h");
    CHECK(items.count("100"),                                    "I100 present");
    CHECK(items.at("100").fields.at("OPS")   == 1,              "100/OPS = 1 (Standby)");
    CHECK(items.at("100").fields.at("SYSTAT") == 2,             "100/SYSTAT = 2 (Degraded)");
    CHECK(items.count("105"),                                    "I105 present");
    CHECK(items.at("105").group_repetitions.size() == 1,         "105 has 1 entry");
    CHECK(items.at("105").group_repetitions[0].at("EC") == 3,    "105/EC = 3 (Time Source Coasting)");
    CHECK(items.count("120"),                                    "I120 present");
    CHECK(items.at("120").group_repetitions.size() == 1,         "120 has 1 entry");
    CHECK(items.at("120").group_repetitions[0].at("CID") == 0xFF00u, "120/CID = 0xFF00");
    CHECK(items.at("120").group_repetitions[0].at("CS")  == 2,   "120/CS = 2 (Maintenance)");
    CHECK(items.at("600").fields.at("LAT")   == s32(1073741824), "600/LAT round-trip");
    CHECK(items.at("600").fields.at("LON")   == s32(268435456),  "600/LON round-trip");
    CHECK(items.at("610").fields.at("HGT")   == 400,             "610/HGT = 100 m");
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = (argc > 1) ? argv[1] : "specs/CAT25.xml";

    Codec codec;
    testSpecLoad(codec, spec_path);
    testDecodeBasicMessage(codec);
    testRoundTripCoreFixed(codec);
    testRoundTripMoreFixed(codec);
    testRoundTripExtendedI100(codec);
    testRoundTripRepGroups(codec);
    testFullRoundTrip(codec);

    std::cout << "\n=== Results: "
              << (failures == 0 ? "ALL PASSED" : std::to_string(failures) + " FAILED")
              << " ===\n";
    return failures == 0 ? 0 : 1;
}
