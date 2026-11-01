// SPDX-License-Identifier: GPL-3.0-or-later
// Citations for every dataset, constant and reference figure used in Mission Control.
#pragma once

#include <string_view>
#include <vector>

namespace cr {

struct Source
{
    std::string_view key;
    std::string_view title;
    std::string_view publisher; // author(s) or organisation, and year
    std::string_view url;
    std::string_view note;      // what we use it for, licence, caveats
};

const std::vector<Source> &sources();
const Source *findSource(std::string_view key);

} // namespace cr
