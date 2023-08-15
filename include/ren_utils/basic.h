/**
 * @brief Provides basic utilities.
 * @file basic.h
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#pragma once
#include <random>
#include <string>

namespace ren_utils {

extern std::random_device rd;
extern std::mt19937 gen;

/// Return random integer in range <min, max>
inline int random_int(int min, int max) {
  max = max + 1;
  return std::rand() % (max - min) + min;
}

/// Return random float in range <0, 1>
inline float random_float() { return (float)std::rand() / (float)RAND_MAX; }

/// Return random float in range <min, max>
inline float random_float(float min, float max) {
  std::uniform_real_distribution<float> dis(min, max);
  return dis(gen);
}

/**
 * @brief Format function for string.
 * @param fmt Format of the output string
 * @vparam Format arguments.
 */
std::string string_format(std::string fmt, ...);

/// String format alias
#define format(...) string_format(__VA_ARGS__)

} // namespace ren_utils
