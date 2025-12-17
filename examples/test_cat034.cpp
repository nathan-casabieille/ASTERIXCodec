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

void printRecord(const AsterixRecord& record, std::size_t record_num) {
    std::cout << "\n--- Data Record " << record_num << " ---\n";
    std::cout << record.getSummary() << "\n\n";
    
    auto item_ids = record.getDataItemIds();
    for (const auto& item_id : item_ids) {
        const auto& item = record.getDataItem(item_id);
        std::cout << item_id;
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
        
        // Si c'est un item répétitif
        if (item.isRepetitive()) {
            std::cout << "  Repetitions: " << item.getRepetitionCount() << "\n";
            for (std::size_t i = 0; i < item.getRepetitionCount(); ++i) {
                std::cout << "  [" << i << "]:\n";
                const auto& rep = item.getRepetition(i);
                auto rep_field_names = rep.getFieldNames();
                for (const auto& field_name : rep_field_names) {
                    const auto& field = rep.getField(field_name);
                    std::cout << "    " << std::setw(18) << std::left << field_name << ": ";
                    std::cout << field.getValue().toString();
                    if (field.hasUnit() && field.getUnit() != "none") {
                        std::cout << " " << field.getUnit();
                    }
                    std::cout << "\n";
                }
            }
        }
    }
}

void decodeAndPrintMessage(const AsterixDecoder& decoder, const std::string& hex_data, const std::string& description) {
    printSeparator(description);
    
    try {
        std::cout << "Raw data: " << hex_data << "\n\n";
        
        // Décoder le message
        AsterixMessage message = decoder.decode(hex_data);
        
        // Afficher le résumé global
        std::cout << message.getSummary() << "\n";
        
        // Afficher chaque Data Record
        if (message.isMultiRecord()) {
            std::cout << "\n⚠ This is a multi-record message!\n";
        }
        
        for (std::size_t i = 0; i < message.getRecordCount(); ++i) {
            const auto& record = message.getRecord(i);
            printRecord(record, i + 1);
        }
        
        // Statistiques
        std::cout << "\n\n=== Statistics ===\n";
        std::cout << "Total Records: " << message.getRecordCount() << "\n";
        
        std::size_t total_items = 0;
        for (std::size_t i = 0; i < message.getRecordCount(); ++i) {
            total_items += message.getRecord(i).getDataItemCount();
        }
        std::cout << "Total Data Items: " << total_items << "\n";
        std::cout << "Average Items per Record: " 
                  << std::fixed << std::setprecision(1)
                  << (static_cast<double>(total_items) / message.getRecordCount()) << "\n";
        
        std::cout << "\n✓ Message decoded successfully!\n";
        
    } catch (const DecodingException& e) {
        std::cout << "✗ Decoding error: " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cout << "✗ Unexpected error: " << e.what() << "\n";
    }
}

int main(int argc, char* argv[]) {
    try {
        printSeparator("ASTERIX CAT034 Multi-Record Decoder Test");
        
        // 1. Charger la spécification CAT034
        std::cout << "Loading CAT034 specification...\n";
        
        std::string spec_file = "specs/cat034_v1.1.xml";
        
        if (argc > 1) {
            spec_file = argv[1];
        }
        
        AsterixCategory cat034(spec_file);
        std::cout << "✓ Specification loaded: CAT" 
                  << static_cast<int>(cat034.getCategoryNumber())
                  << " version " << cat034.getVersion() << "\n";
        
        // 2. Créer le décodeur
        AsterixDecoder decoder(cat034);
        std::cout << "✓ Decoder initialized\n";
        
        // 3. Message CAT034 multi-records
        // Ce message contient 3 Data Records :
        // - Record 1: SAC/SIC=3/5, Type=1 (North Marker), TOD=1234
        // - Record 2: SAC/SIC=3/5, Type=2 (Sector crossing), TOD=1235, Sector=123
        // - Record 3: SAC/SIC=3/5, Type=2 (Sector crossing), TOD=1236, Sector=124
        std::string msg1 = "22 00 1A E0 03 05 01 00 04 D2 F0 03 05 02 00 04 D3 7B F0 03 05 02 00 04 D4 7C";
        
        decodeAndPrintMessage(decoder, msg1, "Multi-Record Message - CAT034");
        
        // Message utilisateur si fourni
        if (argc > 2) {
            std::string custom_msg = argv[2];
            decodeAndPrintMessage(decoder, custom_msg, "Custom Message from Command Line");
        }
        
        printSeparator("Test Completed");
        std::cout << "\nAll tests completed!\n";
        std::cout << "\nUsage: " << argv[0] << " [spec_file.xml] [hex_message]\n";
        std::cout << "  spec_file.xml : Path to CAT034 XML specification (default: specs/cat034_v1.1.xml)\n";
        std::cout << "  hex_message   : Hexadecimal message to decode\n";
        std::cout << "\nExample:\n";
        std::cout << "  " << argv[0] << " specs/cat034_v1.1.xml \"22 00 1A E0 03 05 01 00 04 D2 F0\"\n\n";
        
        return 0;
        
    } catch (const SpecificationException& e) {
        std::cerr << "✗ Specification error: " << e.what() << "\n";
        std::cerr << "\nMake sure the CAT034 XML specification file exists and is valid.\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "✗ Fatal error: " << e.what() << "\n";
        return 1;
    }
}