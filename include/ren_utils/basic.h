/**
 * @brief Provides basic utilities.
 * @file basic.h
 * @author Jakub Kloub (theretikgm@gmail.com)
 */
#pragma once
#include <cstdarg>
#include <ctime>
#include <random>

namespace ren_utils {

extern std::random_device rd;
extern std::mt19937 gen;

/// Return random integer in range <min, max>
inline int random_int(int min, int max) {
  max = max + 1;
  return std::rand() % (max - min) + min;
}

/// Return random float in range <0, 1>
inline float random_float() { return (float)std::rand() / (float)RAND_MAX; }

/// Return random float in range <min, max>
inline float random_float(float min, float max) {
  std::uniform_real_distribution<float> dis(min, max);
  return dis(gen);
}

/**
 * @brief Format function for string.
 * @param fmt Format of the output string
 * @vparam Format arguments.
 */
std::string string_format(std::string fmt, ...);

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

} // namespace ren_utils
