/**
 * @brief Implementatoin of AvgSampler
 * @file AvgSampler.hpp
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#pragma once
#include <cstdint>
#include <functional>
#include "RingBuffer.hpp"

namespace ren_utils {

/// Mode of computing avarage.
enum class SampleMode: uint8_t {
  CONTINUOUS,   ///< Compute avarage from last N values.
  DISCREETE,    ///< Compute avarage from last N values after sampling N values.
};

/**
 * @brief Get avarage of the last N samples.
 *
 * Example:
 * \code{.cpp}
 * float a = 0.0f;
 * auto sampler = AvgSampler<float>(3, &a);
 * while (true) {
 *   printf("avg: %f\n", sampler.GetAvg());
 *   a += 0.1f;
 *   sampler.Sample();
 * }
 * \endcode
 *
 * @tparam T Type of the sampled value. This value needs to
 *           implement `+` and `/` operators.
 */
template<typename T> class AvgSampler {
public:
  using AvgCallback = std::function<void(const T&)>;
  using SampleGetter = std::function<T()>;

public:
  /**
   * @param n_samples Number of last samples from which to compute avarage
   * @param smaple Value to sample from
   * @param sample_mode Mode in which to compute sample avarage.
   */
  AvgSampler(uint32_t n_samples, const T* sample, SampleMode sample_mode = SampleMode::DISCREETE);
  /**
   * @param n_samples Number of last samples from which to compute avarage
   * @param sampler Sampling function used for getting sampled value.
   * @param sample_mode Mode in which to compute sample avarage.
   */
  AvgSampler(uint32_t n_samples, const SampleGetter& sampler, SampleMode sample_mode = SampleMode::DISCREETE);

  /**
   * @brief Set callback for when sampled avarage changes.
   * @param avg_callback Function to call when avarage is updated.
   */
  void SetAvgCallback(const AvgCallback& avg_callback) { m_avgCallback = avg_callback; }
  /// Perform a sample and update avarage if needed.
  void Sample();
  /// Get current avarage
  T GetAvg() const { return m_avg; }
  /// Get sample buffer
  const RingBuffer<T>& GetAvgBuf() { return m_avgBuf; }
  /// Set the sampling mode
  void SetMode(SampleMode mode) { m_mode = mode; }
private:
  // Number of samples after which to compute avarage.
  uint32_t m_totalSamples{ 0 };
  // Number of currently sampled values.
  uint32_t m_currentSamples{ 0 };
  // Current value of avarage.
  T m_avg{};
  // Sampling mode.
  SampleMode m_mode{ SampleMode::DISCREETE };
  // Function to call when avg is updated.
  AvgCallback m_avgCallback{ [](T){} };
  // Getter of samples.
  SampleGetter m_sampler;
  // Buffer containing all
  RingBuffer<T> m_avgBuf;

  /// Compute avarage of values in `m_avgBuf` and save it in `m_avg`.
  void computeAvgFromBuf();
};  // class AvgSampler

template<typename T> AvgSampler<T>::AvgSampler(uint32_t n_samples, const T* sample, SampleMode sample_mode)
  : m_totalSamples(n_samples)
  , m_mode(sample_mode)
  , m_sampler([sample]() { return *sample; })
  , m_avgBuf(RingBuffer<T>(n_samples))
{}

template<typename T> AvgSampler<T>::AvgSampler(uint32_t n_samples, const SampleGetter& sampler, SampleMode sample_mode)
  : m_totalSamples(n_samples)
  , m_mode(sample_mode)
  , m_sampler(sampler)
  , m_avgBuf(RingBuffer<T>(n_samples))
{}

template<typename T>
void AvgSampler<T>::computeAvgFromBuf() {
  m_avg = T{};
  for (auto& i : m_avgBuf)
    m_avg = m_avg + i;
  m_avg = m_avg / m_avgBuf.Size();
}

template<typename T>
void AvgSampler<T>::Sample() {
  m_avgBuf.PushBack(m_sampler());

  switch (m_mode) {
    case SampleMode::DISCREETE:
      m_currentSamples++;
      if (m_currentSamples == m_totalSamples) {
        m_currentSamples = 0;
        computeAvgFromBuf();
        m_avgCallback(m_avg);
      }
      break;
    case SampleMode::CONTINUOUS:
      computeAvgFromBuf();
      m_avgCallback(m_avg);
      break;
  }
}

} // namespace ren_utils

