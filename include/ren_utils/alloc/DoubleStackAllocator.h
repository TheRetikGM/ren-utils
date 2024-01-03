/**
 * @brief Interface for DoubleStackAllocator
 * @file StackAllocator.h
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#pragma once
#include <stdexcept>
#include "allocator.hpp"
#include "../basic.h"

namespace ren_utils {

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
    explicit DoubleStackAllocator(size_t total_size);
    ~DoubleStackAllocator();

    /**
     * @brief Allocate memory on left or right stack.
     * @param n_bytes Number of bytes to allocate
     * @return Valid pointer to memory or nullptr on failure. That is if the two stacks are overlaping.
     */
    void* Alloc(Side side, size_t n_bytes);
    /**
     * @brief Allocate aligned memory on left or right stack.
     * @param side Side of the stack
     * @param n_bytes Number of bytes to allocate
     * @param align Number of bytes to align to
     * @return Pointer to the aligned memory or `nullptr` on failure
     * @note This will allocate total of `n_bytes + align - 1` bytes
     */
    void* Alloc(Side side, size_t n_bytes, Align align);

    /**
     * @brief Allocate memory and construct object in allocator
     * @param side Stack to use
     * @param args Arguments to pass to object constructor
     * @return Pointer to object or `nullptr` on failure.
     */
    template<class T, class... Args>
    T* New(Side side, const Args&... args) {
      T* p_mem = reinterpret_cast<T*>(Alloc(side, sizeof(T)));
      return p_mem ? new (p_mem) T(args...) : nullptr;
    }

    /**
     * @brief Allocate aligned memory and construct object in allocator.
     * @param side Stack to use
     * @param align Number of bytes to align object to
     * @param args Arguments to pass to object constructor
     * @return Aligned pointer to object or `nullptr` on failure
     */
    template<class T, class... Args>
    T* New(Side side, Align align, const Args&... args) {
      T* p_mem = reinterpret_cast<T*>(Alloc(side, sizeof(T), align));
      return p_mem ? new (p_mem) T(args...) : nullptr;
    }

    /// @return Marker to the current top of the left or right stack.
    inline Marker GetMarker(Side side) const {
      if (side == Side::LEFT)
        return Marker{ side, m_left };
      return Marker{ side, m_right };
    }
    /**
     * @brief Free allocated memory up to given marker.
     * @note This will **NOT** destroy any object in this memory.
     * @param marker Marker to which to free memory.
     */
    void FreeToMarker(const Marker& marker);
    /// Free all allocated memory on both stacks.
    inline void ClearAll() {
      m_left = 0;
      m_right = m_totalMemSize;
    }
    /// Free all allocated memory on left or right stack.
    inline void Clear(Side side) {
      if (side == Side::LEFT)
        m_left = 0;
      else
        m_right = m_totalMemSize;
    }
    /// @return Total memory size on which this allocator operates.
    size_t GetSize() const { return m_totalMemSize; }
    /// @return Current used size on the left or right stack.
    inline size_t GetCurrentSize(Side side) const {
      if (side == Side::LEFT)
        return m_left;
      return m_totalMemSize - m_right;
    }
    /// @return True if left or right stack is empty.
    inline bool Empty(Side side) {
      if (side == Side::LEFT)
        return m_left == 0;
      return m_right == m_totalMemSize;
    }
    /// @return True if both stacks are empty.
    inline bool EmptyBoth() { return Empty(Side::LEFT) && Empty(Side::RIGHT); }

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
   * @note For aligned memory allocation pass `Align` as the first `args` parameter.
   */
  template<class T, class... Args>
  Ptr<T, DoubleStackAllocator>
  new_ptr(DoubleStackAllocator& alloc, AllocSide side, const Args&... args) {
    auto marker = alloc.GetMarker(side);
    Ptr<T, DoubleStackAllocator> ptr{ alloc.New<T>(side, args...), { marker, &alloc } };
    if (!ptr.m_Ptr)
      throw std::runtime_error(
          string_format("Cannot allocate memory in %s stack. Stacks would be overlapping. Total size = %lu, Left stack size = %lu. Right stack size = %lu. Wanted size = %lu",
            side == AllocSide::LEFT ? "LEFT" : "RIGHT", alloc.GetSize(),
            alloc.GetCurrentSize(AllocSide::LEFT),
            alloc.GetCurrentSize(AllocSide::RIGHT), sizeof(T))
          );
    return ptr;
  }

  /**
   * @brief Destroy instance of object and free allocated memory by DoubleStackAllocator
   * @param ptr Pointer object returned by new_ptr
   * @warning Same problem as with delete_ptr(Ptr<T, typename StackAllocator::PtrData>& ptr) but for both stacks.
   */
  template<class T>
  void delete_ptr(Ptr<T, DoubleStackAllocator>& ptr) {
    ptr.m_Ptr->~T();
    ptr.m_PtrData.p_alloc->FreeToMarker(ptr.m_PtrData.marker);
    ptr.m_Ptr = nullptr;
  }
} // namespace ren_utils
