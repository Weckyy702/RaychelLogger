/**
*\file Logger.h
*\author weckyy702 (weckyy702@gmail.com)
*\brief RaychelLogger main include file
*\date 2021-06-13
*
*MIT License
*Copyright (c) [2021] [Weckyy702 (weckyy702@gmail.com | https://github.com/Weckyy702)]
*Permission is hereby granted, free of charge, to any person obtaining a copy
*of this software and associated documentation files (the "Software"), to deal
*in the Software without restriction, including without limitation the rights
*to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*copies of the Software, and to permit persons to whom the Software is
*furnished to do so, subject to the following conditions:
*
*The above copyright notice and this permission notice shall be included in all
*copies or substantial portions of the Software.
*
*THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*SOFTWARE.
*
*/
#ifndef LOGGER_H_
#define LOGGER_H_

#include <cstdint>
#if !((defined(__cplusplus) && (__cplusplus >= 201703L)) || defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
    #error "C++17 compilation is required!"
#endif

#include "Helper.h"

#include <array>
#include <chrono>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

#define RAYCHELLOGGER_LOCK_STREAM()                                                                                              \
    ::Logger::details::lockStream();                                                                                             \
    [[maybe_unused]] volatile const auto unlock_mutex_on_exit = details::Finally([]() { ::Logger::details::unlockStream(); });

namespace Logger {
    enum class LogLevel : size_t { debug, info, warn, error, critical, fatal, log };

    using namespace std::string_view_literals;

    using timePoint_t = std::chrono::high_resolution_clock::time_point;

    namespace details {

        /**
        * \brief Get the required level for a message to be logged
        * 
        * \return Current minimum log level
        */
        [[nodiscard]] LOGGER_EXPORT LogLevel requiredLevel() noexcept;

        /**
        * \brief Set the minimum required log level
        */
        LOGGER_EXPORT void setLogLevel(LogLevel) noexcept;

        /**
        * \brief Part of the type-ignorant printing API
        */
        LOGGER_EXPORT void printWithoutLabel(std::string_view) noexcept;

        /**
        * \brief Part of the type-ignorant printing API
        */
        LOGGER_EXPORT void print(std::string_view) noexcept;

        /**
        * \brief Lock the output stream so logging is thread-safe
        */
        LOGGER_EXPORT void lockStream();

        /**
        * \brief Unlock the output stream so logging is thread-safe
        */
        LOGGER_EXPORT void unlockStream() noexcept;

        /**
        * \brief Get the {type_name} at {pointer} representation of an object that cannot be converted using streams
        * 
        * \tparam T Type of the object
        * \param obj Object to be converted
        * \return std::string Representation of obj
        */
        template <typename T>
        [[nodiscard]] std::string getRepNonStreamable(T&& obj) noexcept
        {
            std::ostringstream interpreter;

            interpreter << details::type_name<std::remove_reference_t<T>>() << " at 0x" << std::hex
                        << reinterpret_cast<std::uintptr_t>(&obj); //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

            return interpreter.str();
        }

        /**
        * \brief Get the string representation of an object that can be converted using streams
        * 
        * \tparam T Type of the object
        * \param obj Object to convert
        * \return std::string String representation of the object
        */
        template <typename T>
        [[nodiscard]] std::string getRepStreamable(T&& obj) noexcept
        {
            std::ostringstream interpreter;

            interpreter << obj;

            return interpreter.str();
        }

        /**
        * \brief Get the string representation of a pointer
        * 
        * \tparam T Type of the pointer
        * \param ptr Pointer to convert
        * \return std::string String representation of the object
        */
        template <typename T, typename = std::enable_if_t<!std::is_same_v<std::remove_const_t<T>, char>>>
        [[nodiscard]] std::string getRepStreamable(T* ptr) noexcept
        {
            return getRepNonStreamable(ptr);
        }

        /**
        * \brief Get the string representation of a C-style string
        * 
        * \param ptr A C-style string
        * \return std::string That same string, but better
        */
        template <>
        [[nodiscard]] inline std::string getRepStreamable<const char, void>(const char* ptr) noexcept
        {
            return {ptr};
        }

