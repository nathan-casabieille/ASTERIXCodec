#include "asterix/asterix.hpp"
#include <iostream>
#include <iomanip>
#include <string>

using namespace asterix;

void printSeparator(const std::string& title = "") {
    std::cout << "\n";
    std::cout << "========================================";
    if (!title.empty()) {
        std::cout << "\n" << title;
        std::cout << "\n========================================";
    }
    std::cout << "\n";
}

void decodeAndPrintMessage(const AsterixDecoder& decoder, const std::string& hex_data, const std::string& description) {
    printSeparator(description);
    
    try {
        std::cout << "Raw data: " << hex_data << "\n\n";
        
        // Décoder le message
        AsterixMessage message = decoder.decode(hex_data);
        
        // Afficher le résumé
        std::cout << message.getSummary() << "\n\n";
        
        // Afficher tous les Data Items présents
        std::cout << "Decoded Data Items:\n";
        std::cout << "-------------------\n";
        
        auto item_ids = message.getDataItemIds();
        for (const auto& item_id : item_ids) {
            const auto& item = message.getDataItem(item_id);
            std::cout << "\n" << item_id << ":";
            if (!item.getTitle().empty()) {
                std::cout << " (" << item.getTitle() << ")";
            }
            std::cout << "\n";
            
            // Afficher tous les champs
            auto field_names = item.getFieldNames();
            for (const auto& field_name : field_names) {
                const auto& field = item.getField(field_name);
                std::cout << "  " << std::setw(20) << std::left << field_name << ": ";
                std::cout << field.getValue().toString();
                
                if (field.hasUnit() && field.getUnit() != "none") {
                    std::cout << " " << field.getUnit();
                }
                std::cout << "\n";
            }
        }
        
        // Exemples d'accès direct aux champs
        std::cout << "\n\nDirect Field Access Examples:\n";
        std::cout << "------------------------------\n";
        
        // I002/010 - Data Source Identifier
        if (message.hasDataItem("I002/010")) {
            try {
                auto sac = message.getFieldValue("I002/010", "SAC").asUInt();
                auto sic = message.getFieldValue("I002/010", "SIC").asUInt();
                std::cout << "Data Source: SAC=" << sac << ", SIC=" << sic << "\n";
            } catch (const std::exception& e) {
                std::cout << "Could not access I002/010 fields: " << e.what() << "\n";
            }
        }
        
        // I002/000 - Message Type
        if (message.hasDataItem("I002/000")) {
            try {
                auto msg_type = message.getFieldValue("I002/000", "message_type");
                std::cout << "Message Type: " << msg_type.toString() << "\n";
            } catch (const std::exception& e) {
                std::cout << "Could not access I002/000: " << e.what() << "\n";
            }
        }
        
        // I002/020 - Sector Number
        if (message.hasDataItem("I002/020")) {
            try {
                auto sector = message.getFieldValue("I002/020", "sector_number").asUInt();
                std::cout << "Sector Number: " << sector << "\n";
            } catch (const std::exception& e) {
                std::cout << "Could not access I002/020: " << e.what() << "\n";
            }
        }
        
        // I002/030 - Time of Day
        if (message.hasDataItem("I002/030")) {
            try {
                auto tod = message.getFieldValue("I002/030", "time_of_day").asUInt();
                // Convertir en secondes (format ASTERIX: 1/128 secondes)
                double seconds = tod / 128.0;
                int hours = static_cast<int>(seconds / 3600);
                int minutes = static_cast<int>((seconds - hours * 3600) / 60);
                double secs = seconds - hours * 3600 - minutes * 60;
                
                std::cout << "Time of Day: " << std::setfill('0') 
                         << std::setw(2) << hours << ":"
                         << std::setw(2) << minutes << ":"
                         << std::fixed << std::setprecision(3) 
                         << std::setw(6) << secs << " UTC\n";
            } catch (const std::exception& e) {
                std::cout << "Could not access I002/030: " << e.what() << "\n";
            }
        }
        
        std::cout << "\n✓ Message decoded successfully!\n";
        
    } catch (const DecodingException& e) {
        std::cout << "✗ Decoding error: " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cout << "✗ Unexpected error: " << e.what() << "\n";
    }
}

int main(int argc, char* argv[]) {
    try {
        printSeparator("ASTERIX CAT002 Decoder Test");
        
        // 1. Charger la spécification CAT002
        std::cout << "Loading CAT002 specification...\n";
        
        std::string spec_file = "specs/cat002_v1.1.xml";
        
        // Si un fichier de spec est fourni en argument, l'utiliser
        if (argc > 1) {
            spec_file = argv[1];
        }
        
        AsterixCategory cat002(spec_file);
        std::cout << "✓ Specification loaded: CAT" 
                  << static_cast<int>(cat002.getCategoryNumber())
                  << " version " << cat002.getVersion() << "\n";
        
        // 2. Créer le décodeur
        AsterixDecoder decoder(cat002);
        std::cout << "✓ Decoder initialized\n";
        
        // 3. Messages CAT002 à décoder
        
        // Message 1
        std::string msg1 = "02 00 0C F4 08 11 02 E0 22 05 39 60";
        // Breakdown:
        // 02: Category 002
        // 00 0C: Length = 12 bytes
        // F4: UAP byte (bits 8,7,6,5,3 set)
        // Remaining bytes: data items
        
        decodeAndPrintMessage(decoder, msg1, "Test Message 1");
        
        // Message utilisateur si fourni
        if (argc > 2) {
            std::string custom_msg = argv[2];
            decodeAndPrintMessage(decoder, custom_msg, "Custom Message from Command Line");
        }
        
        printSeparator("Test Completed");
        std::cout << "\nAll tests completed!\n";
        std::cout << "\nUsage: " << argv[0] << " [spec_file.xml] [hex_message]\n";
        std::cout << "  spec_file.xml : Path to CAT002 XML specification (default: specs/cat002_v1.1.xml)\n";
        std::cout << "  hex_message   : Hexadecimal message to decode\n";
        std::cout << "\nExample:\n";
        std::cout << "  " << argv[0] << " specs/cat002_v1.1.xml \"02 00 0C F4 08 11 02 E0 22 05 39 60\"\n\n";
        
        return 0;
        
    } catch (const SpecificationException& e) {
        std::cerr << "✗ Specification error: " << e.what() << "\n";
        std::cerr << "\nMake sure the CAT002 XML specification file exists and is valid.\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "✗ Fatal error: " << e.what() << "\n";
        return 1;
    }
}