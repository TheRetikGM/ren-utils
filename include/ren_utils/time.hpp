/**
 * @brief Utilities which have something to do with time
 *        such as Stopwatch, Timer etc
 * @file time.hpp
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#pragma once
#include "basic.h"
#include <iomanip>
#include <sstream>
#include <string>
#include <ctime>
#include <chrono>

namespace ren_utils {

/**
 * @brief Get information about current time and date.
 */
class TimeInfo {
public:
  const int &m_Year = m_year;    ///< Year from 0
  const int &m_Month = m_month;  ///< Month 1-12
  const int &m_Day = m_day;      ///< Day in given month
  const int &m_Hour = m_hour;    ///< Hour 0-23
  const int &m_Minute = m_minute;
  const int &m_Second = m_second;

  /**
   * @brief Construct TimeInfo from given `std::time_t`
   * @param t Time struct to get the info from. By default it
   *          gets current time and date.
   */
  TimeInfo(std::time_t t = std::time(nullptr)) {
    m_tim = *localtime(&t);
    m_year = m_tim.tm_year + 1900;
    m_month = m_tim.tm_mon + 1;
    m_day = m_tim.tm_mday;
    m_hour = m_tim.tm_hour;
    m_minute = m_tim.tm_min;
    m_second = m_tim.tm_sec;
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
  int m_year{0}, m_month{0}, m_day{0}, m_hour{0}, m_minute{0}, m_second{0};
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

} // namespace ren_utils
