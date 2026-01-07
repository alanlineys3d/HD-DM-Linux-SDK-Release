#ifndef IMAGESAVEUTIL_H
#define IMAGESAVEUTIL_H

#include <vector>
#include <string>

#include <fstream>
#include <sstream>
#include <iostream>

bool SaveRawImage(const std::vector<unsigned char>& imageBuffer,
                  int imageSize,
                  int serialNumber,
                  const std::string& prefix) {
    // Create filename using prefix and serial number
    std::ostringstream filename;
    // Place saved files in a subdirectory relative to the executable's likely run location
    // Assuming the executable runs from DMPreview/build/
    filename << prefix << "_" << serialNumber << ".raw";

    // TODO: Ensure the directory saved_images/ exists or create it.
    //       Need platform-specific code or a cross-platform library like std::filesystem (C++17)
    //       For simplicity now, assuming the directory exists or manually created.
    //       Consider creating it relative to the executable path or DMPreview root.

    // Open file in binary mode
    std::ofstream outFile(filename.str(), std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open file: " << filename.str() << std::endl;
        return false;
    }

    // Check if the buffer has enough data
    if (imageBuffer.size() < static_cast<size_t>(imageSize)) {
        std::cerr << "Error: imageBuffer size (" << imageBuffer.size()
                  << ") is less than imageSize (" << imageSize << ") for " << filename.str() << std::endl;
        outFile.close();
        return false;
    }

    // Write image data to file
    outFile.write(reinterpret_cast<const char*>(imageBuffer.data()), imageSize);

    // Check if write was successful
    if (outFile.fail()) {
        std::cerr << "Failed to write to file: " << filename.str() << std::endl;
        outFile.close();
        return false;
    }

    outFile.close();
    // std::cout << "Saved image to: " << filename.str() << std::endl; // Optional: uncomment for verbose output
    return true;
}

#endif // IMAGESAVEUTIL_H 