#pragma once
// SpecLoader.hpp – Parses an ASTERIX XML spec file into a CategoryDef.

#include "Types.hpp"
#include <filesystem>
#include <stdexcept>
#include <string>

namespace asterix {

// Thrown when the XML is structurally invalid or violates the schema rules.
class SpecLoadError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Loads a Category definition from the given XML file path.
// Throws SpecLoadError on any parse or validation failure.
CategoryDef loadSpec(const std::filesystem::path& xml_path);

} // namespace asterix
