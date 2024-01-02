/**
 * @brief Utilities which have something to do with time
 *        such as Stopwatch, Timer etc
 * @file time.hpp
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#pragma once
#include <iomanip>
#include <sstream>
#include <string>
#include <ctime>
#include <chrono>
#include <functional>

namespace ren_utils {

/**
 * @brief Get information about current time and date.
 */
class TimeInfo {
public:
  int m_Year{0}, m_Month{0}, m_Day{0}, m_Hour{0}, m_Minute{0}, m_Second{0};

  /**
   * @brief Construct TimeInfo from given `std::time_t`
   * @param t Time struct to get the info from. By default it
   *          gets current time and date.
   */
  TimeInfo(std::time_t t = std::time(nullptr)) {
    m_tim = *localtime(&t);
    m_Year = m_tim.tm_year + 1900;
    m_Month = m_tim.tm_mon + 1;
    m_Day = m_tim.tm_mday;
    m_Hour = m_tim.tm_hour;
    m_Minute = m_tim.tm_min;
    m_Second = m_tim.tm_sec;
  }

  /**
   * @brief Convert this type info to string in specified format.
   */
  std::string ToString(const char* fmt = "%Y-%m-%d %H:%M:%S") const {
    std::ostringstream oss;
    oss << std::put_time(&m_tim, fmt);
    return oss.str();
  }

private:
  std::tm m_tim{};
};

using millisec = std::chrono::milliseconds;
using seconds = std::chrono::seconds;
using microsec = std::chrono::microseconds;
using nanosec = std::chrono::nanoseconds;

/**
 * @brief Measure time elapsed like with real stopwatch.
 *
 * Default units are seconds with floating point precision.
 * @tparam Prec Datatype to use for duration
 * @tparam Rat Ratio betwen seconds. For example: std::milli == std::ratio<1, 1000>
 */
template<typename Prec = float, typename Rat = std::ratio<1>>
class Stopwatch {
  using clock = std::chrono::system_clock;
  using time_point = std::chrono::time_point<clock>;
  using duration = std::chrono::duration<Prec, Rat>;
public:
  Stopwatch() : m_curDur(0) { }

  /// Start the stopwatch from last time point (or create new one)
  void Start() {
    m_startPoint = clock::now();
    m_stopped = false;
  }
  /// Create new starting point
  void Restart() { Clear(); Start(); }
  /// Create a stop time-point
  void Stop() {
    time_point end = clock::now();
    m_curDur += std::chrono::duration_cast<duration>(end - m_startPoint);
    m_startPoint = end;
    m_stopped = true;
  }
  /// Clear current elapsed time.
  void Clear() { m_curDur = duration(0); }
  /// Get elapsed time until now or to stop point.
  template<typename T>
  unsigned long ElapsedTime() {
    if (m_stopped)
      return std::chrono::duration_cast<T>(m_curDur);
    return std::chrono::duration_cast<T>(m_curDur + std::chrono::duration_cast<duration>(clock::now() - m_startPoint));
  }

  /// Get elapsed time until now or to stop point as `std::chrono::duration`.
  duration ElapsedTimeDur() {
    if (m_stopped)
      return m_curDur;
    return m_curDur + std::chrono::duration_cast<duration>(clock::now() - m_startPoint);
   }

private:
  // Time sections between start and stop points.
  time_point m_startPoint;
  // Stores duration when the stopwatch was not stopped.
  duration m_curDur;
  bool m_stopped{ false };
};


/**
 * @brief Time an execution of some function.
 */
class Timer {
public:
  bool m_Enabled{ false };  ///< If disabled, then Update() will have no effect.
  bool m_Repeat{ false };   ///< Repeat timer on timeout.
  uint32_t m_ID{ 0 };

  /// Create a new disabled timer.
  Timer() : m_func() { }

  /**
   * @brief Prepare the timer. This needs to be called for the timer to work.
   * @param timeout Number of seconds to timeout after
   * @param func Function to call on timeout
   * @param args Arguments to pass to `func`
   * @note This will enable the timer.
   */
  template<class Func, class... Args>
  void Prepare(float timeout, const Func& func, const Args&... args) {
    m_func = [func, args...](){ func(args...); };
    m_timeout = timeout;
    m_Enabled = true;
    m_currTime = 0.0f;
  }

  /**
   * @brief Prepare the timer. This needs to be called for the timer to work.
   * @param timeout Number of seconds to timeout after
   * @param func Member function to call on timeout
   * @param inst Pointer to the instance of the class which has `func` as a member function.
   * @param args Arguments to pass to `func`
   * @note This will enable the timer.
   */
  template<class Inst, class Func, class... Args, class = std::enable_if_t<std::is_member_function_pointer_v<Func>>>
  void Prepare(float timeout, const Func& func, const Inst& inst, const Args&... args) {
    m_func = [inst, func, args...](){ (inst->*func)(args...); };
    m_timeout = timeout;
    m_Enabled = true;
    m_currTime = 0.0f;
  }

  /**
   * @brief Reset the timer into the unprepared state.
   *
   * This will reset timeout funtion, timeout, current time state and will disable the timer.
   * @note This will **NOT** set the m_Repeat to false
   */
  void Reset() {
    m_func = {};
    m_timeout = 0.0f;
    m_currTime = 0.0f;
    m_Enabled = false;
  }

  /**
   * @brief Clear the current time state.
   *
   * Time state is the number of seconds this timer is running for (accumulated
   * from delta time of Update() method).
   * @note Timer will remain enabled.
   */
  void Clear() { m_currTime = 0.0f; }

  /**
   * @brief Update the timer time and call timeout function if needed.
   * @param dt Delta time
   */
  void Update(float dt) {
    if (!m_Enabled)
      return;

    m_currTime += dt;
    if (m_currTime > m_timeout) {
      m_func();
      Clear();
      m_Enabled = m_Repeat;
    }
  }

  /// @return Current counted time.
  const float& CurrentTime() const { return m_currTime; }
  /// @return  Prepared timeout.
  const float& Timeout() const { return m_timeout; }

private:
  std::function<void()> m_func;
  float m_timeout{ 0.0f };
  float m_currTime{ 0.0f };
};

} // namespace ren_utils
