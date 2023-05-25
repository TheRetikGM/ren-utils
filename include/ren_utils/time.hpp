/**
 * @brief Utilities which have something to do with time
 *        such as Stopwatch, Timer etc
 * @file time.h
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#pragma once
#include "basic.h"
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
   * @brief Convert this type info to string in format `YYYY-mm-dd HH:MM:SS`
   */
  std::string ToString() const {
    return string_format("%04i-%02i-%02i %02i:%02i:%02i", m_year,
                         m_month, m_day, m_hour, m_minute, m_second);
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
 * @brief Take elapsed time from one point to another
 *
 * This class can store some time-point and tell you elapsed time
 * from this point.
 */
class Stopwatch {
  using timer = std::chrono::system_clock;
public:
  Stopwatch() : m_clockStart(), m_clockEnd(), m_elapsedTime(timer::duration::zero()) {}

  /// Create a starting point in stopwatch.
  void Start() { m_clockStart = timer::now(); }

  /// Reset the elapsed time and create starting point.
  void Restart() {
    m_elapsedTime = timer::duration::zero();
    Start();
  }

  /// Create a stop point and compute elapsed time from start point.
  void Stop() {
    m_clockEnd = timer::now();
    m_elapsedTime = m_clockEnd - m_clockStart;
  }

  /// Reset the elapsed time.
  void Clear() { m_elapsedTime = timer::duration::zero(); }
  void Pause() {
    m_clockEnd = timer::now();
    m_elapsedTime += m_clockEnd - m_clockStart;
  }
  template<class units>
  long int ElapsedTime() {
    return static_cast<long int>(std::chrono::duration_cast<units>(m_elapsedTime).count());
  }
  float ElapsedSeconds()
  {
    return float(ElapsedTime<nanosec>()) / 1e9f;
  }
  float ElapsedMilliseconds()
  {
    return float(ElapsedTime<nanosec>()) / 1e6f;
  }

protected:
  timer::time_point m_clockStart;
  timer::time_point m_clockEnd;
  timer::duration	m_elapsedTime;
};

} // namespace ren_utils
