/**
Author: Tylor Slay
Project: ECE510 - EV project
Description: These are functions I have build using lots of other resources to
help with importing, export, and manipulating data. I have tried to follow
Google's coding standard to make things easier to read and understand.

Google C++ coding standard
--------------------------
https://google.github.io/styleguide/cppguide.html

General rules:
- Indents are two spaces. No tabs should be used anywhere.
- Each line must be at most 80 characters long.
- Comments can be // or /* but // is most commonly used.
- Avoid Abbreviations
- Type/Function Names: CamelCase
- Variable Names: lowercase_underscores
- Class Data Members: trailing_underscores_
- Constant Names: kName
- Enumerator/Macro Names: ALL_CAPS

Note: Please reference http://en.cppreference.com/w/cpp for additional help
**/

#ifndef TSUTILITIES_H_INCLUDED
#define TSUTILITIES_H_INCLUDED

// INCLUDE
#include <iostream>     // cin, cout
#include <fstream>      // ofstream, ifstream
#include <sstream>      // stringstream, istringstream, ostringstream
#include <vector>       // reserve, emplace_back
#include <string>       // getline, stoi, stod
#include <algorithm>    // sort
#include <typeinfo>     // typeid

// MACRO
#define DEBUG(x) std::cout << x << std::endl

// tylor slay utilities (tsu)
namespace tsu {

// ToString
// * converts any value to a string
template <typename T>
static std::string ToString (T t_value) {
    std::ostringstream ss;
    ss << t_value;
    return ss.str();
} // end ToString

// ToNumber
// * converts string to desired number type
template <typename T>
static void ToNumber (const std::string kString,
                      const std::string kType,
                      T& number) {
    if (!kString.empty()) {
        if (kType == "j"){
            number = std::stoul(kString);
        } else if (kType == "d"){
            number = std::stod(kString);
        } else {
            number = std::stoi(kString);
        }
    }
} // end ToNumber

// Print
// * simple function to write to terminal
// * a template is used to abstract the data type
template <typename T>
static void Print (const T kOutput) {
    std::cout << kOutput << std::endl;
} // end Print

// PrintVector
// * Print each element within vector
template <typename T>
static void PrintVector (const std::vector<T>& kVector) {
    tsu::Print("\n*** Printing Vector ***\n");
    const unsigned int kSize = kVector.size();
    for (unsigned int i = 0; i < kSize; i++){
        tsu::Print(kVector[i]);
    }
} // end PrintVector

// PrintMatrix
// * Print each element within matrix
template <typename T>
static void PrintMatrix (const std::vector<std::vector<T>>& kMatrix) {
    tsu::Print("\n*** Printing Matrix ***\n");
    std::string line;
    const unsigned int kRows = kMatrix.size();
    const unsigned int kColumns = kMatrix[0].size();
    for (unsigned int i = 0; i < kRows; i++){
        for (unsigned int j = 0; j < kColumns; j++){
            line.append(ToString(kMatrix[i][j]));
            line.append("\t");
        }
        tsu::Print(line);
        line.clear();
    }
} // end PrintMatrix

// CountDelimiter
// * count number of delimiters within string
// * return total
static double CountDelimiter (const std::string& kString,
                              const char kDelimiter) {
    std::string line, item;
    double ctr = 0;

    std::stringstream ss(kString);
    while (std::getline(ss, line)) {
        std::stringstream ss2(line);
        while (std::getline(ss2, item, kDelimiter)) {
            ctr++;
        }
    }
    return ctr;
} // end CountDelimiter

// SplitString
// * split string given delimiter
// * operates on vector passed as a reference to reduce memory
static void SplitString (const std::string& kString,
                         const char kDelimiter,
                         std::vector<std::string>& op_vector) {
    std::string line, item;
    const double kItems = tsu::CountDelimiter(kString, kDelimiter);
    op_vector.reserve(kItems);
    std::stringstream ss(kString);
    while (std::getline(ss, line)) {
        std::stringstream ss2(line);
        while (std::getline(ss2, item, kDelimiter)) {
            op_vector.emplace_back(item);
        }
    }
} // end SplitString

// ReadFile
// * open file and move to end for buffer size
// * read file into allocated string
// * parse string for delimiter
// * update vector with individual strings for processing
static void ReadFile (const std::string kFilename,
                      const char kDelimiter,
                      std::vector<std::string>& op_vector) {
    if (std::ifstream file{kFilename, std::ios::binary | std::ios::ate}) {
        auto size = file.tellg();
        std::string str(size, '\0');  // construct string to stream size
        file.seekg(0);
        if (file.read(&str[0], size)) {
            tsu::SplitString(str, kDelimiter, op_vector);
        } else {
            tsu::Print("*** Error: reading file! ***");
        }
    } else {
        tsu::Print("*** Error: file not found! ***");
    }
} // end ReadFile

// VectorToFile
// * open file and write simple vector to file
template <typename T>
static void VectorToFile (const std::string kFilename,
                          const std::vector<T>& kVector) {
    const unsigned int kItems = kVector.size();
    std::string item;
    // open file to write to
    std::ofstream output_file(kFilename);
    while (output_file.is_open()) {
        for (unsigned int i = 0; i < kItems; i++) {
            item = tsu::ToString(kVector[i]);
            output_file << item << '\n';
        }
        output_file.close();
    }
} // end VectorToFile

// MatrixToFile
// * open file and write matrix using delimiter
template <typename T>
static void MatrixToFile (const std::string kFilename,
                          const char kDelimiter,
                          const std::vector<std::vector<T>>& kMatrix) {
    const unsigned int kRows = kMatrix.size();
    const unsigned int kColumns = kMatrix[0].size();
    std::string item;
    // open file to write to
    std::ofstream output_file(kFilename);
    while (output_file.is_open()) {
        for (unsigned int i = 0; i < kRows; i++) {
            for (unsigned int j = 0; j < kColumns; j++) {
                item = tsu::ToString(kMatrix[i][j]);
                char delimiter = (j == kColumns - 1) ? '\n' : kDelimiter;
                output_file << item << delimiter;
            }
        }
        output_file.close();
    }
} // end MatrixToFile

// VectorToMatrix
// * converts single vector string into a vector of vectors of given type
template <typename T1, typename T2>
static void VectorToMatrix (const std::vector<T1>& kVector,
                            const unsigned int kColumns,
                            const bool kHeader,
                            std::vector<std::vector<T2>>& op_matrix) {
    // determine number of rows
    const unsigned int kItems = kVector.size();
    const unsigned int kRows = kItems/kColumns;

    // preallocate memory for vector
    std::vector<T2> temp_vector;
    temp_vector.reserve(kColumns);
    op_matrix.reserve(kRows);

    // check to see if first row should be ignored
    const unsigned int kStart = (kHeader) ? kColumns : 0;

    // TODO (tylorslay): figure out why typeid doesn't seem to work in linux
    T2 item;
    //const std::string kType = typeid(item).name()
    const std::string kType = "i"; //Tylor

    // loop through each value
    // at then end of each column reset temp vector
    unsigned int ctr = 1;
    for (unsigned int i = kStart; i < kItems; i++){
        if (ctr < kColumns){
            tsu::ToNumber(kVector[i], kType, item);
            temp_vector.emplace_back(item);
            ctr++;
        } else {
            ctr = 1;
            tsu::ToNumber(kVector[i], kType, item);
            temp_vector.emplace_back(item);
            op_matrix.emplace_back(temp_vector);
            temp_vector.clear();
        }
    }
} // end VectorToMatrix

// SortMatrix
// * sorting function takes the column number to sort
// * kGreater (false) will sort least to greatest
// * sorts using lambda expression
template <typename T>
static void SortMatrix (const unsigned int kColumn,
                        const bool kGreater,
                        std::vector<std::vector<T>>& op_matrix){
    std::sort (op_matrix.begin(), op_matrix.end(),
              [&kGreater, &kColumn](const std::vector<T>& a,
                                    const std::vector<T>& b) {
                if (kGreater) {
                    return a[kColumn] > b[kColumn];
                } else {
                    return a[kColumn] < b[kColumn];
                }
              });
} // end SortMatrix

} // end namespace tsu

#endif // TSUTILITIES_H_INCLUDED
