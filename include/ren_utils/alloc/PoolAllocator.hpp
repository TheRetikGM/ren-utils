/**
 * @brief Provide pool-allocator interface.
 * @file pool.hpp
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#pragma once
#include "allocator.hpp"
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <stdexcept>

namespace ren_utils {
  /**
   * @brief Allocator storing items of type `T` in pools.
   *
   * This allocator stores items of type `T` in a pool. This is achieved by allocating a
   * chunk of memory which holds a given number of elements. Initially in the space for
   * each element is stored a pointer to the next element. This creates a linked-list of
   * free elements. When new item is allocated, we just replace the first pointer with
   * object data and change the starting pointer.
   *
   * @tparam T Item object type to store in this pool.
   * @note Because of the current implementation the *least memory* the item will take
   *       is `sizeof(uint8_t*)`. That is because we store the `next` pointers in each
   *       item memory space and thus we need atleast this much. We could avoid this by
   *       storing indices of items in the `pool` instead of pointers, however that
   *       would limit us to `2^(sizeof(T) * CHAR_BIT)` number of items as the index
   *       could be only as big as the type itself.
   */
  template<class T>
  class PoolAllocator {
  public:
    using PtrData = PoolAllocator*;

    /**
     * @brief Create a new PoolAllocator object with given number of items of type `T` and alignment.
     * @param num_items Maximum number of items to hold.
     * @param align Byte-alignment to align each item to when allocated. This **MUST** be power of 2.
     */
    explicit PoolAllocator(size_t num_items, size_t align = 1);
    ~PoolAllocator() { free(m_pool); }

    /// Allocate aligned memory for object of type `T`.
    T* Alloc();
    /**
     * @brief Allocate and instantiate object of type `T`.
     * @param args Arguments to pass to constructor.
     * @return Pointer to object or `nullptr` on failure.
     */
    template<class... Args>
    T* New(const Args&... args) {
      auto mem = Alloc();
      return mem ? new (mem) T(args...) : nullptr;
    }

    /**
     * @brief Free memory of item pointer to by `ptr`.
     * @param ptr Item to free.
     * @note If ptr is `nullptr` then nothing will happen.
     */
    void Free(T* ptr);
    /**
     * @brief Destroy and free item pointed to by `ptr`.
     * @param ptr Item to delete.
     * @note If ptr is `nullptr` then nothing will happen.
     */
    void Delete(T* ptr);

    /// Get number of free items in allocator.
    inline size_t GetFree() const { return m_total - m_used; }
    /// Get number of used items in allocator.
    inline const size_t& GetUsed() const { return m_used; }
    /// Get total number of items in allocator.
    inline const size_t& GetTotal() const { return m_total; }
    /// Get Byte-alignment of items in allocator.
    inline const size_t& GetAlign() const { return m_align; }

  protected:
    /// Array holding all of the items.
    uint8_t* m_pool;
    /// Pointer to the first item in item list.
    uint8_t** m_first;
    /// Total number of items.
    size_t m_total;
    /// Used number of items.
    size_t m_used;
    /// Number of bytes to align to.
    size_t m_align;
  };

  template<class T>
  PoolAllocator<T>::PoolAllocator(size_t num_items, size_t align)
    : m_total(num_items)
    , m_used(0)
    , m_align(align)
  {
    assert(num_items != 0);
    assert(align > 0 && align <= 256);    // Current implementation limit.

    // max(sizeof(T), sizeof(uint8_t*))
    size_t item_size = sizeof(T) >= sizeof(uint8_t*) ? sizeof(T) : sizeof(uint8_t*);
    // Allocate memory for all items. We allocate one more byte for each item, so
    // we can store the alignment shift.
    m_pool = (uint8_t*)malloc((num_items + align) * item_size);
    // Step to take when iterating over all elements in pool.
    size_t step = item_size + align;

    // Set all pointers in the pool to point to their right neighbour.
    for (size_t i = 0; i < num_items; i++) {
      auto p = reinterpret_cast<uint8_t**>(m_pool + i * step);
      *p = (i == num_items - 1) ? nullptr : m_pool + (i + 1) * step;
    }

    // Set starting pointer to point to the first item in pool.
    m_first = reinterpret_cast<uint8_t**>(m_pool);
  }

  template<class T>
  T* PoolAllocator<T>::Alloc() {
    if (m_used == m_total)
      return nullptr;

    // The first item in the linked list will be used.
    auto p_raw = reinterpret_cast<uint8_t*>(m_first);

    // Set the starting pointer to the pointer stored in the first item.
    m_first = reinterpret_cast<uint8_t**>(*m_first);

    // Align the pointer
    uint8_t* aligned_p = Align::Ptr(p_raw, m_align);
    if (aligned_p == p_raw)
      aligned_p += m_align;   // Make space for storing the `shift`.

    // Store how much we shifted the pointer to the first byte *before* the returned pointer.
    ptrdiff_t shift = aligned_p - p_raw;
    assert(shift > 0 && shift <= 256);
    *(aligned_p - 1) = static_cast<uint8_t>(shift & 0xff);

    m_used++;
    return (T*)aligned_p;
  }

  template<class T>
  void PoolAllocator<T>::Free(T* ptr) {
    if (!ptr)
      return;

    // Get the alignment shift and get the original pointer.
    auto u8_ptr = reinterpret_cast<uint8_t*>(ptr);
    ptrdiff_t shift = u8_ptr[-1];
    if (shift == 0)
      shift = 256;
    u8_ptr -= shift;

    // Store pointer to the first item in memory pointed to by `u8_ptr`.
    auto p = reinterpret_cast<uint8_t**>(u8_ptr);
    *p = reinterpret_cast<uint8_t*>(m_first);

    // The first item is now `ptr` as we have added it to start of the list.
    m_first = p;
    m_used--;
  }

  template<class T>
  void PoolAllocator<T>::Delete(T* ptr) {
    if (!ptr)
      return;
    ptr->~T();
    Free(ptr);
  }

  /**
   * @brief Create a new pointer to object in given Allocator
   * @tparam T Object to store
   * @tparam Alloc Allocator to use
   * @tparam Args Arguments to pass to constructor
   * @exception std::runtime_error When object cannot be created in PoolAllocator.
   */
  template<class T, class... Args>
  auto new_ptr(PoolAllocator<T>& alloc, const Args&... args) {
    Ptr<T, PoolAllocator<T>> ptr { alloc.New(args...), &alloc };
    if (!ptr.m_Ptr)
      throw std::runtime_error("Failed to create object in PoolAllocator. Number of free items: " + std::to_string(alloc.GetFree()));
    return ptr;
  }

  /**
   * @brief Delete pointer allocated with new_ptr() function.
   * @tparam T Object stored inside the pointer
   * @tparam Alloc Allocator used to allocate the object.
   * @param ptr Pointer created by new_ptr<T, typename PoolAllocator<T>::PtrData>()
   */
  template<class T>
  void delete_ptr(Ptr<T, PoolAllocator<T>>& ptr) {
    ptr.m_PtrData->Delete(ptr.m_Ptr);
  }
};  // namespace ren_utils
