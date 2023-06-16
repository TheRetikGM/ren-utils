/**
 * @brief Provides definitions of classes from stack.h
 * @file stack.cpp
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#include "ren_utils/alloc/stack.hpp"

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
