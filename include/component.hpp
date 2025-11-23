#include <bitset>
#include <cstdint>

#ifndef COMPONENT_H
#define COMPONENT_H
// MAX_COMPONENTS is the maximum number of components that can be used
constexpr std::uint8_t MAX_COMPONENTS = 32;
using signature = std::bitset<MAX_COMPONENTS>;
using component_type = std::uint8_t;
#endif