// test_cat07.cpp – Tests for CAT07 Transmission of Directed Interrogation Messages.

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

// ─────────────────────────────────────────────────────────────────────────────
//  Test 1: XML spec loads correctly
// ─────────────────────────────────────────────────────────────────────────────
static void testSpecLoad(Codec& codec, const fs::path& spec_path) {
    std::cout << "\n=== Test: CAT07 spec load ===\n";
    CategoryDef cat = loadSpec(spec_path);

    CHECK(cat.cat == 7,          "cat number = 7");
    CHECK(cat.edition == "1.12", "edition = 1.12");

    for (auto id : {"010","020","025","030","040","042","050","055","060","065",
                    "070","080","085","090","100","110","120","130","140","161",
                    "170","200","210","220","230","240","250","260","400","410",
                    "415","420","440","450","REF","SPF"}) {
        CHECK(cat.items.count(id), std::string("item ") + id + " present");
    }
    CHECK(cat.items.size() == 36, "36 items total");

    // Dual UAP
    CHECK(cat.uap_variations.count("downlink"), "UAP 'downlink' exists");
    CHECK(cat.uap_variations.count("uplink"),   "UAP 'uplink' exists");
    CHECK(cat.uap_case.has_value(),             "UAP case discriminator present");
    CHECK(cat.uap_case->item_id  == "410",      "UAP case item_id = 410");
    CHECK(cat.uap_case->field    == "MSGTYP",   "UAP case field = MSGTYP");

    // Downlink: 35 slots
    const auto& dl = cat.uap_variations.at("downlink");
    CHECK(dl.size() == 35, "downlink UAP has 35 slots");
    CHECK(dl[0]  == "010", "DL slot  1 = 010");
    CHECK(dl[1]  == "025", "DL slot  2 = 025");
    CHECK(dl[2]  == "410", "DL slot  3 = 410");
    CHECK(dl[3]  == "140", "DL slot  4 = 140");
    CHECK(dl[4]  == "400", "DL slot  5 = 400");
    CHECK(dl[5]  == "020", "DL slot  6 = 020");
    CHECK(dl[6]  == "040", "DL slot  7 = 040");
    CHECK(dl[7]  == "070", "DL slot  8 = 070");
    CHECK(dl[13] == "161", "DL slot 14 = 161");
    CHECK(dl[14] == "042", "DL slot 15 = 042");
    CHECK(dl[28] == "060", "DL slot 29 = 060");
    CHECK(dl[29] == "450", "DL slot 30 = 450");
    CHECK(dl[30] == "085", "DL slot 31 = 085");
    CHECK(dl[33] == "SPF", "DL slot 34 = SPF");
    CHECK(dl[34] == "REF", "DL slot 35 = REF");

    // Uplink: 21 slots
    const auto& ul = cat.uap_variations.at("uplink");
    CHECK(ul.size() == 21, "uplink UAP has 21 slots");
    CHECK(ul[0]  == "010", "UL slot  1 = 010");
    CHECK(ul[2]  == "410", "UL slot  3 = 410");
    CHECK(ul[10] == "415", "UL slot 11 = 415");
    CHECK(ul[11] == "420", "UL slot 12 = 420");
    CHECK(ul[12] == "440", "UL slot 13 = 440");
    CHECK(ul[19] == "SPF", "UL slot 20 = SPF");
    CHECK(ul[20] == "REF", "UL slot 21 = REF");

    // Item types
    for (auto id : {"010","025","040","042","050","055","060","065",
                    "070","080","090","100","110","140","161","200",
                    "210","220","230","240","260","400","410","420"}) {
        CHECK(cat.items.at(id).type == ItemType::Fixed,
              std::string(id) + " is Fixed");
    }
    CHECK(cat.items.at("020").type == ItemType::Extended,       "020 is Extended");
    CHECK(cat.items.at("170").type == ItemType::Extended,       "170 is Extended");
    CHECK(cat.items.at("030").type == ItemType::Repetitive,     "030 is Repetitive (FX)");
    CHECK(cat.items.at("250").type == ItemType::RepetitiveGroup,"250 is RepetitiveGroup");
    CHECK(cat.items.at("440").type == ItemType::RepetitiveGroup,"440 is RepetitiveGroup");
    CHECK(cat.items.at("085").type == ItemType::Compound,       "085 is Compound");
    CHECK(cat.items.at("120").type == ItemType::Compound,       "120 is Compound");
    CHECK(cat.items.at("130").type == ItemType::Compound,       "130 is Compound");
    CHECK(cat.items.at("415").type == ItemType::Compound,       "415 is Compound");
    CHECK(cat.items.at("450").type == ItemType::Compound,       "450 is Compound");
    CHECK(cat.items.at("REF").type == ItemType::SP,             "REF is SP/Explicit");
    CHECK(cat.items.at("SPF").type == ItemType::SP,             "SPF is SP/Explicit");

    // Fixed byte sizes
    CHECK(cat.items.at("010").fixed_bytes == 2,  "010 = 2 bytes");
    CHECK(cat.items.at("025").fixed_bytes == 2,  "025 = 2 bytes");
    CHECK(cat.items.at("040").fixed_bytes == 4,  "040 = 4 bytes");
    CHECK(cat.items.at("042").fixed_bytes == 4,  "042 = 4 bytes");
    CHECK(cat.items.at("050").fixed_bytes == 2,  "050 = 2 bytes");
    CHECK(cat.items.at("055").fixed_bytes == 1,  "055 = 1 byte");
    CHECK(cat.items.at("060").fixed_bytes == 2,  "060 = 2 bytes");
    CHECK(cat.items.at("065").fixed_bytes == 1,  "065 = 1 byte");
    CHECK(cat.items.at("070").fixed_bytes == 2,  "070 = 2 bytes");
    CHECK(cat.items.at("080").fixed_bytes == 2,  "080 = 2 bytes");
    CHECK(cat.items.at("090").fixed_bytes == 2,  "090 = 2 bytes");
    CHECK(cat.items.at("100").fixed_bytes == 4,  "100 = 4 bytes");
    CHECK(cat.items.at("110").fixed_bytes == 2,  "110 = 2 bytes");
    CHECK(cat.items.at("140").fixed_bytes == 3,  "140 = 3 bytes");
    CHECK(cat.items.at("161").fixed_bytes == 2,  "161 = 2 bytes");
    CHECK(cat.items.at("200").fixed_bytes == 4,  "200 = 4 bytes");
    CHECK(cat.items.at("210").fixed_bytes == 4,  "210 = 4 bytes");
    CHECK(cat.items.at("220").fixed_bytes == 3,  "220 = 3 bytes");
    CHECK(cat.items.at("230").fixed_bytes == 2,  "230 = 2 bytes");
    CHECK(cat.items.at("240").fixed_bytes == 6,  "240 = 6 bytes");
    CHECK(cat.items.at("260").fixed_bytes == 7,  "260 = 7 bytes");
    CHECK(cat.items.at("400").fixed_bytes == 2,  "400 = 2 bytes");
    CHECK(cat.items.at("410").fixed_bytes == 1,  "410 = 1 byte");
    CHECK(cat.items.at("420").fixed_bytes == 8,  "420 = 8 bytes");

    // I020 / I170 Extended octets
    CHECK(cat.items.at("020").octets.size() == 6, "020 has 6 octets");
    CHECK(cat.items.at("170").octets.size() == 2, "170 has 2 octets");

    // I250 RepetitiveGroup: MBDATA(56)+BDS1(4)+BDS2(4) = 64 bits
    CHECK(cat.items.at("250").rep_group_bits == 64, "250 rep_group_bits = 64");
    // I440 RepetitiveGroup: BDS1(4)+BDS2(4) = 8 bits
    CHECK(cat.items.at("440").rep_group_bits == 8,  "440 rep_group_bits = 8");

    // I085 Compound sub-items
    const auto& i085 = cat.items.at("085");
    CHECK(i085.compound_sub_items.size() == 7, "085 has 7 sub-items");
    CHECK(i085.compound_sub_items[0].name == "SUM", "085 sub[0] = SUM");
    CHECK(i085.compound_sub_items[1].name == "PMN", "085 sub[1] = PMN");
    CHECK(i085.compound_sub_items[2].name == "POS", "085 sub[2] = POS");
    CHECK(i085.compound_sub_items[3].name == "GA",  "085 sub[3] = GA");
    CHECK(i085.compound_sub_items[4].name == "EM1", "085 sub[4] = EM1");
    CHECK(i085.compound_sub_items[5].name == "TOS", "085 sub[5] = TOS");
    CHECK(i085.compound_sub_items[6].name == "XP",  "085 sub[6] = XP");
    CHECK(i085.compound_sub_items[0].fixed_bytes == 1, "085/SUM = 1 byte");
    CHECK(i085.compound_sub_items[1].fixed_bytes == 4, "085/PMN = 4 bytes");
    CHECK(i085.compound_sub_items[2].fixed_bytes == 6, "085/POS = 6 bytes");
    CHECK(i085.compound_sub_items[3].fixed_bytes == 2, "085/GA  = 2 bytes");
    CHECK(i085.compound_sub_items[4].fixed_bytes == 2, "085/EM1 = 2 bytes");
    CHECK(i085.compound_sub_items[5].fixed_bytes == 1, "085/TOS = 1 byte");
    CHECK(i085.compound_sub_items[6].fixed_bytes == 1, "085/XP  = 1 byte");

    // I130 Compound sub-items
    const auto& i130 = cat.items.at("130");
    CHECK(i130.compound_sub_items.size() == 7, "130 has 7 sub-items");
    CHECK(i130.compound_sub_items[0].name == "SRL", "130 sub[0] = SRL");
    CHECK(i130.compound_sub_items[6].name == "APD", "130 sub[6] = APD");

    // I415 Compound sub-items
    const auto& i415 = cat.items.at("415");
    CHECK(i415.compound_sub_items.size() == 7, "415 has 7 sub-items");
    CHECK(i415.compound_sub_items[5].name == "RIM",  "415 sub[5] = RIM");
    CHECK(i415.compound_sub_items[6].name == "MIPT", "415 sub[6] = MIPT");
    CHECK(i415.compound_sub_items[5].fixed_bytes == 6, "415/RIM = 6 bytes");
    CHECK(i415.compound_sub_items[6].fixed_bytes == 1, "415/MIPT = 1 byte");

    // I450 Compound sub-items
    const auto& i450 = cat.items.at("450");
    CHECK(i450.compound_sub_items.size() == 6, "450 has 6 sub-items");
    CHECK(i450.compound_sub_items[0].name == "TR", "450 sub[0] = TR");
    CHECK(i450.compound_sub_items[3].name == "MS", "450 sub[3] = MS");
    CHECK(i450.compound_sub_items[3].fixed_bytes == 2, "450/MS = 2 bytes");

    codec.registerCategory(std::move(cat));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 2: Decode a minimal downlink message  (I010 + I410 + I140)
// ─────────────────────────────────────────────────────────────────────────────
static void testDecodeDownlinkBasic(Codec& codec) {
    std::cout << "\n=== Test: Decode basic downlink message ===\n";

    // UAP downlink slot order: 010 025 410 140 400 020 040 ...
    // Present: 010(slot1), 410(slot3), 140(slot4)
    // FSPEC byte 1: b7=1(010) b6=0 b5=1(410) b4=1(140) b3=0 b2=0 b1=0 b0=FX
    // All in 1 FSPEC byte → FX=0: 0b10110000 = 0xB0
    // Total: CAT(1)+LEN(2)+FSPEC(1)+010(2)+410(1)+140(3) = 10 bytes
    std::vector<uint8_t> frame = {
        0x07,             // CAT = 7
        0x00, 0x0A,       // LEN = 10
        0xB0,             // FSPEC: 010(b7)+410(b5)+140(b4), FX=0
        0x01, 0x02,       // I010: SAC=1, SIC=2
        0x01,             // I410: MSGTYP=1 (downlink success)
        0x00, 0x40, 0x00  // I140: ToD = 0x004000 = 16384 → 128 s
    };

    hexdump(frame, "Downlink basic input");

    DecodedBlock blk = codec.decode(frame);
    CHECK(blk.valid, "block is valid");
    CHECK(blk.records.size() == 1, "1 record");
    if (blk.records.empty()) return;

    const auto& rec = blk.records[0];
    CHECK(rec.items.count("010"),  "I010 present");
    CHECK(rec.items.count("410"),  "I410 present");
    CHECK(rec.items.count("140"),  "I140 present");
    CHECK(!rec.items.count("020"), "I020 absent");

    CHECK(rec.items.at("010").fields.at("SAC")    == 1,      "I010.SAC=1");
    CHECK(rec.items.at("010").fields.at("SIC")    == 2,      "I010.SIC=2");
    CHECK(rec.items.at("410").fields.at("MSGTYP") == 1,      "I410.MSGTYP=1");
    CHECK(rec.items.at("140").fields.at("ToD")    == 0x4000, "I140.ToD=0x4000");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 3: Round-trip for Fixed items in downlink
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripFixedDownlink(Codec& codec) {
    std::cout << "\n=== Test: Round-trip fixed items (downlink) ===\n";

    DecodedRecord rec;
    rec.uap_variation = "downlink";

    { DecodedItem it; it.fields["SAC"]=10; it.fields["SIC"]=20; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=0; rec.items["410"]=it; }  // MSGTYP=0 → downlink
    { DecodedItem it; it.fields["ToD"] = 43200u * 128u; rec.items["140"]=it; }
    { DecodedItem it; it.fields["PRI"]=0; it.fields["RN"]=12345; rec.items["400"]=it; }
    { DecodedItem it; it.fields["RHO"]=0x1000; it.fields["THETA"]=0x8000; rec.items["040"]=it; }
    { DecodedItem it; it.fields["X"]=s16(-100); it.fields["Y"]=200; rec.items["042"]=it; }

    auto encoded = codec.encode(7, {rec});
    hexdump(encoded, "FixedDownlink encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("010").fields.at("SAC") == 10,        "010/SAC = 10");
    CHECK(items.at("010").fields.at("SIC") == 20,        "010/SIC = 20");
    CHECK(items.at("410").fields.at("MSGTYP") == 0,      "410/MSGTYP = 0");
    CHECK(items.at("140").fields.at("ToD") == 43200u*128u,"140/ToD round-trip");
    CHECK(items.at("400").fields.at("PRI") == 0,         "400/PRI = 0");
    CHECK(items.at("400").fields.at("RN")  == 12345,     "400/RN = 12345");
    CHECK(items.at("040").fields.at("RHO")   == 0x1000,  "040/RHO round-trip");
    CHECK(items.at("040").fields.at("THETA") == 0x8000,  "040/THETA round-trip");
    CHECK(items.at("042").fields.at("X") == s16(-100),   "042/X = -100 (signed)");
    CHECK(items.at("042").fields.at("Y") == 200,         "042/Y = 200");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 4: Round-trip for Extended I020 (target report descriptor)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripExtendedI020(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Extended I020 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "downlink";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["410"]=it; }

    // I020: fill all 6 octets of the Extended item
    DecodedItem i020;
    // Octet 1
    i020.fields["TYP"]=3; i020.fields["SIM"]=0; i020.fields["RDP"]=1;
    i020.fields["SPI"]=0; i020.fields["RAB"]=0;
    // Octet 2
    i020.fields["TST"]=0; i020.fields["ERR"]=0; i020.fields["XPP"]=1;
    i020.fields["ME"]=0; i020.fields["MI"]=0; i020.fields["FOEFRI"]=1;
    // Octet 3
    i020.fields["ADSB_EP"]=1; i020.fields["ADSB_VAL"]=1;
    i020.fields["SCN_EP"]=0;  i020.fields["SCN_VAL"]=0;
    i020.fields["PAI_EP"]=1;  i020.fields["PAI_VAL"]=0;
    // Octet 4
    i020.fields["ACASXV_EP"]=1; i020.fields["ACASXV_VAL"]=1;
    i020.fields["POXPR_EP"]=0;  i020.fields["POXPR_VAL"]=0;
    // Octet 5
    i020.fields["POACT_EP"]=0;   i020.fields["POACT_VAL"]=0;
    i020.fields["DTFXPR_EP"]=1;  i020.fields["DTFXPR_VAL"]=0;
    i020.fields["DTFACT_EP"]=0;  i020.fields["DTFACT_VAL"]=0;
    // Octet 6
    i020.fields["IRMXPR_EP"]=0;  i020.fields["IRMXPR_VAL"]=0;
    i020.fields["IRMACT_EP"]=0;  i020.fields["IRMACT_VAL"]=0;
    rec.items["020"] = i020;

    auto encoded = codec.encode(7, {rec});
    hexdump(encoded, "ExtendedI020 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& f = blk.records[0].items.at("020").fields;
    CHECK(f.at("TYP")      == 3, "020/TYP = 3");
    CHECK(f.at("RDP")      == 1, "020/RDP = 1");
    CHECK(f.at("XPP")      == 1, "020/XPP = 1");
    CHECK(f.at("FOEFRI")   == 1, "020/FOEFRI = 1");
    CHECK(f.at("ADSB_EP")  == 1, "020/ADSB_EP = 1");
    CHECK(f.at("ADSB_VAL") == 1, "020/ADSB_VAL = 1");
    CHECK(f.at("PAI_EP")   == 1, "020/PAI_EP = 1");
    CHECK(f.at("ACASXV_EP")== 1, "020/ACASXV_EP = 1");
    CHECK(f.at("DTFXPR_EP")== 1, "020/DTFXPR_EP = 1");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 5: Round-trip for Repetitive-FX I030 (warning/error conditions)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripRepetitiveFXI030(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Repetitive-FX I030 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "downlink";

    { DecodedItem it; it.fields["SAC"]=5; it.fields["SIC"]=6; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=2; rec.items["410"]=it; }

    // I030: two warning codes (16=Duplicated Mode S, 3=Split plot)
    DecodedItem i030;
    i030.repetitions = {16, 3};
    rec.items["030"] = i030;

    auto encoded = codec.encode(7, {rec});
    hexdump(encoded, "RepFX I030 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& i = blk.records[0].items.at("030");
    CHECK(i.repetitions.size() == 2,  "030 has 2 repetitions");
    CHECK(i.repetitions[0]     == 16, "030 rep[0] = 16");
    CHECK(i.repetitions[1]     == 3,  "030 rep[1] = 3");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 6: Round-trip for Compound I085 (Mode 5/Extended Mode 1/X-Pulse)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCompoundI085(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Compound I085 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "downlink";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["410"]=it; }

    DecodedItem i085;
    // SUM: M5=1, ID=1, DA=0, M1=1, M2=0, M3=0, MC=1
    i085.compound_sub_fields["SUM"]["M5"] = 1;
    i085.compound_sub_fields["SUM"]["ID"] = 1;
    i085.compound_sub_fields["SUM"]["DA"] = 0;
    i085.compound_sub_fields["SUM"]["M1"] = 1;
    i085.compound_sub_fields["SUM"]["M2"] = 0;
    i085.compound_sub_fields["SUM"]["M3"] = 0;
    i085.compound_sub_fields["SUM"]["MC"] = 1;
    // PMN: PIN=0x1234, NAT=7, MIS=5
    i085.compound_sub_fields["PMN"]["PIN"] = 0x1234;
    i085.compound_sub_fields["PMN"]["NAT"] = 7;
    i085.compound_sub_fields["PMN"]["MIS"] = 5;
    // XP: X5=1, X3=1
    i085.compound_sub_fields["XP"]["X5"] = 1;
    i085.compound_sub_fields["XP"]["XC"] = 0;
    i085.compound_sub_fields["XP"]["X3"] = 1;
    i085.compound_sub_fields["XP"]["X2"] = 0;
    i085.compound_sub_fields["XP"]["X1"] = 0;
    rec.items["085"] = i085;

    auto encoded = codec.encode(7, {rec});
    hexdump(encoded, "Compound I085 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& c = blk.records[0].items.at("085").compound_sub_fields;
    CHECK(c.count("SUM"),            "085.SUM present");
    CHECK(c.at("SUM").at("M5") == 1,"085/SUM/M5 = 1");
    CHECK(c.at("SUM").at("ID") == 1,"085/SUM/ID = 1");
    CHECK(c.at("SUM").at("M1") == 1,"085/SUM/M1 = 1");
    CHECK(c.at("SUM").at("MC") == 1,"085/SUM/MC = 1");
    CHECK(c.count("PMN"),                 "085.PMN present");
    CHECK(c.at("PMN").at("PIN") == 0x1234,"085/PMN/PIN = 0x1234");
    CHECK(c.at("PMN").at("NAT") == 7,     "085/PMN/NAT = 7");
    CHECK(c.at("PMN").at("MIS") == 5,     "085/PMN/MIS = 5");
    CHECK(c.count("XP"),             "085.XP present");
    CHECK(c.at("XP").at("X5") == 1, "085/XP/X5 = 1");
    CHECK(c.at("XP").at("X3") == 1, "085/XP/X3 = 1");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 7: Round-trip for Repetitive I250 (Mode S MB data)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripRepetitiveI250(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Repetitive I250 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "downlink";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=4; rec.items["410"]=it; }

    DecodedItem i250;
    i250.group_repetitions.push_back({{"MBDATA", 0x01020304050607ULL},
                                       {"BDS1", 4}, {"BDS2", 0}});
    i250.group_repetitions.push_back({{"MBDATA", 0x0AABBCCDDEEFF0ULL & 0x00FFFFFFFFFFFFFFULL},
                                       {"BDS1", 1}, {"BDS2", 7}});
    rec.items["250"] = i250;

    auto encoded = codec.encode(7, {rec});
    hexdump(encoded, "Repetitive I250 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& gr = blk.records[0].items.at("250").group_repetitions;
    CHECK(gr.size() == 2,        "250 has 2 repetitions");
    CHECK(gr[0].at("BDS1") == 4,"250 rep[0] BDS1=4");
    CHECK(gr[0].at("BDS2") == 0,"250 rep[0] BDS2=0");
    CHECK(gr[1].at("BDS1") == 1,"250 rep[1] BDS1=1");
    CHECK(gr[1].at("BDS2") == 7,"250 rep[1] BDS2=7");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 8: Round-trip for Compound I130 (radar plot characteristics)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripCompoundI130(Codec& codec) {
    std::cout << "\n=== Test: Round-trip Compound I130 ===\n";

    DecodedRecord rec;
    rec.uap_variation = "downlink";

    { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["410"]=it; }

    DecodedItem i130;
    i130.compound_sub_fields["SRL"]["SRL"] = 64;
    i130.compound_sub_fields["SAM"]["SAM"] = s8(-70);
    i130.compound_sub_fields["PRL"]["PRL"] = 32;
    rec.items["130"] = i130;

    auto encoded = codec.encode(7, {rec});
    hexdump(encoded, "Compound I130 encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& c = blk.records[0].items.at("130").compound_sub_fields;
    CHECK(c.count("SRL"),                "130.SRL present");
    CHECK(c.at("SRL").at("SRL") == 64,  "130/SRL/SRL = 64");
    CHECK(c.count("SAM"),                "130.SAM present");
    CHECK(c.at("SAM").at("SAM") == s8(-70), "130/SAM/SAM = -70 dBm");
    CHECK(c.count("PRL"),                "130.PRL present");
    CHECK(c.at("PRL").at("PRL") == 32,  "130/PRL/PRL = 32");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 9: Round-trip for uplink message (I415, I420, I440)
// ─────────────────────────────────────────────────────────────────────────────
static void testRoundTripUplink(Codec& codec) {
    std::cout << "\n=== Test: Round-trip uplink message ===\n";

    DecodedRecord rec;
    rec.uap_variation = "uplink";

    { DecodedItem it; it.fields["SAC"]=2; it.fields["SIC"]=3; rec.items["010"]=it; }
    { DecodedItem it; it.fields["MSGTYP"]=5; rec.items["410"]=it; }
    { DecodedItem it; it.fields["PRI"]=1; it.fields["RN"]=99; rec.items["400"]=it; }

    // I420: polar window
    { DecodedItem it;
      it.fields["RS"]=0x0400; it.fields["RE"]=0x0800;
      it.fields["TS"]=0x2000; it.fields["TE"]=0x4000;
      rec.items["420"]=it; }

    // I440: two BDS register requests
    DecodedItem i440;
    i440.group_repetitions.push_back({{"BDS1", 4}, {"BDS2", 0}});
    i440.group_repetitions.push_back({{"BDS1", 5}, {"BDS2", 0}});
    rec.items["440"] = i440;

    // I415: RIM sub-item
    DecodedItem i415;
    i415.compound_sub_fields["RIM"]["LO"]       = 1;
    i415.compound_sub_fields["RIM"]["MSPROB"]   = 2;
    i415.compound_sub_fields["RIM"]["M5FORMAT"] = 0;
    i415.compound_sub_fields["RIM"]["M4CS"]     = 0;
    i415.compound_sub_fields["RIM"]["M5S"]  = 0; i415.compound_sub_fields["RIM"]["SM5S"] = 0;
    i415.compound_sub_fields["RIM"]["SM54"] = 0; i415.compound_sub_fields["RIM"]["SM5C"] = 0;
    i415.compound_sub_fields["RIM"]["SM53"] = 0; i415.compound_sub_fields["RIM"]["SM52"] = 0;
    i415.compound_sub_fields["RIM"]["SM51"] = 0;
    i415.compound_sub_fields["RIM"]["M5"]   = 0; i415.compound_sub_fields["RIM"]["RCMA"] = 0;
    i415.compound_sub_fields["RIM"]["RCMC"] = 0; i415.compound_sub_fields["RIM"]["CMC"]  = 0;
    i415.compound_sub_fields["RIM"]["CM3A"] = 0; i415.compound_sub_fields["RIM"]["MS"]   = 1;
    i415.compound_sub_fields["RIM"]["M4S"]  = 0; i415.compound_sub_fields["RIM"]["SMC"]  = 0;
    i415.compound_sub_fields["RIM"]["SM3A"] = 0; i415.compound_sub_fields["RIM"]["SM2"]  = 0;
    i415.compound_sub_fields["RIM"]["SM1"]  = 0; i415.compound_sub_fields["RIM"]["MCO"]  = 0;
    i415.compound_sub_fields["RIM"]["M3O"]  = 0; i415.compound_sub_fields["RIM"]["MCS"]  = 0;
    i415.compound_sub_fields["RIM"]["M3S"]  = 0; i415.compound_sub_fields["RIM"]["MD"]   = 0;
    i415.compound_sub_fields["RIM"]["MC"]   = 0; i415.compound_sub_fields["RIM"]["MB"]   = 0;
    i415.compound_sub_fields["RIM"]["M4"]   = 0; i415.compound_sub_fields["RIM"]["M3A"]  = 0;
    i415.compound_sub_fields["RIM"]["M2"]   = 0; i415.compound_sub_fields["RIM"]["M1"]   = 0;
    i415.compound_sub_fields["MIPT"]["MIPT"] = 0xAB;
    rec.items["415"] = i415;

    auto encoded = codec.encode(7, {rec});
    hexdump(encoded, "Uplink encoded");
    CHECK(!encoded.empty(), "encode produced output");

    DecodedBlock blk = codec.decode(encoded);
    CHECK(blk.valid, "block valid");
    if (blk.records.empty()) return;

    const auto& items = blk.records[0].items;
    CHECK(items.at("410").fields.at("MSGTYP") == 5,  "410/MSGTYP = 5 (uplink)");
    CHECK(items.at("400").fields.at("PRI")    == 1,  "400/PRI = 1");
    CHECK(items.at("400").fields.at("RN")     == 99, "400/RN = 99");
    CHECK(items.count("420"),                         "I420 present");
    CHECK(items.at("420").fields.at("RS") == 0x0400, "420/RS round-trip");
    CHECK(items.at("420").fields.at("TE") == 0x4000, "420/TE round-trip");
    CHECK(items.count("440"),                         "I440 present");
    const auto& gr440 = items.at("440").group_repetitions;
    CHECK(gr440.size() == 2,         "440 has 2 repetitions");
    CHECK(gr440[0].at("BDS1") == 4, "440 rep[0] BDS1=4");
    CHECK(gr440[1].at("BDS1") == 5, "440 rep[1] BDS1=5");
    CHECK(items.count("415"),                          "I415 present");
    const auto& c415 = items.at("415").compound_sub_fields;
    CHECK(c415.count("RIM"),                "415.RIM present");
    CHECK(c415.at("RIM").at("LO")     == 1,"415/RIM/LO = 1");
    CHECK(c415.at("RIM").at("MSPROB") == 2,"415/RIM/MSPROB = 2");
    CHECK(c415.at("RIM").at("MS")     == 1,"415/RIM/MS = 1");
    CHECK(c415.count("MIPT"),               "415.MIPT present");
    CHECK(c415.at("MIPT").at("MIPT")  == 0xAB,"415/MIPT = 0xAB");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Test 10: UAP discriminator – downlink vs uplink items
// ─────────────────────────────────────────────────────────────────────────────
static void testUAPDiscriminator(Codec& codec) {
    std::cout << "\n=== Test: UAP discriminator (downlink vs uplink) ===\n";

    // Downlink: encode I220 (present only in downlink UAP)
    {
        DecodedRecord rec;
        rec.uap_variation = "downlink";
        { DecodedItem it; it.fields["SAC"]=1; it.fields["SIC"]=1; rec.items["010"]=it; }
        { DecodedItem it; it.fields["MSGTYP"]=1; rec.items["410"]=it; }
        { DecodedItem it; it.fields["AA"]=0xABCDEF; rec.items["220"]=it; }

        auto enc = codec.encode(7, {rec});
        hexdump(enc, "Downlink with I220 encoded");
        DecodedBlock blk = codec.decode(enc);
        CHECK(blk.valid, "DL: block valid");
        if (!blk.records.empty()) {
            CHECK(blk.records[0].items.count("220"),                    "DL: I220 present");
            CHECK(blk.records[0].items.at("220").fields.at("AA") == 0xABCDEFu, "DL: I220.AA");
        }
    }

    // Uplink: encode I440 (present only in uplink UAP)
    {
        DecodedRecord rec;
        rec.uap_variation = "uplink";
        { DecodedItem it; it.fields["SAC"]=2; it.fields["SIC"]=2; rec.items["010"]=it; }
        { DecodedItem it; it.fields["MSGTYP"]=7; rec.items["410"]=it; }
        DecodedItem i440;
        i440.group_repetitions.push_back({{"BDS1", 6}, {"BDS2", 0}});
        rec.items["440"] = i440;

        auto enc = codec.encode(7, {rec});
        hexdump(enc, "Uplink with I440 encoded");
        DecodedBlock blk = codec.decode(enc);
        CHECK(blk.valid, "UL: block valid");
        if (!blk.records.empty()) {
            CHECK(blk.records[0].items.at("410").fields.at("MSGTYP") == 7, "UL: MSGTYP=7");
            CHECK(blk.records[0].items.count("440"),                        "UL: I440 present");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    fs::path spec_path = (argc > 1) ? argv[1] : "specs/CAT07.xml";

    Codec codec;
    testSpecLoad(codec, spec_path);
    testDecodeDownlinkBasic(codec);
    testRoundTripFixedDownlink(codec);
    testRoundTripExtendedI020(codec);
    testRoundTripRepetitiveFXI030(codec);
    testRoundTripCompoundI085(codec);
    testRoundTripRepetitiveI250(codec);
    testRoundTripCompoundI130(codec);
    testRoundTripUplink(codec);
    testUAPDiscriminator(codec);

    std::cout << "\n=== Results: "
              << (failures == 0 ? "ALL PASSED" : std::to_string(failures) + " FAILED")
              << " ===\n";
    return failures == 0 ? 0 : 1;
}
