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

namespace Logger {
    enum class LogLevel : size_t { debug = 0U, info, warn, error, critical, fatal, log };

    using namespace std::string_literals;

    using timePoint_t = std::chrono::high_resolution_clock::time_point;
    using duration_t = std::chrono::milliseconds;

    namespace _ {

        //internal function. Not to be used directly
        LOGGER_EXPORT [[nodiscard]] LogLevel requiredLevel() noexcept;

        //internal function. Not to be used directly
        LOGGER_EXPORT void setLogLevel(LogLevel) noexcept;

        //internal function. Not to be used directly
        LOGGER_EXPORT void printWithoutLabel(std::string_view) noexcept;

        //internal function. Not to be used directly
        LOGGER_EXPORT void print(std::string_view) noexcept;

        LOGGER_EXPORT void lockStream();

        LOGGER_EXPORT void unlockStream() noexcept;

        //internal function. Not to be used directly
        template <typename T>
        [[nodiscard]] std::string getRepNonStreamable(T&& obj) noexcept
        {
            std::ostringstream interpreter;

            interpreter << details::type_name<std::remove_reference_t<T>>() << " at 0x" << std::hex << reinterpret_cast<std::uintptr_t>(&obj);

            return interpreter.str();
        }

        template <typename T>
        [[nodiscard]] std::string getRepStreamable(T&& obj) noexcept
        {
            std::ostringstream interpreter;

            interpreter << obj;

            return interpreter.str();
        }

        template <typename T, typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<T>, char>>>
        [[nodiscard]] std::string getRepStreamable(T* obj) noexcept
        {
            return getRepNonStreamable(&obj);
        }

        //overload getRepStreamable so C-style strings don't get logged as pointers
        template <>
        [[nodiscard]] inline std::string getRepStreamable<const char, void>(const char* obj) noexcept
        {
            return {obj};
        }

        template <>
        [[nodiscard]] inline std::string getRepStreamable<char, void>(char* obj) noexcept
        {
            return getRepStreamable(const_cast<const char*>(obj));
        }

        template<>
        [[nodiscard]] inline std::string getRepStreamable<volatile char, void>(volatile char* obj) noexcept
        {
            return getRepStreamable(const_cast<const char*>(obj));
        }

        //internal function. Not to be used directly
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

        //internal function. Not to be used directly
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

        template <typename T, typename... Args>
        void logConcurrent(LogLevel lv, bool log_with_label, T&& obj, Args&&... args)
        {
            lockStream();
            [[maybe_unused]] volatile const auto unlock_mutex_on_exit = details::Finally([]() { unlockStream(); });

            if (lv < _::requiredLevel() && lv != LogLevel::fatal) { //fatal messages cannot be blocked
                return;
            }

            setLogLevel(lv);
            logObj(log_with_label, std::forward<T>(obj));
            (logObj(false, std::forward<Args>(args)), ...);
        }

        template <typename... Args>
        void log(LogLevel lv, bool log_with_label, Args&&... args)
        {
            _::logConcurrent(lv, log_with_label, std::forward<Args>(args)...);
        }
    } // namespace _

    /// \brief Log a message with the provided level. Can log multiple objects seperated by a comma
    /// \tparam ...Args Types of the objects to be logged
    /// \param lv the level with which to log
    /// \param ...args Objects to be logged
    template <typename... Args>
    void log(LogLevel lv, Args&&... args)
    {
        _::log(lv, true, std::forward<Args>(args)...);
    }

    /// \brief Log a message with the DEBUG level. Can log multiple objects seperated by comma
    /// \tparam ...Args Types of the objects to be logged
    /// \param ...args Objects to be logged
    template <typename... Args>
    void debug(Args&&... args)
    {
        _::log(LogLevel::debug, true, std::forward<Args>(args)...);
    }

    /// \brief Log a message with the INFO level. Can log multiple objects seperated by comma
    /// \tparam ...Args Types of the objects to be logged
    /// \param ...args Objects to be logged
    template <typename... Args>
    void info(Args&&... args)
    {
        _::log(LogLevel::info, true, std::forward<Args>(args)...);
    }

