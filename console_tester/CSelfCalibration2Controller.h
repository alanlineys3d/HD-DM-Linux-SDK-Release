#ifndef CSelfCalibration2Controller_H
#define CSelfCalibration2Controller_H

#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include "../eSPDI/selfK2.h"
#include <iostream>
#include <utility>
#include <vector>
#include <unordered_map>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstdint>
#include <algorithm>
// Global variables for buffer encoding/decoding operations
// These are shared state used by encode_buffer_data and reorder_rectify_buffer
// Must match Windows implementation behavior
typedef double FL_t;
extern int n_read_buffer_data_items;       // Tracks number of items read from buffer
extern int n_write_buffer_data_items;      // Tracks number of items to write to buffer
extern int n_data_structure_elements;      // Fixed size for rectification data structure
extern uint16_t table_size;                // Tracks final encoded buffer size

class CSelfCalibration2Controller
{

public:
    class BinaryParser {
    private:
        std::vector<uint8_t> data;
        std::unordered_map<uint16_t, uint8_t> dataMap; // Maps addresses to values
    public:
        /**
         * Constructor that takes a buffer of binary data
         */
        explicit BinaryParser(std::vector<uint8_t>  buffer) : data(std::move(buffer)) {
            parse();
        }

        /**
         * Parse the binary data and populate the dataMap
         */
        void parse() {
            if (data.size() < 2) {
                std::cerr << "Error: Data buffer too small" << std::endl;
                return;
            }

            // Get total data count from first 2 bytes (big endian)
            uint16_t dataCount = (data[0] << 8) | data[1];
            std::cout << "Total data count: " << dataCount
                      << " (0x" << std::hex << std::uppercase << dataCount << std::dec << ")" << std::endl;

            // Parse data pairs (2-byte address, 1-byte value)
            size_t offset = 2; // Start after the data count
            size_t parsedEntries = 0;

            while (offset + 2 < data.size() && parsedEntries < dataCount) {
                // Extract address (2 bytes)
                uint16_t address = (data[offset] << 8) | data[offset + 1];

                // Extract value (1 byte)
                uint8_t value = data[offset + 2];

                // Store in map
                dataMap[address] = value;

                // Move to next entry
                offset += 3;
                parsedEntries++;
            }

            std::cout << "Parsed " << parsedEntries << " entries" << std::endl;
        }

        /**
         * Get the value at a specific address
         * @param address - The address to look up
         * @param value - Output parameter for the value if found
         * @return bool - True if address exists, false otherwise
         */
        bool getValue(uint16_t address, uint8_t& value) const {
            auto it = dataMap.find(address);
            if (it != dataMap.end()) {
                value = it->second;
                return true;
            }
            return false;
        }

        /**
         * Patch a value at a specific address
         * @param address - The address to patch
         * @param value - The new value (1 byte)
         * @return bool - True if successful, false if address doesn't exist
         */
        bool patchValue(uint16_t address, uint8_t value) {
            // Check if address exists
            if (dataMap.find(address) == dataMap.end()) {
                std::cout << "Address 0x" << std::hex << std::uppercase << address
                          << " does not exist" << std::dec << std::endl;
                return false;
            }

            // Update the value in the map
            dataMap[address] = value;

            // Find the address in the binary data and update it
            size_t offset = 2; // Start after the data count
            while (offset + 2 < data.size()) {
                uint16_t currentAddress = (data[offset] << 8) | data[offset + 1];
                if (currentAddress == address) {
                    data[offset + 2] = value;
                    std::cout << "Patched address 0x" << std::hex << std::uppercase << address
                              << " with value 0x" << static_cast<int>(value) << std::dec << std::endl;
                    return true;
                }
                offset += 3;
            }

            return true;
        }

        /**
         * Export the current data as a vector
         * @return std::vector<uint8_t> - The modified data
         */
        std::vector<uint8_t> exportData() const {
            return data;
        }

        /**
         * Save the current data to a file
         * @param filename - The filename to save to
         * @return bool - True if successful, false otherwise
         */
        bool saveToFile(const std::string& filename) const {
            std::ofstream outFile(filename, std::ios::binary);
            if (!outFile) {
                std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
                return false;
            }

            outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
            return outFile.good();
        }

