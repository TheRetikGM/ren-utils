/**
 * @brief Definitions of DoubleStackAllocator class
 * @file stack.cpp
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#include "ren_utils/alloc/DoubleStackAllocator.h"
#include <cstdint>
#include <cassert>

using namespace ren_utils;

DoubleStackAllocator::DoubleStackAllocator(size_t total_size)
  : m_memory(nullptr)
  , m_totalMemSize(total_size)
  , m_left(0)
  , m_right(total_size)
{
  assert(total_size != size_t(-1));
  m_memory = new uint8_t[total_size];
}

DoubleStackAllocator::~DoubleStackAllocator() { delete[] m_memory; }

void* DoubleStackAllocator::Alloc(Side side, size_t n_bytes) {
  size_t m = m_left;
  if (side == Side::LEFT) {
    if (m_left + n_bytes > m_right)
      return nullptr;
    m_left += n_bytes;
  } else {
    if (n_bytes > m_right || m_right - n_bytes < m_left)
      return nullptr;
    m = m_right -= n_bytes;
  }
  return m_memory + m;
}

void* DoubleStackAllocator::Alloc(Side side, size_t n_bytes, Align align) {
  void* p_mem = Alloc(side, n_bytes + align - 1);
  return p_mem ? Align::AlignPtr(p_mem, align) : nullptr;
}

void DoubleStackAllocator::FreeToMarker(const Marker& marker) {
  const std::string errmsg = string_format(
      "This %s marker is not valid. It may have been implicitly freed by a call to FreeToMarker()"
      " with marker that was pointing to lower object in the stack.",
      marker.side == Side::LEFT ? "LEFT" : "RIGHT"
    );
  if (marker.side == Side::LEFT) {
    if (marker.idx > m_left)
      throw std::invalid_argument(errmsg);
    m_left = marker.idx;
  } else {
    if (marker.idx < m_right)
      throw std::invalid_argument(errmsg);
    m_right = marker.idx;
  }
}
