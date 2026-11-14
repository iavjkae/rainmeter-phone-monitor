// SPDX-License-Identifier: MIT
#include "fps/process_filter.h"

#include <algorithm>
#include <cctype>
#include <utility>

namespace phone_monitor::fps {
namespace {

char lower(char ch) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
}

bool matches(std::string_view pattern, std::string_view value) {
    // Remember the last '*' and grow its match on failure. This avoids recursion and
    // keeps user-supplied wildcard patterns independent of regex syntax or application lists.
    std::size_t p = 0, v = 0, star = std::string_view::npos, restart = 0;
    while (v < value.size()) {
        if (p < pattern.size() && (pattern[p] == '?' || lower(pattern[p]) == lower(value[v]))) {
            ++p;
            ++v;
        } else if (p < pattern.size() && pattern[p] == '*') {
            star = p++;
            restart = v;
        } else if (star != std::string_view::npos) {
            p = star + 1;
            v = ++restart;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*')
        ++p;
    return p == pattern.size();
}

bool any_match(const std::vector<std::string> &patterns, std::string_view name) {
    return std::any_of(patterns.begin(), patterns.end(),
                       [&](const auto &pattern) { return matches(pattern, name); });
}

} // namespace

ProcessFilter::ProcessFilter(SelectionPolicy policy) : policy_(std::move(policy)) {}

bool ProcessFilter::allows(std::string_view name) const {
    return !name.empty() && (policy_.include.empty() || any_match(policy_.include, name)) &&
           !any_match(policy_.exclude, name);
}

} // namespace phone_monitor::fps