        /**
        * \brief Get the string representation of a C-style string
        * 
        * \param ptr A C-style string
        * \return std::string That same string, but better
        */
        template <>
        [[nodiscard]] inline std::string getRepStreamable<char, void>(char* ptr) noexcept
        {
            return {ptr};
        }

        /**
        * \brief Print an object that can be converted using streams
        * 
        * \tparam T Type of the object
        * \param log_with_label If the object should be logged with [LABEL] in front of it
        * \param obj Object to log
        */
        template <typename T, std::enable_if_t<details::is_to_stream_writable_v<std::ostringstream, T>, bool> = true>
        void logObj(bool log_with_label, T&& obj) noexcept
        {
            std::string representation = getRepStreamable(std::forward<T>(obj));

            if (log_with_label) {
                print(representation);
            } else {
                printWithoutLabel(representation);
            }
        }

        /**
        * \brief Print an object that cannot be converted using streams
        * 
        * \tparam T Type of the object
        * \param log_with_label If the object should be logged with [LABEL] in front of it
        * \param obj Object to log
        */
        template <typename T, std::enable_if_t<!details::is_to_stream_writable_v<std::ostringstream, T>, bool> = false>
        void logObj(bool log_with_label, T&& obj) noexcept
        {
            std::string representation = getRepNonStreamable(std::forward<T>(obj));

            if (log_with_label) {
                print(representation);
            } else {
                printWithoutLabel(representation);
            }
        }

        /**
        * \brief Log args in a thread safe way
        * 
        * \tparam T Type of the first object
        * \tparam Args Type of the other objects
        * \param level Level used for checking if the objects should be logged at all and responsible for the label
        * \param log_with_label If the first object should be logged with [LABEL] in front of it
        * \param obj First object to log
        * \param args Rest of the objects to log
        */
        template <typename T, typename... Args>
        void logConcurrent(LogLevel level, bool log_with_label, T&& obj, Args&&... args)
        {
            RAYCHELLOGGER_LOCK_STREAM();

            if (level < details::requiredLevel() && level != LogLevel::fatal) { //fatal messages cannot be blocked
                return;
            }

            setLogLevel(level);
            logObj(log_with_label, std::forward<T>(obj));
            (logObj(false, std::forward<Args>(args)), ...);
        }

        /**
        * \brief End the timer associated with label
        * 
        * \param label Label of the timer
        */
         [[nodiscard]] LOGGER_EXPORT std::chrono::nanoseconds endTimer(const std::string& label) noexcept;

        /**
        * \brief Get the Timer associated with label
        * 
        * \param label Label of the timer
        * \return LOGGER_EXPORT 
        */
       [[nodiscard]] LOGGER_EXPORT std::chrono::nanoseconds getTimer(const std::string& label) noexcept;
    } // namespace details

    /// \brief Log a message with the provided level. Can log multiple objects seperated by a comma
    /// \tparam ...Args Types of the objects to be logged
    /// \param lv the level with which to log
    /// \param ...args Objects to be logged
    template <typename... Args>
    void log(LogLevel level, Args&&... args)
    {
        details::logConcurrent(level, true, std::forward<Args>(args)...);
    }

    /// \brief Log a message with the DEBUG level. Can log multiple objects seperated by comma
    /// \tparam ...Args Types of the objects to be logged
    /// \param ...args Objects to be logged
    template <typename... Args>
    void debug(Args&&... args)
    {
        details::logConcurrent(LogLevel::debug, true, std::forward<Args>(args)...);
    }

    /// \brief Log a message with the INFO level. Can log multiple objects seperated by comma
    /// \tparam ...Args Types of the objects to be logged
    /// \param ...args Objects to be logged
    template <typename... Args>
    void info(Args&&... args)
    {
        details::logConcurrent(LogLevel::info, true, std::forward<Args>(args)...);
    }

    /// \brief Log a message with the WARN level. Can log multiple objects seperated by comma
    /// \tparam ...Args Types of the objects to be logged
    /// \param ...args Objects to be logged
    template <typename... Args>
    void warn(Args&&... args)
    {
        details::logConcurrent(LogLevel::warn, true, std::forward<Args>(args)...);
    }

