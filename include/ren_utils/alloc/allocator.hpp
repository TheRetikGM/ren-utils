/**
 * @brief This file provides utilities for easier work with allocators
 * @file allocator.hpp
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#pragma once
#include <cassert>
#include <type_traits>

namespace ren_utils {
  /**
   * @brief Class representing a pointer to some object allocated with some allocator.
   * @tparam T Custom object to hold pointer to. This would be user-defined object.
   * @tparam AllocPtrData Allocator-specific data for help with managing this pointer.
   */
  template<class T, class AllocPtrData>
  class Ptr {
  public:
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
  };

  /**
   * @brief Create a new pointer to object in given Allocator
   * @tparam T Object to store
   * @tparam Alloc Allocator to use
   * @tparam Args Arguments to pass to constructor
   * @note This function needs to be specialized for each allocator explicitly.
   */
  template<class T, class Alloc, class... Args>
  Ptr<T, typename Alloc::PtrData> new_ptr(Alloc, const Args&...) = delete;

  /**
   * @brief Delete pointer allocated with new_ptr() function.
   * @tparam T Object stored inside the pointer
   * @tparam Alloc Allocator used to allocate the object.
   * @note This function needs to be specialized for each allocator explicitly.
   */
  template<class T, class Alloc>
  void delete_ptr(Ptr<T, typename Alloc::PtrData>&) = delete;
} // namespace ren_utils
