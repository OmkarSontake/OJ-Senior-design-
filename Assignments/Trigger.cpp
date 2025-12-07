#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cctype>
#include <algorithm>

// ✅ Check if numeric
bool isNumber(const std::string& str) {
    if (str.empty()) return false;
    bool decimalPointFound = false;
    for (char ch : str) {
        if (ch == '.') {
            if (decimalPointFound) return false;
            decimalPointFound = true;
        } else if (!isdigit(ch) && ch != '-') {
            return false;
        }
    }
    return true;
}

// ✅ Split CSV line
std::vector<std::string> splitCSVLine(const std::string& line) {
    std::vector<std::string> result;
    std::stringstream ss(line);
    std::string cell;
    while (std::getline(ss, cell, ',')) {
        result.push_back(cell);
    }
    return result;
}

// ✅ Read triggers and return all row values
std::vector<std::vector<std::string>> readTriggeredData(const std::string& filePath,
                                                        std::vector<std::string>& headersOut) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filePath << std::endl;
        return {};
    }

    std::string line;

    // --- Step 1: Skip first 14 rows (Excel rows 1–14)
    for (int i = 0; i < 14 && std::getline(file, line); i++) {}

    // --- Step 2: Read header row (row 15)
    if (!std::getline(file, line)) {
        std::cerr << "File ended before header row found." << std::endl;
        return {};
    }
    headersOut = splitCSVLine(line);

    // Find Buy Triggered and Sell Triggered columns
    int buyIndex = -1, sellIndex = -1;
    for (size_t i = 0; i < headersOut.size(); i++) {
        std::string hLower = headersOut[i];
        std::transform(hLower.begin(), hLower.end(), hLower.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        if (hLower.find("buy triggered") != std::string::npos) buyIndex = i;
        if (hLower.find("sell triggered") != std::string::npos) sellIndex = i;
    }

    if (buyIndex == -1 || sellIndex == -1) {
        std::cerr << "Could not locate Buy Triggered / Sell Triggered columns." << std::endl;
        return {};
    }

    // --- Step 3: Process all rows and collect triggers
    std::vector<std::vector<std::string>> triggeredData;
    while (std::getline(file, line)) {
        auto cols = splitCSVLine(line);
        if (cols.size() <= std::max(buyIndex, sellIndex)) continue;

        std::string buyStr = cols[buyIndex];
        std::string sellStr = cols[sellIndex];

        if (isNumber(buyStr) && std::stod(buyStr) > 0) {
            auto row = cols;
            row.push_back("Buy");
            triggeredData.push_back(row);
        }
        if (isNumber(sellStr) && std::stod(sellStr) > 0) {
            auto row = cols;
            row.push_back("Sell");
            triggeredData.push_back(row);
        }
    }

    file.close();
    return triggeredData;
}

// ✅ Write output CSV
void writeTriggeredDataToFile(const std::string& outputFilePath,
                              const std::vector<std::string>& headers,
                              const std::vector<std::vector<std::string>>& data) {
    std::ofstream outFile(outputFilePath);
    if (!outFile.is_open()) {
        std::cerr << "Error opening output file: " << outputFilePath << std::endl;
        return;
    }

    // Write headers + Type
    for (size_t i = 0; i < headers.size(); i++) {
        outFile << headers[i] << ",";
    }
    outFile << "Type\n";

    // Write rows
    for (const auto& row : data) {
        for (size_t i = 0; i < row.size(); i++) {
            outFile << row[i];
            if (i < row.size() - 1) outFile << ",";
        }
        outFile << "\n";
    }

    outFile.close();
    std::cout << "Triggered data written to " << outputFilePath << std::endl;
}

// ✅ Main
int main() {
    std::string staticPath = "C:/Users/dedhi/OneDrive/Desktop/Project/Static_Data.csv";
    std::string outputPath = "C:/Users/dedhi/OneDrive/Desktop/Project/triggers.csv";

    std::vector<std::string> headers;
    auto triggeredData = readTriggeredData(staticPath, headers);

    // Add Type column once
    std::vector<std::string> headersWithType = headers;

    writeTriggeredDataToFile(outputPath, headersWithType, triggeredData);

    std::cout << "Total triggers: " << triggeredData.size() << std::endl;
    return 0;
}
