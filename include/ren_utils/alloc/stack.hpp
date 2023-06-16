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
#include <type_traits>
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


  /**
   * @brief Double stack allocator. Provides object allocation in two opposite stacks.
   *
   * This object allows us to dynamically allocate memory on either one of the two stacks. These
   * stacks are opposing one another in the same memory buffer. One of them is from the start (left) and
   * the other is from the end (right). Both stacks act like the StackAllocator.
   */
  class DoubleStackAllocator {
  public:
    /// Identify left or right stack in DoubleStackAllocator.
    enum class Side { LEFT = 0, RIGHT };
    /// Marks position on the left or right stack. This position is always on the edge of allocated memory.
    struct Marker {
      const Side side;
      const size_t idx;
    };
    /// Custom data for use with Ptr
    struct PtrData {
      Marker marker;
      DoubleStackAllocator* p_alloc;
    };

    /**
     * @brief Create DoubleStackAllocator with given memory size
     * @param total_size Total size of the memory buffer in bytes.
     */
    explicit DoubleStackAllocator(size_t total_size)
      : m_memory(nullptr)
      , m_totalMemSize(total_size)
      , m_left(0)
      , m_right(total_size)
    {
      assert(total_size != size_t(-1));
      m_memory = new uint8_t[total_size];
    }
    ~DoubleStackAllocator() { delete[] m_memory; }

    /**
     * @brief Allocate memory on left or right stack.
     * @param n_bytes Number of bytes to allocate
     * @return Valid pointer to memory or nullptr on failure. That is if the two stacks are overlaping.
     */
    template<Side S> void* Alloc(size_t n_bytes) {
      size_t m = m_left;
      if constexpr (S == Side::LEFT) {
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
    /// @return Marker to the current top of the left or right stack.
    template<Side S> inline Marker GetMarker() const {
      if constexpr (S == Side::LEFT)
        return Marker{ S, m_left };
      else
        return Marker{ S, m_right };
    }
    /**
     * @brief Free allocated memory up to given marker.
     * @note This will **NOT** destroy any object in this memory.
     * @param marker Marker to which to free memory.
     */
    void FreeToMarker(const Marker& marker) {
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
    /// Free all allocated memory on both stacks.
    inline void ClearAll() {
      m_left = 0;
      m_right = m_totalMemSize;
    }
    /// Free all allocated memory on left or right stack.
    template<Side S> inline void Clear() {
      if constexpr (S == Side::LEFT)
        m_left = 0;
      else
        m_right = m_totalMemSize;
    }
    /// @return Total memory size on which this allocator operates.
    size_t GetSize() const { return m_totalMemSize; }
    /// @return Current used size on the left or right stack.
    template<Side S> inline size_t GetCurrentSize() const {
      if constexpr (S == Side::LEFT)
        return m_left;
      else
        return m_totalMemSize - m_right;
    }
    /// @return True if left or right stack is empty.
    template<Side S> inline bool Empty() {
      if constexpr (S == Side::LEFT)
        return m_left == 0;
      else
        return m_right == m_totalMemSize;
    }
    /// @return True if both stacks are empty.
    inline bool EmptyBoth() { return Empty<Side::LEFT>() && Empty<Side::RIGHT>(); }

  private:
    uint8_t* m_memory{ nullptr };
    size_t m_totalMemSize;
    size_t m_left;
    size_t m_right;
  };

  using AllocSide = DoubleStackAllocator::Side;

  /**
   * @brief Create new instance of object allocated inside DoubleStackAllocator
   * @tparam T Type of the object to instantiate
   * @tparam S Left or Right stack to instantiate the object in.
   * @param alloc Instance of DoubleStackAllocator
   * @param args Arguments to pass to constructor
   * @return Pointer object wrapping this instance.
   * @exception std::runtime_error when there is no space for this object in DoubleStackAllocator.
   */
  template<class T, AllocSide S, class... Args>
  Ptr<T, DoubleStackAllocator::PtrData>
  new_ptr(DoubleStackAllocator& alloc, const Args&... args) {
    auto marker = alloc.GetMarker<S>();
    void* p_mem = alloc.Alloc<S>(sizeof(T));
    if (!p_mem)
      throw std::runtime_error(
          string_format("Cannot allocate memory in %s stack. Stacks would be overlapping. Total size = %lu, Left stack size = %lu. Right stack size = %lu. Wanted size = %lu",
            S == AllocSide::LEFT ? "LEFT" : "RIGHT", alloc.GetSize(), alloc.GetCurrentSize<AllocSide::LEFT>(), alloc.GetCurrentSize<AllocSide::RIGHT>(), sizeof(T))
          );
    Ptr<T, DoubleStackAllocator::PtrData> ptr{
      new (p_mem) T(args...),
      { marker, &alloc }
    };
    return ptr;
  }

  /**
   * @brief Destroy instance of object and free allocated memory by DoubleStackAllocator
   * @param ptr Pointer object returned by new_ptr
   * @warning Same problem as with delete_ptr(Ptr<T, typename StackAllocator::PtrData>& ptr) but for both stacks.
   */
  template<class T>
  void delete_ptr(Ptr<T, typename DoubleStackAllocator::PtrData>& ptr) {
    ptr->~T();
    ptr.m_PtrData.p_alloc->FreeToMarker(ptr.m_PtrData.marker);
    ptr.m_Ptr = nullptr;
  }
} // namespace ren_utils
