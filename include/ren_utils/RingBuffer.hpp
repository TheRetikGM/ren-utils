/**
 * @brief Implementation of the ring buffer with dynamic size.
 * @file RingBuffer.hpp
 * @author Jakub Kloub (theretkgm@gmail.com)
 */
#pragma once
#include <stdexcept>
#include <optional>
#include <functional>
#include <string>

namespace ren_utils {
/**
 * @brief Ring buffer of given size.
 *
 * Example:
 * @code{.cpp}
 * auto buf = RingBuffer<int>(3);
 * buf.PushBack(1);  // { 1 }
 * buf.PushBack(2);  // { 1, 2 }
 * buf.PushBack(3);  // { 1, 2, 3 }
 * buf.PushBack(4);  // { 2, 3, 4 }
 * @endcode
 */
template <typename T> class RingBuffer {
public:
  /**
   * @brief Create new CircleBuffer
   * @param capacity Maximum size of the buffer, before it starts
   *                 from the start
   * @exception std::invalid_argument when capacity is 0
   */
  RingBuffer(size_t capacity);
  ~RingBuffer();

  /**
   * @brief Insert item to the back of the buffer
   * @param item Item to insert
   */
  T& PushBack(const T& item);
  /**
   * @brief Remove last element in buffer and return its copy
   * @exception std::length_error when the buffer is empty.
   */
  T PopBack();
  /**
   * @brief Remove first element in buffer and return its copy
   * @exception std::length_error when the buffer is empty.
   */
  T PopFront();
  /// Get the first item in buffer
  std::optional<std::reference_wrapper<T>> Front();
  /// Get the last item in buffer
  std::optional<std::reference_wrapper<T>> Back();
  /// Returns true if buffer is empty
  bool Empty() const;
  /// Remove all elements in the buffer
  void Clear();
  size_t Size() const { return m_size; }

  T& operator[](size_t index);
  /**
   * @exception std::out_of_range when index is out of range.
   */
  T& at(size_t index);
  std::optional<std::reference_wrapper<T>> try_at(size_t index);

  /**
   * @brief RingBuffer forward and backward iterator.
   */
  class iterator {
    RingBuffer<T>& buf;
    long long index{ 0 };
    int step{ 1 };
  public:
    /**
     * @param circle_buf Buffer to iterate over
     * @param index Index to start from
     * @param step Number of elements to step over when incremented
     */
    iterator(RingBuffer<T>& circle_buf, size_t index, int step = 1) : buf(circle_buf), index(index), step(step) {}

    iterator& operator++() {
      index += step;
      if (index <= 0)
        index = 0;
      else if (index > (long long int)buf.m_size)
        index = buf.m_size;
      return *this;
    }
    bool operator==(const iterator& other) const { return this->index == other.index; }
    bool operator!=(const iterator& other) const { return this->index != other.index; }
    T& operator*() { return buf.at((size_t)index); }
  };
  iterator begin() { return iterator(*this, 0); };
  iterator end() { return iterator(*this, m_size); };
  iterator rbegin() { return iterator(*this, m_size - 1, -1); };
  iterator rend() { return iterator(*this, -1); };

private:
  T* m_buf{ nullptr };
  size_t m_front{ 0 };
  size_t m_back{ 0 };
  size_t m_size{ 0 };
  size_t m_capacity{ 0 };

  // Construct in-place item copy.
  void newItem(const T& item, size_t buf_index);
  // Destroy in-place item copy.
  void deleteItem(size_t buf_index);
};

template<typename T>
RingBuffer<T>::RingBuffer(size_t capacity) : m_capacity(capacity) {
  if (capacity == 0)
    throw std::invalid_argument("Capacity cannot be 0");
  m_buf = (T*)malloc(sizeof(T) * capacity);
}

template<typename T>
RingBuffer<T>::~RingBuffer() {
  Clear();
  free(m_buf);
}

template<typename T>
void RingBuffer<T>::newItem(const T& item, size_t buf_index) {
  new (m_buf + buf_index) T(item);
}
template<typename T>
void RingBuffer<T>::deleteItem(size_t buf_index) {
  m_buf[buf_index].~T();
}

template<typename T>
T& RingBuffer<T>::PushBack(const T& item) {
  if (m_size == m_capacity) {
    deleteItem(m_front);
    m_front = (m_front + 1) % m_capacity;
  } else
    m_size++;

  newItem(item, m_back);
  size_t back = m_back;
  m_back = (m_back + 1) % m_capacity;

  return m_buf[back];
}

template<typename T>
T RingBuffer<T>::PopFront() {
  if (m_size == 0)
    throw std::length_error("Cannot pop front from empty buffer.");
  T t = m_buf[m_front];
  deleteItem(m_front);
  m_front = (m_front + 1) % m_capacity;
  m_size--;
  return t;
}

template<typename T>
T RingBuffer<T>::PopBack() {
  if (m_size == 0)
    throw std::length_error("Cannot pop back from empty buffer.");

  T t = m_buf[m_back];
  deleteItem(m_back);
  if (m_back == 0)
    m_back = m_capacity - 1;
  else
    m_back--;

  m_size--;
}

template<typename T>
std::optional<std::reference_wrapper<T>> RingBuffer<T>::Front() {
  if (m_size == 0)
    return {};
  return m_buf[m_front];
}

template<typename T>
std::optional<std::reference_wrapper<T>> RingBuffer<T>::Back() {
  if (m_size == 0)
    return {};
  return m_buf[m_back];
}

template<typename T>
bool RingBuffer<T>::Empty() const {
  return m_size == 0;
}

template<typename T>
void RingBuffer<T>::Clear() {
  for (size_t i = 0; i < m_size; i++)
    deleteItem((m_front + i) % m_capacity);
  m_front = m_back = m_size = 0;
}

template<typename T>
T& RingBuffer<T>::operator[](size_t index) {
  return m_buf[(m_front + index) % m_capacity];
}

template<typename T>
T& RingBuffer<T>::at(size_t index) {
  if (index >= m_size)
    throw std::out_of_range("Index " + std::to_string(index) + " is out of range.");
  return m_buf[(m_front + index) % m_capacity];
}

template<typename T>
std::optional<std::reference_wrapper<T>> RingBuffer<T>::try_at(size_t index) {
  if (index >= m_size)
    return {};
  return m_buf[(m_front + index) % m_capacity];
}

} // namespace ren_utils