        /**
         * Print all data entries
         */
        void printData() const {
            std::cout << "Address\tValue" << std::endl;
            std::cout << "-------\t-----" << std::endl;

            // Create a sorted view of the map entries
            std::vector<std::pair<uint16_t, uint8_t>> sortedEntries;
            for (const auto& entry : dataMap) {
                sortedEntries.push_back(entry);
            }

            std::sort(sortedEntries.begin(), sortedEntries.end());

            // Use a standard C++11 compatible iterator approach instead of C++17 structured bindings
            for (const auto& entry : sortedEntries) {
                const uint16_t address = entry.first;
                const uint8_t value = entry.second;
                std::cout << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << address
                          << "\t0x" << std::setw(2) << static_cast<int>(value) << std::dec << std::endl;
            }
        }
    };

    struct PARAM_t {
        float mean_shift_kernel_size_acquisition;
        float mean_shift_kernel_size_tracking;
        bool b_auto_adjust_period;
        float update_period_in_seconds_of_cy_in_acquisition;
        float update_period_in_seconds_of_cy_in_tracking;
        float cy_sampling_period_in_seconds;
        float temperatureThreshold;
        float valid_min_fill_rate_threshold;
        bool b_dynamic_kernel_size;
        float max_devication_of_cy;
        SelfK2::C_Cy_Compensator::E_ESTIMATOR_TYPES e_estimator_type;
        float blind_zone_ratio;
        float fillrate_threshold_to_enter_tracking;
        int convergency_cnt_threshold;
        int max_acq_iterations;
        float smoothing_factor;
    };

