/**
 * @brief Definitions for StackAllocator class
 * @file stack.cpp
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#include "ren_utils/alloc/StackAllocator.h"
#include <cstddef>
#include <cstdint>
#include <cassert>

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

void* StackAllocator::AllocAligned(size_t n_bytes, size_t align) {
  // Store enough bytes for even worst case scenario (align - 1),
  // and store one more byte for storing the align shift.
  size_t total_bytes = n_bytes + align;

  // Allate enough memory for the alignment.
  uint8_t* p_mem = (uint8_t*)Alloc(total_bytes);
  if (!p_mem)
    return nullptr;

  // Align the pointer and store the alignment.
  uint8_t* p_aligned_mem = Align::AlignPtrStore(p_mem, align);
  return p_aligned_mem;
}

void* StackAllocator::GetAlignedBase(void* p_aligned_mem) {
  if (!p_aligned_mem)
    return nullptr;
  return Align::UnalignPtr(p_aligned_mem);
}

void StackAllocator::FreeToMarker(const Marker& marker) {
  if (marker > m_pTop)
    throw std::invalid_argument("This marker is not valid. It may have been implicitly freed by a call to FreeToMarker() with marker that was pointing to lower object in the stack.");
  m_pTop = marker;
}

