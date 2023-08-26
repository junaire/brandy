#pragma once

#include "json.hpp"

namespace nl = nlohmann;

inline constexpr std::string_view kTerminators[] = {"jmp", "br", "ret"};