    class CFile_IO_Data_Structure_Rectify {
    public:
        FL_t RECTIFY_CONTROL;    /*    , bytes_shift= 0 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 8    , n_bytes = 1    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 0 ,     */
        FL_t RECTIFY_MODULE_ENABLE;    /*    , bytes_shift= 1 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 8    , n_bytes = 1    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 0 ,     */
        FL_t RECTIFY_WIDTH;    /*    , bytes_shift= 2 ,    ,High_bit =  3    ,Low_bit =  0    , n_bits = 12    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 0 ,     */
        FL_t RECTIFY_HEIGHT;    /*    , bytes_shift= 4 ,    ,High_bit =  3    ,Low_bit =  0    , n_bits = 12    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_M00;    /*    , bytes_shift= 6 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 21    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_M01;    /*    , bytes_shift= 8 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 11    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 21    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_M02;    /*    , bytes_shift= 10 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 19    , n_bytes = 3    , little_endian= 1 ,    , FractionalBit = 17    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_M10;    /*    , bytes_shift= 13 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 11    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 21    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_M11;    /*    , bytes_shift= 15 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 21    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_M12;    /*    , bytes_shift= 17 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 19    , n_bytes = 3    , little_endian= 1 ,    , FractionalBit = 17    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_M20;    /*    , bytes_shift= 20 ,    ,High_bit =  3    ,Low_bit =  0    , n_bits = 12    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 21    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_M21;    /*    , bytes_shift= 22 ,    ,High_bit =  1    ,Low_bit =  0    , n_bits = 10    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 21    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_M22;    /*    , bytes_shift= 24 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 19    , n_bytes = 3    , little_endian= 1 ,    , FractionalBit = 17    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_H_STR;    /*    , bytes_shift= 27 ,    ,High_bit =  1    ,Low_bit =  0    , n_bits = 10    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_H_END;    /*    , bytes_shift= 29 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 11    , n_bytes = 2    , little_endian= 1 ,    -, FractionalBit = 1    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_V_STR;    /*    , bytes_shift= 31 ,    ,High_bit =  1    ,Low_bit =  0    , n_bits = 10    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_V_END;    /*    , bytes_shift= 33 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 11    , n_bytes = 2    , little_endian= 1 ,    -, FractionalBit = 1    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_M00;    /*    , bytes_shift= 35 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 21    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_M01;    /*    , bytes_shift= 37 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 11    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 21    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_M02;    /*    , bytes_shift= 39 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 19    , n_bytes = 3    , little_endian= 1 ,    , FractionalBit = 17    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_M10;    /*    , bytes_shift= 42 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 11    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 21    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_M11;    /*    , bytes_shift= 44 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 21    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_M12;    /*    , bytes_shift= 46 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 19    , n_bytes = 3    , little_endian= 1 ,    , FractionalBit = 17    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_M20;    /*    , bytes_shift= 49 ,    ,High_bit =  3    ,Low_bit =  0    , n_bits = 12    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 21    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_M21;    /*    , bytes_shift= 51 ,    ,High_bit =  1    ,Low_bit =  0    , n_bits = 10    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 21    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_M22;    /*    , bytes_shift= 53 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 19    , n_bytes = 3    , little_endian= 1 ,    , FractionalBit = 17    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_H_STR;    /*    , bytes_shift= 56 ,    ,High_bit =  1    ,Low_bit =  0    , n_bits = 10    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_H_END;    /*    , bytes_shift= 58 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 11    , n_bytes = 2    , little_endian= 1 ,    -, FractionalBit = 1    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_V_STR;    /*    , bytes_shift= 60 ,    ,High_bit =  1    ,Low_bit =  0    , n_bits = 10    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_V_END;    /*    , bytes_shift= 62 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 11    , n_bytes = 2    , little_endian= 1 ,    -, FractionalBit = 1    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_P1;    /*    , bytes_shift= 64 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 8    , n_bytes = 1    , little_endian= 1 ,    , FractionalBit = 13    , DontCare= 0 ,     */
        FL_t DontCare_0x41_1byte;    /*    , bytes_shift= 65 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 8    , n_bytes = 1    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 1 ,     */
        FL_t RECTIFY_L_RECT_P2;    /*    , bytes_shift= 66 ,    ,High_bit =  6    ,Low_bit =  0    , n_bits = 7    , n_bytes = 1    , little_endian= 1 ,    , FractionalBit = 13    , DontCare= 0 ,     */
        FL_t DontCare_0x43_1byte;    /*    , bytes_shift= 67 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 8    , n_bytes = 1    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 1 ,     */
        FL_t RECTIFY_L_RECT_K1;    /*    , bytes_shift= 68 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 10    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_K2;    /*    , bytes_shift= 70 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 10    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_K3;    /*    , bytes_shift= 72 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 10    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_K4;    /*    , bytes_shift= 74 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 10    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_K5;    /*    , bytes_shift= 76 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 10    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_K6;    /*    , bytes_shift= 78 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 10    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_CX;    /*    , bytes_shift= 80 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 2    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_CY;    /*    , bytes_shift= 82 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 2    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_FX;    /*    , bytes_shift= 84 ,    ,High_bit =  1    ,Low_bit =  0    , n_bits = 18    , n_bytes = 3    , little_endian= 1 ,    , FractionalBit = 3    , DontCare= 0 ,     */
        FL_t RECTIFY_L_RECT_FY;    /*    , bytes_shift= 87 ,    ,High_bit =  1    ,Low_bit =  0    , n_bits = 18    , n_bytes = 3    , little_endian= 1 ,    , FractionalBit = 3    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_P1;    /*    , bytes_shift= 90 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 8    , n_bytes = 1    , little_endian= 1 ,    , FractionalBit = 13    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_P2;    /*    , bytes_shift= 91 ,    ,High_bit =  6    ,Low_bit =  0    , n_bits = 7    , n_bytes = 1    , little_endian= 1 ,    , FractionalBit = 13    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_K1;    /*    , bytes_shift= 92 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 10    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_K2;    /*    , bytes_shift= 94 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 10    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_K3;    /*    , bytes_shift= 96 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 10    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_K4;    /*    , bytes_shift= 98 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 10    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_K5;    /*    , bytes_shift= 100 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 10    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_K6;    /*    , bytes_shift= 102 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 10    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_CX;    /*    , bytes_shift= 104 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 2    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_CY;    /*    , bytes_shift= 106 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 2    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_FX;    /*    , bytes_shift= 108 ,    ,High_bit =  1    ,Low_bit =  0    , n_bits = 18    , n_bytes = 3    , little_endian= 1 ,    , FractionalBit = 3    , DontCare= 0 ,     */
        FL_t RECTIFY_R_RECT_FY;    /*    , bytes_shift= 111 ,    ,High_bit =  1    ,Low_bit =  0    , n_bits = 18    , n_bytes = 3    , little_endian= 1 ,    , FractionalBit = 3    , DontCare= 0 ,     */
        FL_t RECTIFY_PREP_PROCESS;    /*    , bytes_shift= 114 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 8    , n_bytes = 1    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 0 ,     */
        FL_t RECTIFY_PREP_SCAL_H_FAC;    /*    , bytes_shift= 115 ,    ,High_bit =  4    ,Low_bit =  0    , n_bits = 13    , n_bytes = 2    , little_endian= 0 ,    , FractionalBit = 9    , DontCare= 0 ,     */
        FL_t RECTIFY_PREP_SCAL_V_FAC;    /*    , bytes_shift= 117 ,    ,High_bit =  4    ,Low_bit =  0    , n_bits = 13    , n_bytes = 2    , little_endian= 0 ,    , FractionalBit = 9    , DontCare= 0 ,     */
        FL_t RECTIFY_PREP_YUV_H_START;    /*    , bytes_shift= 119 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 11    , n_bytes = 2    , little_endian= 0 ,    -, FractionalBit = 1    , DontCare= 0 ,     */
        FL_t RECTIFY_PREP_YUV_H_SIZE;    /*    , bytes_shift= 121 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 11    , n_bytes = 2    , little_endian= 0 ,    -, FractionalBit = 1    , DontCare= 0 ,     */
        FL_t RECTIFY_PREP_YUV_V_START;    /*    , bytes_shift= 123 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 11    , n_bytes = 2    , little_endian= 0 ,    -, FractionalBit = 1    , DontCare= 0 ,     */
        FL_t RECTIFY_PREP_YUV_V_SIZE;    /*    , bytes_shift= 125 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 11    , n_bytes = 2    , little_endian= 0 ,    -, FractionalBit = 1    , DontCare= 0 ,     */
        FL_t RECTIFY_BLK_WIDTH;    /*    , bytes_shift= 127 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 8    , n_bytes = 1    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 0 ,     */
        FL_t DontCare_0x80_16bytes;    /*    , bytes_shift= 128 ,    ,High_bit =  127    ,Low_bit =  0    , n_bits = 128    , n_bytes = 16    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 1 ,     */
        FL_t DontCare_0x90_16bytes;    /*    , bytes_shift= 144 ,    ,High_bit =  127    ,Low_bit =  0    , n_bits = 128    , n_bytes = 16    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 1 ,     */
        FL_t DontCare_0xa0_15bytes;    /*    , bytes_shift= 160 ,    ,High_bit =  119    ,Low_bit =  0    , n_bits = 120    , n_bytes = 15    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 1 ,     */
        FL_t RPOST_YUVP_ENB;    /*    , bytes_shift= 175 ,    ,High_bit =  7    ,Low_bit =  0    , n_bits = 8    , n_bytes = 1    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 0 ,     */
        FL_t DontCare_0xb0_10bytes;    /*    , bytes_shift= 176 ,    ,High_bit =  79    ,Low_bit =  0    , n_bits = 80    , n_bytes = 10    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 1 ,     */
        FL_t RPOST_SCAL_H_FAC;    /*    , bytes_shift= 186 ,    ,High_bit =  4    ,Low_bit =  0    , n_bits = 13    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 9    , DontCare= 0 ,     */
        FL_t RPOST_SCAL_V_FAC;    /*    , bytes_shift= 188 ,    ,High_bit =  4    ,Low_bit =  0    , n_bits = 13    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 9    , DontCare= 0 ,     */
        FL_t RPOST_YUV_H_STR;    /*    , bytes_shift= 190 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 11    , n_bytes = 2    , little_endian= 1 ,    -, FractionalBit = 1    , DontCare= 0 ,     */
        FL_t RPOST_YUV_H_SIZE;    /*    , bytes_shift= 191 ,    ,High_bit =  6    ,Low_bit =  4    , n_bits = 11    , n_bytes = 2    , little_endian= 0 ,    -, FractionalBit = 1    , DontCare= 0 ,     */
        FL_t RPOST_YUV_V_STR;    /*    , bytes_shift= 193 ,    ,High_bit =  2    ,Low_bit =  0    , n_bits = 11    , n_bytes = 2    , little_endian= 1 ,    -, FractionalBit = 1    , DontCare= 0 ,     */
        FL_t RPOST_YUV_V_SIZE;    /*    , bytes_shift= 194 ,    ,High_bit =  6    ,Low_bit =  4    , n_bits = 11    , n_bytes = 2    , little_endian= 0 ,    -, FractionalBit = 1    , DontCare= 0 ,     */
        FL_t DontCare_0xc4_2bytes;    /*    , bytes_shift= 196 ,    ,High_bit =  15    ,Low_bit =  0    , n_bits = 16    , n_bytes = 2    , little_endian= 1 ,    , FractionalBit = 0    , DontCare= 1 ,     */
    };