    /// \brief Log a message with the ERROR level. Can log multiple objects seperated by comma
    /// \tparam ...Args Types of the objects to be logged
    /// \param ...args Objects to be logged
    template <typename... Args>
    void error(Args&&... args)
    {
        details::logConcurrent(LogLevel::error, true, std::forward<Args>(args)...);
    }

    /// \brief Log a message with the ERROR level. Can log multiple objects seperated by comma
    /// \tparam ...Args Types of the objects to be logged
    /// \param ...args Objects to be logged
    template <typename... Args>
    void critical(Args&&... args)
    {
        details::logConcurrent(LogLevel::critical, true, std::forward<Args>(args)...);
    }

    /// \brief Log a message with the FATAL level. Can log multiple objects seperated by comma
    /// \tparam ...Args Types of the objects to be logged
    /// \param ...args Objects to be logged
    template <typename... Args>
    void fatal(Args&&... args)
    {
        details::logConcurrent(LogLevel::fatal, true, std::forward<Args>(args)...);
    }

    /// \brief Log a message regardless of the minimum required log level. Can log multiple objects seperated by a comma.
    /// \tparam ...Args Types of the objects to be logged
    /// \param ...args Objects to be logged
    template <typename... Args>
    void log(Args&&... args)
    {
        details::logConcurrent(LogLevel::log, false, std::forward<Args>(args)...);
    }

    /**
    * \brief Set the label for a specific log level
    * 
    * \param level Level for the label
    * \param label Label for the level
    */
    LOGGER_EXPORT void setLogLabel(LogLevel level, std::string_view label) noexcept;

    /**
    * \brief Set the color escape sequence for a specific log level
    * 
    * \param level Level for the color seqeuence
    * \param color_escape_sequence color sequence for the level
    */
    LOGGER_EXPORT void setLogColor(LogLevel level, std::string_view color_escape_sequence) noexcept;

    /**
    * \brief Set the output stream for all logging operations
    * 
    * \param new_out_stream new output stream
    */
    LOGGER_EXPORT void setOutStream(std::ostream& new_out_stream);

    /**
    * \brief Disable colored output
    */
    LOGGER_EXPORT void disableColor() noexcept;

    /**
    * \brief Enable colored output
    */
    LOGGER_EXPORT void enableColor() noexcept;

    /**
    * \brief Start a new timer and associate it with the label. If the label already exists, the timer will be overriden
    * 
    * \param label label for the new timer
    * \return std::string the new label
    */
    LOGGER_EXPORT std::string startTimer(std::string_view label) noexcept;

    /**
    * \brief End the timer associated with %label. Removes that timer from the list of active timers
    * 
    * \tparam T Type of duration returned. Internal precision is at most std::nanoseconds (note that these functions are not meant for such precise timing)
    * \param label Label for the timer
    * \return T Duration since the timer started. Truncated to the precision of T
    */
    template <typename T = std::chrono::milliseconds>
    [[nodiscard]] T endTimer(const std::string& label) noexcept
    {
        const auto dur = details::endTimer(label);

        return std::chrono::duration_cast<T>(dur);
    }

    /**
    * \brief Get the value of the timer associated with %label. Does NOT remove that timer from the list of active timers
    * 
    * \tparam T Type of duration returned. Internal precision is at most std::nanoseconds (note that these functions are not intended for precise timing)
    * \param label Label for the timer
    * \return T Duration since the timer started. Truncated to the precision of T
    */
    template <typename T = std::chrono::milliseconds>
    [[nodiscard]] T getTimer(const std::string& label) noexcept
    {
        const auto dur = details::getTimer(label);

        return std::chrono::duration_cast<T>(dur);
    }

    /**
    * \brief End the timer associated with %label and log its duration
    * 
    * \tparam T Type of duration used. Internal presicions is at most std::nanoseconds (note that these functions are not intendend for precise timing)
    * \param level Level used for logging the duration
    * \param label Label for the timer
    * \param prefix Optional prefix that is logged before the duration. If the prefix is empty, the format will be [LEVEL] {label}: {duration} {suffix} '\n'
    * \param suffix Optional suffix that is logged after the duration. The default is chosen based on the duration Type (nanoseconds => "ns" microseconds => "us" etc.)
    */
    template <typename T>
    void logDuration(
        LogLevel level, const std::string& label, std::string_view prefix = ""sv,
        std::string_view suffix = details::suffix_for<T>::value) noexcept
    {
        const auto dur = endTimer<T>(label).count();

        if (dur == -1) {
            return;
        }

        if (prefix.empty()) {
            log(level, label, ": ", dur, suffix, '\n');
        } else {
            log(level, prefix, dur, suffix, '\n');
        }
    }

