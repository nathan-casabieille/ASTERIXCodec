#ifndef ASTERIX_HPP
#define ASTERIX_HPP

// Header principal qui inclut tous les composants de la bibliothèque ASTERIX

// Core components
#include "asterix/core/types.hpp"
#include "asterix/core/asterix_category.hpp"
#include "asterix/core/asterix_decoder.hpp"
#include "asterix/core/asterix_message.hpp"

// Data components
#include "asterix/data/field_value.hpp"
#include "asterix/data/field.hpp"
#include "asterix/data/data_item.hpp"

// Specification components
#include "asterix/spec/field_spec.hpp"
#include "asterix/spec/data_item_spec.hpp"
#include "asterix/spec/uap_spec.hpp"
#include "asterix/spec/xml_parser.hpp"

// Utility components
#include "asterix/utils/exceptions.hpp"
#include "asterix/utils/byte_buffer.hpp"
#include "asterix/utils/bit_reader.hpp"

#endif // ASTERIX_HPP