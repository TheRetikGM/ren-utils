/**
 * @brief Logging utilities
 * @file logging.hpp
 * @author Jakub Kloub (theretikgm@gmail.com)
 *
 * Example logging workflow:
 * \code{.cpp}
 * FILE* f = fopen("log.txt", "w");
 * LogEmitter::AddListener<StreamLogger>(StreamArr{ stdout, f });
 * LOG_S("Some useful status information..");
 * LOG_E("Critical error!");
 * LogEmitter::GetListener<StreamLogger>()->Erase(stdout);
 * LOG_I("This information log is only in log.txt");
 * fclose(f);
 * \endcode
 */
#pragma once
#include "time.hpp"
#include <filesystem>
#include <thread>
#include <mutex>
#include <unordered_map>


namespace ren_utils {

/// Identify the log type
enum class LogLevel : int {
  info = 0, ///< Information log
  status,   ///< Status information
  warning,  ///< Warning about something
  error,    ///< Something fails, but we can still recover
  critical, ///< Program cannot continue
};

/// Index this with LogLevel to get a string representation of LogLevel.
static constexpr const char *LOG_LEVEL_STRINGS[] = {"Info", "Status", "Warning",
                                                    "Error", "Critical"};

/// Data of a single log entry.
struct LogInfo {
  /// When was the log created.
  TimeInfo time_info{ TimeInfo() };
  /// Type of log.
  LogLevel level{ LogLevel::info };
  /// Thread from which the log was emmited.
  std::thread::id thread{std::this_thread::get_id()};
  /// Logged source file.
  std::filesystem::path file{""};
  /// Logged source file line number.
  uint32_t line{0};
  /// Message of the log.
  std::string message{""};
};

/**
 * @brief Interface of log listeners
 *
 * Log listener is a class, which waits for emmited log
 * from LogEmitter. It decides what happens to the
 * emmited logs.
 */
class LogListener {
public:
  virtual ~LogListener() {}
  /**
   * @brief Function called when LogEmitter emits a log
   * @param log Log emitted from LogEmitter
   */
  virtual void OnLog(const LogInfo& log) = 0;
};

using StreamArr = std::vector<std::FILE*>;  // Used in Ren::StreamLogger

/**
 * @brief Outputs logs to `C` file streams
 *
 * Example:
 * \code{.cpp}
 * FILE* log_file = fopen("log.txt", "w");
 * LogEmitter::AddListener<StreamLogger>(StreamArr{ stdout, log_file });
 * \endcode
 */
class StreamLogger : public LogListener {
public:
  StreamArr m_Streams{};

  /**
   * @brief Construct StreamLogger which outputs to given streams.
   * @param streams Valid open C files to output logs to.
   */
  StreamLogger(StreamArr streams = { stdout }) {
    for (auto &&s : streams)
      m_Streams.push_back(s);
  }

  void OnLog(const LogInfo& log) override {
    for (std::FILE *stream : m_Streams)
      std::fprintf(
          stream, "%s    %12s    %15s:%-4i    %s\n",
          log.time_info.ToString().c_str(), LOG_LEVEL_STRINGS[(int)log.level],
          log.file.filename().string().c_str(), log.line, log.message.c_str());
  };

  /**
   * @brief Remove C file from output files.
   * @param file File which was specified at construction
   */
  void Erase(std::FILE* file) {
    int i = 0;
    for (i = 0; i < (int)m_Streams.size(); i++)
      if (m_Streams[i] == file)
        break;
    if (i < (int)m_Streams.size())
      m_Streams.erase(m_Streams.begin() + i);
  }
};

/**
 * @brief Singleton class, which emmits logs to all registered listeners
 *
 * This class creates and ownes instances of added listeners. Each type of listener
 * can only have a single added instance.
 *
 * @note You can use macros LOG_I(), LOG_E() etc. as shorthands for logging.
 */
class LogEmitter {
public:
  /**
   * @brief Emmit a log to all listeners.
   * @param level Log type
   * @param message Message stored in log.
   * @param file File stored in log. For example source file.
   * @param line Line stored in log. For exampel source file line number.
   */
  static void Log(LogLevel level, std::string message,
                  std::filesystem::path file, uint32_t line) {
    LogInfo log;
    log.level = level;
    log.message = message;
    log.file = file;
    log.line = line;

    for (auto&& [_, listener] : s_logListeners)
      listener->OnLog(log);
  }

  /**
   * @brief Create instance of LogListener child class using args.
   * @param args Arguments passed to the constructor
   * @tparam T Type of the child class to construct
   */
  template <typename T, typename... Args>
  static T* AddListener(Args... args) {
    int id = getID<T>();
    if (s_logListeners.count(id) != 0)
      return dynamic_cast<T *>(s_logListeners[id].get());

    s_logListeners[id] =
        std::shared_ptr<LogListener>(new T(std::forward<Args>(args)...));
    return dynamic_cast<T *>(s_logListeners[id].get());
  }