    enum class Mode {
        RUNTIME_CORRECTION,
        DEPTH_BROKEN_REPAIR
    };

    enum class E_THERMAL_SENSOR_MODEL {
        ONSEMI_AR0135 = 100,
        ONSEMI_AR0144 = 101,
        STM_VD5X = 200,
        OV_OG02 = 300,
        OV_OG01 = 301,
        TI_TMP1075 = 1000,
    };

    CSelfCalibration2Controller(void *APCHandle, DEVSELINFO *pDevSelInfo, size_t width, size_t height);
    ~CSelfCalibration2Controller();

    void UpdateRectifyLogData(eSPCtrl_RectLogData &data);
    bool IsDepthStreaming();
    void SwapDepthImage(std::vector<uint8_t> depthBuffer);
    /**
     * Handle the SelfK2 functions states
     * @return
     */
    int RunSelfK2();
    void StopSelfK2();
    void ResetSelfK2();
    bool WriteToFlash(float compensateCy, int index);
    inline float GetCurrentSensorTemperature();

    void SetMode(Mode mode);
    float GetCurrentCompCy();
    Mode GetMode() const { return m_currentMode.load(); }
    
    bool IsRunning() const { return m_isRunning.load(); }

    /**
     * Set false if we intend to stop compensator from calibrating the depth quality.
     * @param shouldWork
     */
    void SetCompensatorWorking(bool shouldWork);
    bool GetCompensatorWorking(void);

