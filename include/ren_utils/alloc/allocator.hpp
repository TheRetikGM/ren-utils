/**
 * @brief This file provides utilities for easier work with allocators
 * @file allocator.hpp
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace ren_utils {
  /// Struct wrapping byte alignment size and providing useful functions for it.
  struct Align {
    size_t m_Bytes;
    explicit Align(size_t bytes) : m_Bytes(bytes) {}
    inline operator size_t() const { return m_Bytes; }
    inline bool operator==(const size_t& rhs) const { return m_Bytes == rhs; }
    inline bool operator!=(const size_t& rhs) const { return m_Bytes == rhs; }
    inline bool operator<(const size_t& rhs) const { return m_Bytes == rhs; }
    inline bool operator>(const size_t& rhs) const { return m_Bytes == rhs; }

    /**
     * @brief Align pointer to given byte boundary
     * @tparam T Type of the pointer to align
     * @param addr Pointer to align
     * @param align Boundary to align to. This **MUST** be power of 2.
     * @return Aligned pointer offsetted by up to `align - 1` bytes.
     */
    template<typename T>
    inline static T* Ptr(T* addr, size_t align) {
      uintptr_t p = reinterpret_cast<uintptr_t>(addr);
      uintptr_t aligned = Align::Ptr(p, align);
      return reinterpret_cast<T*>(aligned);
    }

    /**
     * @brief Align pointer to given byte boundary
     * @tparam T Type of the pointer to align
     * @param addr Pointer to align
     * @param align Boundary to align to. This **MUST** be power of 2.
     * @return Aligned pointer offsetted by up to `align - 1` bytes.
     */
    inline static uintptr_t Ptr(uintptr_t addr, size_t align) {
      const uintptr_t mask = align - 1;
      assert((align & mask) == 0);   // Power of 2
      return (addr + mask) & ~mask;
    }
  };

  /**
   * @brief Class representing a pointer to some object allocated with some allocator.
   * @tparam T Custom object to hold pointer to. This would be user-defined object.
   * @tparam AllocPtrData Allocator-specific data for help with managing this pointer.
   */
  template<class T, class Alloc>
  class Ptr {
  public:
    using AllocPtrData = typename Alloc::PtrData;
    /// Raw stored pointer of the object T.
    T* m_Ptr{ nullptr };
    /// Allocator-specific pointer data.
    AllocPtrData m_PtrData{};

    inline T* operator->() {
      assert(m_Ptr != nullptr);
      return m_Ptr;
    }
    inline T& Get() {
      assert(m_Ptr != nullptr);
      return *m_Ptr;
    }
    inline T& operator*() { return this->Get(); }
    inline operator bool() const { return m_Ptr; }
    inline bool operator==(const Ptr<T, AllocPtrData>& rhs) const { return m_Ptr == rhs.m_Ptr; }
    inline bool operator!=(const Ptr<T, AllocPtrData>& rhs) const { return m_Ptr != rhs.m_Ptr; }
    inline bool operator!() const { return !m_Ptr; }
  };

  /**
   * @brief Create a new pointer to object in given Allocator
   * @tparam T Object to store
   * @tparam Alloc Allocator to use
   * @tparam Args Arguments to pass to constructor
   * @note This function needs to be specialized for each allocator explicitly.
   */
  template<class T, class Alloc, class... Args>
  Ptr<T, Alloc> new_ptr(Alloc, const Args&...) = delete;

  /**
   * @brief Delete pointer allocated with new_ptr() function.
   * @tparam T Object stored inside the pointer
   * @tparam Alloc Allocator used to allocate the object.
   * @note This function needs to be specialized for each allocator explicitly.
   */
  template<class T, class Alloc>
  void delete_ptr(Ptr<T, Alloc>&) = delete;
} // namespace ren_utils
