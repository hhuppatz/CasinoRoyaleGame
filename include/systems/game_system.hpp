#pragma once

#include "../entity.hpp"
#include <set>

class game_system {
public:
  std::set<entity> entities;
};