    void Reset() { 
        std::lock_guard<std::mutex> lock(m_CompensatorMutex);
        m_Reset = true; 
    }

private:
    void temperatureMonitorThread();
    void compensatorThread();
    void stopThreads();

private:
    //camera
    void *mAPCHandle = nullptr;
    std::vector<DEVSELINFO *> m_deviceSelInfoList;
    size_t mWidth = 0;
    size_t mHeight = 0;
    DEVINFORMATIONEX mDevInformationEx;
    uint16_t mProductId = 0x0000;
    // SelfK2
    std::mutex m_CompensatorMutex;
    std::vector<unsigned char> m_Depth;
    std::unique_ptr<SelfK2::C_Cy_Compensator> m_cy_compensator;
    std::unique_ptr<SelfK2::C_Focal_Compensator> m_focal_compensator;
    std::unique_ptr<eSPCtrl_RectLogData> m_RectifyData;
    struct PARAM_t param_repair =  { 9, 1, true, 1.0, 999.0, 0.15, 1, 0.025, true, 5.0,
            SelfK2::C_Cy_Compensator::E_ESTIMATOR_TYPES::_confidence_weighted_ranking, 0.00, 0.98,  1, 30, 0.0 };
    struct PARAM_t param_runtime = { 1, 1, true, 1.0,  60.0, 0.15, 1,  0.10, true, 2.5,
            SelfK2::C_Cy_Compensator::E_ESTIMATOR_TYPES::_confidence_weighted_ranking, 0.00, 0.997, 3, 30, 0.5 };

    std::atomic<bool> m_isRunning{false};
    std::atomic<bool> m_threadsShouldExit{false};
    std::unique_ptr<std::thread> m_temperatureThread;
    std::unique_ptr<std::thread> m_compensatorThread;
    std::mutex m_mutex;
    std::mutex mDepthMutex;
    float m_currentTemperature{0.0f};
    std::atomic<Mode> m_currentMode;
    struct PARAM_t m_cy_compensator_param;
    bool m_Reset{false};
    bool mCompensatorWorking{true};

    inline void  SetCurrentSensorTemperature(float temperature);

    static int get_temperature_param(uint16_t pid, E_THERMAL_SENSOR_MODEL &e_sensor_model, int &sensor_slave_addr,
                                     int &nSensorMode);

    int get_temperature_of_img_sensor_ar0144(void *pHandleAPC, E_THERMAL_SENSOR_MODEL e_sensor_model,
                                             int sensor_slave_addr, int nSensorMode, float &fTemperature);

    int get_temperature_of_img_sensor_ar0135(void *pHandleAPC, E_THERMAL_SENSOR_MODEL e_sensor_model,
                                             int sensor_slave_addr, int nSensorMode, float &fTemperature);

    int get_temperature_of_img_sensor_st_vd5x(void *pHandleAPC, E_THERMAL_SENSOR_MODEL e_sensor_model,
                                          int sensor_slave_addr, int nSensorMode, float &fTemperature);

    int get_temperature(void *pHandleAPC, E_THERMAL_SENSOR_MODEL e_sensor_model, int sensor_slave_addr, int nSensorMode,
                        float &fTemperature);

    int get_temperature_of_ti_tmp_xxxx(void *pHandleAPC, E_THERMAL_SENSOR_MODEL e_sensor_model, int sensor_slave_addr,
                                       int nSensorMode, float &fTemperature);

    void updateStatusText();
    std::string m_statusText;
    std::stringstream m_outAndInfo;
};

#endif // CSelfCalibration2Controller_H

