/**
 * @brief Interface for StackAllocator
 * @file StackAllocator.h
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
    /// Identifies position between allocated spaces in the stack.
    using Marker = size_t;
    const static Marker InvalidMarker = Marker(-1);
    struct PtrData{
      /// Marker identifying start of the object the pointer is pointing to.
      Marker marker{ StackAllocator::InvalidMarker };
      /// Pointer to Allocator where the object is allocated.
      StackAllocator* p_alloc{ nullptr };
    };

    /**
     * @brief Create new empty StackAllocator with given stack size
     * @param stack_size Size of the stack to allocate.
     */
    explicit StackAllocator(size_t stack_size);
    /// Free the entire stack on destrution.
    ~StackAllocator();

    /**
     * @brief Allocate memory on the stack
     * @param n_bytes Number of bytes to allocate
     * @return Pointer to the memory or `nullptr` on failure.
     */
    void* Alloc(size_t n_bytes);
    /**
     * @brief Allocate aligned memory on the stack
     * @param n_bytes Number of bytes to allocate
     * @param align Number of bytes to align to. This **MUST**  be a power of 2.
     * @note Supports up to 256 alignemt.
     * @return Pointer to the aligned memory or `nullptr` on failure.
     */
    void* AllocAligned(size_t n_bytes, size_t align);
    template<class T, class... Args>
    inline T* New(const Args&... args) {
      T* mem = reinterpret_cast<T*>(Alloc(sizeof(T)));
      return mem ? new (mem) T(args...) : nullptr;
    }
    template<class T, class... Args>
    inline T* NewAligned(const size_t& align, const Args&... args) {
      T* mem = reinterpret_cast<T*>(AllocAligned(sizeof(T), align));
      return mem ? new (mem) T(args...) : nullptr;
    }
    /**
     * @brief Get the base of the aligned memory address.
     * @param p_aligned_mem Pointer to the memory allocated with AllocAligned
     * @note Undefined behaviour if the `p_aligned_mem` was not allocated with
     *       AllocAligned.
     * @return Pointer to the base address.
     */
    void* GetAlignedBase(void* p_aligned_mem);
    /// @return marker to the current top of the stack.
    inline Marker GetMarker() const { return Marker(m_pTop); }
    /**
     * @brief Free all allocated memory up to the given marker.
     * @param marker Marker to free the memory up to
     * @exception std::invalid_argument on invalid marker
     */
    void FreeToMarker(const Marker& marker);
    /// Free all memory allocated in the stack allocator.
    inline void Clear() { m_pTop = 0; }
    /// @return Total stack size defined at construction.
    inline size_t GetSize() const { return m_totalStackSize; }
    /// @return Current used size on the stack.
    inline size_t GetCurrentSize() const { return m_pTop; }
    /// @return True if stack is empty.
    inline bool Empty() { return m_pTop == 0; }

  private:
    /// Holds the stack memory. All the allocations point to this memory.
    uint8_t* m_stack{ nullptr };
    /// Size of the stack.
    size_t m_totalStackSize;
    /// Pointer to the top of the stack (after the last element).
    size_t m_pTop;
  };

  template<class T, class Func>
  inline Ptr<T, StackAllocator>
  _new_ptr(StackAllocator& alloc, const Func& new_func) {
    auto marker = alloc.GetMarker();
    Ptr<T, StackAllocator> ptr{ new_func(), { marker, &alloc } };
    if (!ptr.m_Ptr)
      throw std::runtime_error(string_format("Cannot allocate memory for object in StackAllocator. StackAllocator stack size = %lu, wanted size = %lu", alloc.GetSize(), alloc.GetSize() + sizeof(T)));
    return ptr;
  }


  /**
   * @brief Create new instance of object allocated inside StackAllocator.
   * @tparam T Type of the object to instantiate
   * @param alloc Instance of StackAllocator
   * @param args Arguments to pass to constructor
   * @return Pointer object wrapping this instance.
   * @exception std::runtime_error when there is no space for this object in StackAllocator
   */
  template<class T, class... Args>
  Ptr<T, StackAllocator> new_ptr(StackAllocator& alloc, const Args&... args) {
    const auto& alloc_func = [&alloc, &args...] {
      return alloc.New<T>(args...);
    };
    return _new_ptr<T>(alloc, alloc_func);
  }

  /**
   * @brief Create new aligned instance of object allocated inside StackAllocator.
   * @tparam T Type of the object to instantiate
   * @param alloc Instance of StackAllocator
   * @param align Number of bytes to align instance to. This **MUST** be power of 2.
   * @param args Arguments to pass to constructor
   * @return Pointer object wrapping this instance.
   * @exception std::runtime_error when there is no space for this object in StackAllocator
   */
  template<class T, class... Args>
  Ptr<T, StackAllocator> new_ptr(StackAllocator& alloc, Align align, const Args&... args) {
    const auto& alloc_func = [&alloc, &align, &args...]{
      return alloc.NewAligned<T>(align, args...);
    };
    return _new_ptr<T>(alloc, alloc_func);
  }

  /**
   * @brief Destroy instance of object and free allocated memory in StackAllocator.
   * @param ptr Pointer object returned by new_ptr
   * @warning If pointer object that was representing object that is lower in the stack is deleted first, that that will invalidate this pointer.
   *          Currently this cannot be always detected, because if you delete a lower pointer object, then allocate new object and then try to delete
   *          the old higher pointer object, then it will not be detected.
   */
  template<class T>
  void delete_ptr(Ptr<T, StackAllocator>& ptr) {
    ptr.m_Ptr->~T();
    ptr.m_PtrData.p_alloc->FreeToMarker(ptr.m_PtrData.marker);
    ptr.m_Ptr = nullptr;
  }
} // namespace ren_utils