    /// \brief Log a message with the WARN level. Can log multiple objects seperated by comma
    /// \tparam ...Args Types of the objects to be logged
    /// \param ...args Objects to be logged
    template <typename... Args>
    void warn(Args&&... args)
    {
        _::log(LogLevel::warn, true, std::forward<Args>(args)...);
    }

    /// \brief Log a message with the ERROR level. Can log multiple objects seperated by comma
    /// \tparam ...Args Types of the objects to be logged
    /// \param ...args Objects to be logged
    template <typename... Args>
    void error(Args&&... args)
    {
        _::log(LogLevel::error, true, std::forward<Args>(args)...);
    }

    /// \brief Log a message with the ERROR level. Can log multiple objects seperated by comma
    /// \tparam ...Args Types of the objects to be logged
    /// \param ...args Objects to be logged
    template <typename... Args>
    void critical(Args&&... args)
    {
        _::log(LogLevel::critical, true, std::forward<Args>(args)...);
    }

    /// \brief Log a message with the FATAL level. Can log multiple objects seperated by comma
    /// \tparam ...Args Types of the objects to be logged
    /// \param ...args Objects to be logged
    template <typename... Args>
    void fatal(Args&&... args)
    {
        _::log(LogLevel::fatal, true, std::forward<Args>(args)...);
    }

    /// \brief Log a message regardless of the minimum required log level. Can log multiple objects seperated by a comma.
    /// \tparam ...Args Types of the objects to be logged
    /// \param ...args Objects to be logged
    template <typename... Args>
    void log(Args&&... args)
    {
        _::log(LogLevel::log, false, std::forward<Args>(args)...);
    }

    //set the label that is displayed in front of the message [LABEL] msg
    LOGGER_EXPORT void setLogLabel(LogLevel, std::string_view) noexcept;

    //set the color that should be used for the level. Must be an ANSI compatible color code
    LOGGER_EXPORT void setLogColor(LogLevel, std::string_view) noexcept;

    //set an alternative stream for the logger to write to
    LOGGER_EXPORT void setOutStream(std::ostream& os);

    //disable colored output
    LOGGER_EXPORT void disableColor() noexcept;

    //enable colored output
    LOGGER_EXPORT void enableColor() noexcept;

    //start a timer and associate it with the label
    LOGGER_EXPORT std::string startTimer(std::string_view label) noexcept;

    //return the duration since the last call to startTimer(label). removes label from the list of active labels
    LOGGER_EXPORT [[nodiscard]] duration_t endTimer(const std::string& label) noexcept;

    //return the duration since the last call to startTimer(label). does NOT remove label from the list of acitve labels
    LOGGER_EXPORT [[nodiscard]] duration_t getTimer(const std::string& label) noexcept;

    //log the duration since the last call to startTimer(label). removes label from the list of active labels
    LOGGER_EXPORT void
    logDuration(const std::string& label, std::string_view prefix = ""s, std::string_view suffix = "ms\n"s) noexcept;

    //log the duration since the last call to startTimer(label). does NOT remove label from the list of acitve labels
    LOGGER_EXPORT void
    logDurationPersistent(const std::string& label, std::string_view prefix = ""s, std::string_view suffix = "ms\n"s) noexcept;
    //set minimum level the Message has to be sent with in order to show up. Default: INFO
    LOGGER_EXPORT LogLevel setMinimumLogLevel(LogLevel) noexcept;

    /// \brief Configure the Logger to log into a file instead of a std::ostream
    /// \param directory Directory of the logfile. must end with "/"
    /// \param fileName Name of the logfile. Does not need an extension
    LOGGER_EXPORT void initLogFile(std::string_view directory, std::string_view fileName = "Log.log");

    /**
	*\brief Close the current log file, if existent
	*/
    LOGGER_EXPORT void dumpLogFile() noexcept;
} // namespace Logger

#endif /* LOGGER_H_ */
