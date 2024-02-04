/**
 * @file basic.cpp
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#include "ren_utils/basic.h"
#include <cstdarg>

using namespace ren_utils;

std::random_device ren_utils::rd;
std::mt19937 ren_utils::gen(ren_utils::rd());

std::string ren_utils::string_format(std::string fmt, ...) {
  int size = 100;
  std::string str;
  va_list ap;

  while (1) {
    str.resize(size);
    va_start(ap, fmt);
    int n = vsnprintf(&str[0], size, fmt.c_str(), ap);
    va_end(ap);

    if (n > -1 && n < size) {
      str.resize(n); // Make sure there are no trailing zero char
      return str;
    }
    if (n > -1)
      size = n + 1;
    else
      size *= 2;
  }
}

