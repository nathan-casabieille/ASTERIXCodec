// SpecLoader.cpp – Parses the ASTERIX XML spec into a CategoryDef.
// Uses pugixml for robust, zero-copy XML parsing.

#include "ASTERIXCodec/SpecLoader.hpp"

#include <pugixml.hpp>

#include <charconv>
#include <cstring>
#include <string>

namespace asterix {

// ─── Small parsing helpers ────────────────────────────────────────────────────

static uint64_t parseU64(const char* s, const char* ctx) {
    uint64_t v = 0;
    auto [ptr, ec] = std::from_chars(s, s + std::strlen(s), v);
    if (ec != std::errc{})
        throw SpecLoadError(std::string(ctx) + ": cannot parse uint '" + s + "'");
    return v;
}

static double parseDouble(const char* s, const char* ctx) {
    char* end = nullptr;
    double v = std::strtod(s, &end);
    if (end == s)
        throw SpecLoadError(std::string(ctx) + ": cannot parse double '" + s + "'");
    return v;
}

static Encoding parseEncoding(const char* s) {
    if (!s || *s == '\0')           return Encoding::Raw;
    if (strcmp(s, "raw")            == 0) return Encoding::Raw;
    if (strcmp(s, "table")          == 0) return Encoding::Table;
    if (strcmp(s, "unsigned_quantity") == 0) return Encoding::UnsignedQuantity;
    if (strcmp(s, "signed_quantity")   == 0) return Encoding::SignedQuantity;
    if (strcmp(s, "string_octal")   == 0) return Encoding::StringOctal;
    throw SpecLoadError(std::string("Unknown encoding: '") + s + "'");
}

static Presence parsePresence(const char* s) {
    if (!s || strcmp(s, "optional")    == 0) return Presence::Optional;
    if (strcmp(s, "mandatory")         == 0) return Presence::Mandatory;
    if (strcmp(s, "conditional")       == 0) return Presence::Conditional;
    throw SpecLoadError(std::string("Unknown presence: '") + s + "'");
}

// ─── Parse a single <Element> or <Spare> node into an ElementDef ──────────────

static ElementDef parseElementNode(pugi::xml_node node) {
    ElementDef e;

    if (strcmp(node.name(), "Spare") == 0) {
        e.is_spare   = true;
        e.encoding   = Encoding::Spare;
        e.bits       = static_cast<uint16_t>(parseU64(node.attribute("bits").as_string("0"), "Spare.bits"));
        return e;
    }

    // <Element>
    e.name     = node.attribute("name").as_string("");
    e.bits     = static_cast<uint16_t>(parseU64(node.attribute("bits").as_string("0"), "Element.bits"));
    e.encoding = parseEncoding(node.attribute("encoding").as_string("raw"));

    if (e.bits == 0)
        throw SpecLoadError("Element '" + e.name + "' has bits=0");

    // Optional quantity attributes
    if (auto a = node.attribute("scale");    a) e.scale = parseDouble(a.as_string(), "scale");
    if (auto a = node.attribute("unit");     a) e.unit  = a.as_string();
    if (auto a = node.attribute("min");      a) { e.min_val = parseDouble(a.as_string(), "min"); e.has_range = true; }
    if (auto a = node.attribute("max");      a) { e.max_val = parseDouble(a.as_string(), "max"); e.has_range = true; }

    // <Entry> children for table encoding
    for (auto entry : node.children("Entry")) {
        uint64_t    val  = parseU64(entry.attribute("value").as_string("0"), "Entry.value");
        std::string mean = entry.attribute("meaning").as_string("");
        e.table[val]     = std::move(mean);
    }

    return e;
}

// ─── Parse a <Fixed> block ─────────────────────────────────────────────────────

static void parseFixed(pugi::xml_node node, DataItemDef& item) {
    item.type = ItemType::Fixed;
    uint32_t total_bits = 0;
    for (auto child : node.children()) {
        if (strcmp(child.name(), "Element") == 0 || strcmp(child.name(), "Spare") == 0) {
            ElementDef e = parseElementNode(child);
            total_bits  += e.bits;
            item.elements.push_back(std::move(e));
        }
    }
    if (total_bits % 8 != 0)
        throw SpecLoadError("Fixed item '" + item.id + "' total bits (" +
                            std::to_string(total_bits) + ") not a multiple of 8");
    item.fixed_bytes = static_cast<uint16_t>(total_bits / 8);
}

// ─── Parse an <Extended> block ────────────────────────────────────────────────

static void parseExtended(pugi::xml_node node, DataItemDef& item) {
    item.type = ItemType::Extended;
    for (auto octet_node : node.children("Octet")) {
        OctetDef oct;
        uint32_t bits = 0;
        for (auto child : octet_node.children()) {
            if (strcmp(child.name(), "Element") == 0 || strcmp(child.name(), "Spare") == 0) {
                ElementDef e = parseElementNode(child);
                bits        += e.bits;
                oct.elements.push_back(std::move(e));
            }
        }
        if (bits != 7)
            throw SpecLoadError("Extended item '" + item.id +
                                "' octet must sum to 7 bits, got " + std::to_string(bits));
        item.octets.push_back(std::move(oct));
    }
    if (item.octets.empty())
        throw SpecLoadError("Extended item '" + item.id + "' has no <Octet> children");
}

// ─── Parse a <Repetitive> block ───────────────────────────────────────────────

static void parseRepetitive(pugi::xml_node node, DataItemDef& item) {
    item.type = ItemType::Repetitive;
    // Exactly one <Element> child expected (the 7-bit repeated field)
    auto elem_node = node.child("Element");
    if (!elem_node)
        throw SpecLoadError("Repetitive item '" + item.id + "' has no <Element>");

    ElementDef e = parseElementNode(elem_node);
    if (e.bits != 7)
        throw SpecLoadError("Repetitive item '" + item.id +
                            "' element must be 7 bits, got " + std::to_string(e.bits));
    item.rep_element = std::move(e);
}

// ─── Parse an <Explicit> block ────────────────────────────────────────────────

static void parseExplicit(pugi::xml_node /*node*/, DataItemDef& item) {
    item.type = ItemType::SP; // both SP and generic Explicit handled the same way
}

// ─── Parse one <DataItem> node ────────────────────────────────────────────────

static DataItemDef parseDataItem(pugi::xml_node node) {
    DataItemDef item;
    item.id       = node.attribute("id").as_string("");
    item.name     = node.attribute("name").as_string("");
    item.presence = parsePresence(node.attribute("presence").as_string("optional"));

    if (item.id.empty())
        throw SpecLoadError("<DataItem> missing 'id' attribute");

    if (auto fixed    = node.child("Fixed"))      parseFixed(fixed, item);
    else if (auto ext = node.child("Extended"))   parseExtended(ext, item);
    else if (auto rep = node.child("Repetitive")) parseRepetitive(rep, item);
    else if (auto exp = node.child("Explicit"))   parseExplicit(exp, item);
    else
        throw SpecLoadError("DataItem '" + item.id + "' has no recognised type element");

    return item;
}

// ─── Parse <UAPs> block ───────────────────────────────────────────────────────

static void parseUAPs(pugi::xml_node node, CategoryDef& cat) {
    cat.default_variation = node.attribute("default").as_string("default");

    for (auto var_node : node.children("Variation")) {
        std::string var_name = var_node.attribute("name").as_string("");
        if (var_name.empty())
            throw SpecLoadError("<Variation> missing 'name'");

        std::vector<std::string> uap;
        for (auto item_node : var_node.children("Item")) {
            uap.emplace_back(item_node.attribute("ref").as_string("-"));
        }
        cat.uap_variations[var_name] = std::move(uap);
    }

    // <Case item="020" field="TYP"> discriminator
    if (auto case_node = node.child("Case")) {
        UapCase uc;
        uc.item_id = case_node.attribute("item").as_string("");
        uc.field   = case_node.attribute("field").as_string("");
        if (uc.item_id.empty() || uc.field.empty())
            throw SpecLoadError("<Case> missing 'item' or 'field' attribute");

        for (auto when : case_node.children("When")) {
            uint64_t    val = parseU64(when.attribute("value").as_string("0"), "Case.When.value");
            std::string use = when.attribute("use").as_string("");
            if (use.empty())
                throw SpecLoadError("<When> missing 'use' attribute");
            uc.value_to_variation[val] = std::move(use);
        }
        cat.uap_case = std::move(uc);
    }
}

// ─── Public entry point ───────────────────────────────────────────────────────

CategoryDef loadSpec(const std::filesystem::path& xml_path) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(xml_path.c_str());
    if (!result)
        throw SpecLoadError("Failed to parse XML '" + xml_path.string() +
                            "': " + result.description());

    pugi::xml_node root = doc.child("Category");
    if (!root)
        throw SpecLoadError("XML root element must be <Category>");

    CategoryDef cat;
    cat.cat     = static_cast<uint8_t>(parseU64(root.attribute("cat").as_string("0"), "Category.cat"));
    cat.name    = root.attribute("name").as_string("");
    cat.edition = root.attribute("edition").as_string("");
    cat.date    = root.attribute("date").as_string("");

    if (cat.cat == 0)
        throw SpecLoadError("<Category cat=\"...\"> is missing or zero");

    // Parse all <DataItem> nodes
    for (auto item_node : root.child("DataItems").children("DataItem")) {
        DataItemDef item = parseDataItem(item_node);
        cat.items[item.id] = std::move(item);
    }

    // Parse UAP
    if (auto uaps = root.child("UAPs"))
        parseUAPs(uaps, cat);
    else
        throw SpecLoadError("No <UAPs> element found in category " + std::to_string(cat.cat));

    if (cat.uap_variations.empty())
        throw SpecLoadError("Category " + std::to_string(cat.cat) + " has no UAP variations");

    return cat;
}

} // namespace asterix
