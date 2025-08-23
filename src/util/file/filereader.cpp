// util/file/filereader.cpp -- The FileReader class provides generic methods for reading from a binary data file.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <fstream>

#include "core/core.hpp"
//#include "util/file/binpath.hpp"
#include "util/file/filereader.hpp"
#include "util/file/fileutils.hpp"

namespace gorp {

// Loads a data file into memory.
FileReader::FileReader(std::string filename, bool allow_missing_file) : read_index_(0)
{   
    //core().log("Loading and parsing data file " + filename + ".");
    //filename = BinPath::game_path(filename);
    if (!fileutils::file_exists(filename))
    {
        if (allow_missing_file) return;
        else throw std::runtime_error("Cannot load file: " + filename);
    }
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) throw std::runtime_error("Cannot load file: " + filename);
    std::streampos file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    data_.resize(static_cast<size_t>(file_size));
    file.read(data_.data(), file_size);
    file.close();
}

// Reads a blob of binary data, in the form of a std::vector<char>
std::vector<char> FileReader::read_char_vec()
{
    const uint32_t size = read_data<uint32_t>();
    std::vector<char> buffer(data_.begin() + read_index_, data_.begin() + read_index_ + size);
    read_index_ += size;
    return buffer;
}

// Reads a string from the loaded file.
std::string FileReader::read_string()
{
    uint32_t len = read_data<uint32_t>();
    std::string result(data_.begin() + read_index_, data_.begin() + read_index_ + len);
    read_index_ += len;
    return result;
}

};
