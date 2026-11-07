#pragma once

#include <string_view>

namespace statetap {

// This is a compileable API sketch. Capture and transport begin after L5.
void initialize() noexcept;
void shutdown() noexcept;

template <typename T>
void tap(std::string_view topic, const T& value) noexcept {
    (void)topic;
    (void)value;
}

}  // namespace statetap

