/**
 * @brief Provides definitions of classes from stack.h
 * @file stack.cpp
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#include "ren_utils/alloc/stack.h"
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

  // Align the allocated pointer. This will add max `align - 1` bytes to it.
  uint8_t* p_aligned_mem = Align::Ptr(p_mem, align);
  if (p_aligned_mem == p_mem)
    p_aligned_mem += align;   // So that we can store the shift

  // Store the number of shifted bytes in the one byte before the allocated address.
  ptrdiff_t shift = p_aligned_mem - p_mem;
  assert(shift > 0 && shift <= 256);
  p_aligned_mem[-1] = static_cast<uint8_t>(shift & 0xff);

  return p_aligned_mem;
}

void* StackAllocator::GetAlignedBase(void* p_aligned_mem) {
  if (!p_aligned_mem)
    return nullptr;
  uint8_t* p_amem = reinterpret_cast<uint8_t*>(p_aligned_mem);
  ptrdiff_t shift = p_amem[-1];
  if (shift == 0)
    shift = 256;
  uint8_t* p_base = p_amem - shift;
  return reinterpret_cast<void*>(p_base);
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
