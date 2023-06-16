/**
 * @brief Provides definitions of classes from stack.h
 * @file stack.cpp
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#include "ren_utils/alloc/stack.h"

using namespace ren_utils;

StackAllocator::StackAllocator(size_t stack_size)
  : m_totalStackSize(stack_size)
  , m_pTop(0)
{
  if (stack_size == 0 || stack_size == InvalidMarker)
    throw std::invalid_argument(string_format("Invalid stack_size{ %lu }. It cannot be equal to 0 or %lu.", stack_size, InvalidMarker));
  m_stack = new uint8_t[stack_size];
}

StackAllocator::~StackAllocator() {
  delete[] m_stack;
}

void* StackAllocator::Alloc(size_t n_bytes) {
  if (m_pTop + n_bytes > m_totalStackSize)
    return nullptr;
  size_t p = m_pTop;
  m_pTop += n_bytes;
  return m_stack + p;
}

void StackAllocator::FreeToMarker(const Marker& marker) {
  if (marker > m_pTop)
    throw std::invalid_argument("This marker is not valid. It may have been implicitly freed by a call to FreeToMarker() with marker that was pointing to lower object in the stack.");
  m_pTop = marker;
}

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
