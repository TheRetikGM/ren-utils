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
    inline static T* AlignPtr(T* addr, size_t align) {
      uintptr_t p = reinterpret_cast<uintptr_t>(addr);
      uintptr_t aligned = Align::AlignPtr(p, align);
      return reinterpret_cast<T*>(aligned);
    }

    /**
     * @brief Align pointer to given byte boundary
     * @tparam T Type of the pointer to align
     * @param addr Pointer to align
     * @param align Boundary to align to. This **MUST** be power of 2.
     * @return Aligned pointer offsetted by up to `align - 1` bytes.
     */
    inline static uintptr_t AlignPtr(uintptr_t addr, size_t align) {
      const uintptr_t mask = align - 1;
      assert((align & mask) == 0);   // Power of 2
      return (addr + mask) & ~mask;
    }

    /**
     * @brief Align pointer and store the shift in the one byte before align.
     * @param orig Pointer to align
     * @param align Number of bytes to align to
     * @return Aligned pointer (original shifted by atleast 1 to `align` bytes)
     * @note The memory pointed to by `orig` **MUST** have space for additional `align` bytes. That
     *       is `sizeof(T)+align` bytes.
     */
    template<typename T>
    static T* AlignPtrStore(T* orig, size_t align) {
      uint8_t* p_orig = reinterpret_cast<uint8_t*>(orig);
      uint8_t* p_aligned = reinterpret_cast<uint8_t*>(AlignPtr(orig, align));

      if (p_orig == p_aligned)
        p_aligned += align;
      ptrdiff_t shift = p_aligned - p_orig;

      // We store shift in one byte and 0 represents 256, so
      // this is the limit.
      assert(shift > 0 && shift <= 256);

      // Store shift in the one byte before aligned address.
      p_aligned[-1] = static_cast<uint8_t>(shift & 0xff);
      return reinterpret_cast<T*>(p_aligned);
    }

    /**
     * @brief Unalign the pointer aligned with AlignPtrStore
     * @param aligned Pointer aligned with AlignPtrStore
     * @return Original unaligned pointer
     */
    template<typename T>
    static T* UnalignPtr(T* aligned) {
      uint8_t* p_aligned = reinterpret_cast<uint8_t*>(aligned);
      ptrdiff_t shift = p_aligned[-1];
      if (shift == 0)
        shift = 256;
      p_aligned -= shift;
      return reinterpret_cast<T*>(p_aligned);
    }

    template<typename T>
    inline static bool IsAligned(T* ptr, size_t align) {
      return reinterpret_cast<uintptr_t>(ptr) % align == 0;
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
