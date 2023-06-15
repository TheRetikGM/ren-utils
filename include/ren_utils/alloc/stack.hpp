/**
 * @brief Stack-based allocators.
 * @file stack.hpp
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <stdexcept>
#include "allocator.hpp"
#include "../basic.h"

namespace ren_utils {

  /**
   * @brief Stack allocator. Provides allocation of element in stack manner.
   *
   * This class is used for allocating dynamic objects in stack manner. New objects
   * are created on top of the stack in already pre-alocated memory. Only objects on
   * the top of the stack can be destroyed or user can destroy all objects to some
   * stack marker.
   */
  class StackAllocator {
  public:
    using Marker = size_t;
    const static Marker InvalidMarker = Marker(-1);
    struct PtrData{ Marker marker{ StackAllocator::InvalidMarker }; StackAllocator* p_alloc{ nullptr }; };

    /**
     * @brief Create new empty StackAllocator with given stack size
     * @param stack_size Size of the stack to allocate.
     */
    explicit StackAllocator(size_t stack_size)
      : m_totalStackSize(stack_size)
      , m_pTop(0)
    {
      assert(stack_size != size_t(-1));
      m_stack = new uint8_t[stack_size];
    }

    /// Free the entire stack on destrution.
    ~StackAllocator() {
      delete[] m_stack;
    }

    /**
     * @brief Allocate memory on the stack
     * @param n_bytes Number of bytes to allocate
     * @return Pointer to the memory or `nullptr` on failure.
     */
    void* Alloc(size_t n_bytes) {
      if (m_pTop + n_bytes >= m_totalStackSize)
        return nullptr;
      size_t p = m_pTop;
      m_pTop += n_bytes;
      return m_stack + p;
    }

    /// @return marker to the current top of the stack.
    Marker GetMarker() const { return Marker(m_pTop); }

    /**
     * @brief Free all allocated memory up to the given marker.
     * @param marker Marker to free the memory up to
     * @exception std::invalid_argument on invalid marker
     */
    void FreeToMarker(const Marker& marker) {
      if (marker > m_pTop)
        throw std::invalid_argument("This marker is not valid. It may have been implicitly freed by a call to FreeToMarker() with marker that was pointing to lower object in the stack.");
      m_pTop = marker;
    }

    /// Free all memory allocated in the stack allocator.
    void Clear() { m_pTop = 0; }

    /// @return Total stack size defined at construction.
    size_t GetSize() { return m_totalStackSize; }
    /// @return Current used size on the stack.
    size_t GetCurrentSize() { return m_pTop; }
    /// @return True of stack is empty.
    bool Empty() { return m_pTop == 0; }

  private:
    /// Holds the stack memory. All the allocations point to this memory.
    uint8_t* m_stack{ nullptr };
    /// Size of the stack.
    size_t m_totalStackSize;
    /// Pointer to the top of the stack (after the last element).
    size_t m_pTop;
  };

  /**
   * @brief Create new instance of object allocated inside StackAllocator.
   * @tparam T Type of the object to instantiate
   * @param alloc Instance of StackAllocator
   * @param args Arguments to pass to constructor
   * @return Pointer object wrapping this instance.
   * @exception std::runtime_error when there is no space for this object in StackAllocator
   */
  template<class T, class... Args>
  Ptr<T, StackAllocator::PtrData> new_ptr(StackAllocator& alloc, const Args&... args) {
    auto ptr = Ptr<T, StackAllocator::PtrData>();
    ptr.m_PtrData.marker = alloc.GetMarker();
    auto mem = alloc.Alloc(sizeof(T));
    if (!mem)
      throw std::runtime_error(string_format("Cannot allocate memory for object in StackAllocator. StackAllocator stack size = %lu, wanted size = %lu", alloc.GetSize(), alloc.GetSize() + sizeof(T)));
    ptr.m_PtrData.p_alloc = &alloc;
    ptr.m_Ptr = new (mem) T(args...);
    return ptr;
  }

  /**
   * @brief Destroy instance of object and free allocated memory in StackAllocator.
   * @param ptr Pointer object returned by new_ptr
   * @warning If pointer object that was representing object that is lower in the stack is deleted first, that that will invalidate this pointer.
   *          Currently this cannot be always detected, because if you delete a lower pointer object, then allocate new object and then try to delete
   *          the old higher pointer object, then it will not be detected.
   */
  template<class T>
  void delete_ptr(Ptr<T, StackAllocator::PtrData>& ptr) {
    ptr.m_Ptr->~T();
    ptr.m_PtrData.p_alloc->FreeToMarker(ptr.m_PtrData.marker);
    ptr.m_Ptr = nullptr;
  }
} // namespace ren_utils