    /**
    * \brief End the timer associated with %label and log its duration. This function is for API stability
    * 
    * \tparam T Type of duration used. Internal presicions is at most std::nanoseconds (note that these functions are not intendend for precise timing)
    * \param label Label for the timer
    * \param prefix Optional prefix that is logged before the duration. If the prefix is empty, the format will be [LEVEL] {label}: {duration} {suffix} '\n'
    * \param suffix Optional suffix that is logged after the duration. The default is chosen based on the duration Type (nanoseconds => "ns" microseconds => "us" etc.)
    */
    template <typename T = std::chrono::milliseconds>
    void logDuration(
        const std::string& label, std::string_view prefix = ""sv,
        std::string_view suffix = details::suffix_for<T>::value) noexcept
    {
        logDuration<T>(LogLevel::log, label, prefix, suffix);
    }

    /**
    * \brief Get duration of the timer associated with %label and log it. Does NOT remove that timer from the list of active timers
    * 
    * \tparam T Type of duration used. Internal presicions is at most std::nanoseconds (note that these functions are not intendend for precise timing)
    * \param level Level used for logging the duration
    * \param label Label for the timer
    * \param prefix Optional prefix that is logged before the duration. If the prefix is empty, the format will be [LEVEL] {label}: {duration} {suffix} '\n'
    * \param suffix Optional suffix that is logged after the duration. The default is chosen based on the duration Type (nanoseconds => "ns" microseconds => "us" etc.)
    */
    template <typename T = std::chrono::milliseconds>
    void logDurationPersistent(
        LogLevel level, const std::string& label, std::string_view prefix = ""sv,
        std::string_view suffix = details::suffix_for<T>::value) noexcept
    {
        const auto dur = getTimer<T>(label).count();

        if (dur == -1) {
            return;
        }

        if (prefix.empty()) {
            log(level, label, ": ", dur, suffix, '\n');
        } else {
            log(level, prefix, dur, suffix, '\n');
        }
    }

    /**
    * \brief Get duration of the timer associated with %label and log it. Does NOT remove that timer from the list of active timers. This function is for API stability
    * 
    * \tparam T Type of duration used. Internal presicions is at most std::nanoseconds (note that these functions are not intendend for precise timing)
    * \param label Label for the timer
    * \param prefix Optional prefix that is logged before the duration. If the prefix is empty, the format will be [LEVEL] {label}: {duration} {suffix} '\n'
    * \param suffix Optional suffix that is logged after the duration. The default is chosen based on the duration Type (nanoseconds => "ns" microseconds => "us" etc.)
    */
    template <typename T = std::chrono::milliseconds>
    void logDurationPersistent(
        const std::string& label, std::string_view prefix = ""sv,
        std::string_view suffix = details::suffix_for<T>::value) noexcept
    {
        logDurationPersistent<T>(LogLevel::log, label, prefix, suffix);
    }

    /**
    * \brief Set the Minimum Log Level needed for the message to actually be printed. INFO by default
    * 
    * \return LogLevel The new minimum log level
    */
    LOGGER_EXPORT LogLevel setMinimumLogLevel(LogLevel) noexcept;

    /**
    * \brief Initialize a new log file.
    * 
    * \param directory Name of the directory for the log file
    * \param fileName Name of the log file. "Log.log" by default
    * \return LOGGER_EXPORT 
    */
    LOGGER_EXPORT void initLogFile(std::string_view directory, std::string_view fileName = "Log.log");

    /**
    * \brief If existent, save the current log file to disk and close it.
    */
    LOGGER_EXPORT void dumpLogFile() noexcept;
} // namespace Logger

#endif /* LOGGER_H_ */
