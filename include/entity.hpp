#include <cstdint>
#ifndef ENTITY_H
#define ENTITY_H
// Entity is merely an ID (unique)
using entity = std::uint32_t;
constexpr entity MAX_ENTITIES = 10000;
#endif