  /**
   * @brief Create instance of LogListener child class using initializer list.
   * @param init_list Initializer list passed to the constructor
   * @tparam T Type of the child class to construct
   * @tparam I Type contained in initializer list
   */
  template <typename T, typename I>
  static T* AddListener(std::initializer_list<I> init_list) {
    int id = getID<T>();
    if (s_logListeners.count(id) != 0)
      return dynamic_cast<T *>(s_logListeners[id].get());

    s_logListeners[id] = std::shared_ptr<LogListener>(new T(init_list));
    return dynamic_cast<T *>(s_logListeners[id].get());
  }

  /**
   * @brief Create instance of LogListener child class with
   *        default constructor.
   * @tparam T Type of the child class to construct
   */
  template <typename T>
  static T* AddListener() {
    int id = getID<T>();
    if (s_logListeners.count(id) != 0)
      return dynamic_cast<T *>(s_logListeners[id].get());

    s_logListeners[id] = std::shared_ptr<LogListener>(new T());
    return dynamic_cast<T *>(s_logListeners[id].get());
  }

  /**
   * @brief Get instsance of the added listener.
   * @tparam T Type of LogListener child. This type must have
   *           been constructed using AddListener<T>() method
   */
  template <typename T>
  static T* GetListener() {
    int id = getID<T>();
    if (s_logListeners.count(id) != 0)
      return dynamic_cast<T*>(s_logListeners[id].get());
    else
      return nullptr;
  }

  /**
   * @brief Destroy the instance of given added listener.
   * @tparam T Type of LogListener child. This type must have
   *           been constructed using AddListener<T>() method
   */
  template <typename T>
  static void RemoveListener() {
    s_logListeners.erase(getID<T>());
  }

protected:
  inline static std::unordered_map<int, std::shared_ptr<LogListener>>
      s_logListeners{};
  inline static int s_lastID{0};

  template <typename T> static int getID() {
    static int current_id = s_lastID++;
    return current_id;
  }
};

/// Emit information log.
#define LOG_I(message) ren_utils::LogEmitter::Log(ren_utils::LogLevel::info, message, __FILE__, __LINE__)
/// Emit statuc log.
#define LOG_S(message) ren_utils::LogEmitter::Log(ren_utils::LogLevel::status, message, __FILE__, __LINE__)
/// Emit warning log.
#define LOG_W(message) ren_utils::LogEmitter::Log(ren_utils::LogLevel::warning, message, __FILE__, __LINE__)
/// Emit error log.
#define LOG_E(message) ren_utils::LogEmitter::Log(ren_utils::LogLevel::error, message, __FILE__, __LINE__)
/// Emit critical error log.
#define LOG_C(message) ren_utils::LogEmitter::Log(ren_utils::LogLevel::critical, message, __FILE__, __LINE__)

/**
 * @brief Thread-safe wrapper for LogEmitter.
 */
class TsLogEmitter {
public:
  inline static std::mutex s_Mtx{};

  /// Thread-safe version of LogEmitter::Log() method.
  static void Log(LogLevel level, std::string message,
                  std::filesystem::path file, uint32_t line) {
    std::lock_guard<std::mutex> lock(s_Mtx);
    return LogEmitter::Log(level, message, file, line);
  }

  /// Thread-safe version of LogEmitter::AddListener() method.
  template <typename T, typename... Args>
  static T* AddListener(Args... args) {
    std::lock_guard<std::mutex> lock(s_Mtx);
    return LogEmitter::AddListener<T, Args...>(args...);
  }

  /// Thread-safe version of LogEmitter::AddListener() method.
  template <typename T, typename I>
  static T* AddListener(std::initializer_list<I> init_list) {
    std::lock_guard<std::mutex> lock(s_Mtx);
    return LogEmitter::AddListener<T, I>(init_list);
  }

  /// Thread-safe version of LogEmitter::AddListener() method.
  template <typename T>
  static T* AddListener() {
    std::lock_guard<std::mutex> lock(s_Mtx);
    return LogEmitter::AddListener<T>();
  }

  /// Thread-safe version of LogEmitter::GetListener() method.
  template <typename T>
  static T* GetListener() {
    std::lock_guard<std::mutex> lock(s_Mtx);
    return LogEmitter::GetListener<T>();
  }

  /// Thread-safe veresion of LogEmitter::RemoveListener() method.
  template <typename T>
  static void RemoveListener() {
    std::lock_guard<std::mutex> lock(s_Mtx);
    LogEmitter::RemoveListener<T>();
  }
};

/// Thread-safe emit information log.
#define TS_LOG_I(message) ren_utils::TsLogEmitter::Log(ren_utils::LogLevel::info, message, __FILE__, __LINE__)
/// Thread-safe emit statuc log.
#define TS_LOG_S(message) ren_utils::TsLogEmitter::Log(ren_utils::LogLevel::status, message, __FILE__, __LINE__)
/// Thread-safe emit warning log.
#define TS_LOG_W(message) ren_utils::TsLogEmitter::Log(ren_utils::LogLevel::warning, message, __FILE__, __LINE__)
/// Thread-safe emit error log.
#define TS_LOG_E(message) ren_utils::TsLogEmitter::Log(ren_utils::LogLevel::error, message, __FILE__, __LINE__)
/// Thread-safe emit critical error log.
#define TS_LOG_C(message) ren_utils::TsLogEmitter::Log(ren_utils::LogLevel::critical, message, __FILE__, __LINE__)

} // namespace ren_utils

