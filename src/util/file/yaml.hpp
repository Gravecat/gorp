// util/file/yaml.hpp -- Helper functions to make using RapidYAML easier.

// SPDX-FileType: SOURCE
// SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <map>

#include "3rdparty/rapidyaml/rapidyaml-0.9.0.hpp"
#include "core/global.hpp"

namespace gorp {

class YAML {
public:
                    YAML();                                     // Blank constructor.
                    YAML(const std::string& filename, bool allow_backslash = false);    // Calls load_file() when constructing.
    std::string     get(int index) const;                       // Retrieves a value from a sequence, as a string.
    YAML            get_child(const std::string& key) const;    // Retrieves a child noderef of this tree.
    std::vector<std::string>    get_seq(const std::string &key) const;  // Retrieves all values of a sequence.
    bool            is_map() const;                             // Checks if the noderef points to a valid map.
    bool            is_seq() const;                             // Checks if the noderef points to a valid sequence.
    bool            key_exists(const std::string& key) const;   // Checks if a given key exists.
    std::vector<std::string>    keys() const;                   // Retrieves the key values of a map.
    std::map<std::string, std::string>  keys_vals() const;      // Retrieves the key/value pairs of a map.
    void            load_file(const std::string& filename, bool allow_backslash = false);   // Loads a YAML file into memory and parse it.
    int             size() const;                               // Checks the number of children on the noderef.
    std::string     val(const std::string& key) const;          // Returns the value of a key, as a string.

protected:
                    YAML(const ryml::Tree tree, ryml::ConstNodeRef new_ref);    // Creates a new YAML object from a parent tree.

private:
    ryml::ConstNodeRef  noderef() const;    // Returns the noderef for the loaded tree.

    ryml::ConstNodeRef  ref_;   // The NodeRef for this part of the tree.
    ryml::Tree          tree_;  // The parsed YAML data.
};

}   // namespace westgate
