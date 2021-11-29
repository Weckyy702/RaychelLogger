/**
*\file Logger.cpp
*\author weckyy702 (weckyy702@gmail.com)
*\brief RaychelLogger implementation file
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

#include "RaychelLogger/Logger.h"

#if __has_include(<filesystem>)
    #include <filesystem>
namespace fs = std::filesystem;
#else
    #include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif
#include <fstream>
#include <iostream>
#include <mutex>
#include <unordered_map>

namespace Logger {

    static LogLevel currentLevel = LogLevel::info;
    static LogLevel minLogLevel = LogLevel::info;

    static bool doColor = true;
    constexpr std::string_view reset_col = "\x1b[0m";

    static std::ostream outStream{std::cout.rdbuf()};
    static std::ofstream logFile;

    static std::recursive_mutex mtx;
    static bool mtx_engaged;

    static std::unordered_map<std::string, timePoint_t> timePoints;

    static std::array<std::string_view, 7> levelLabels = {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL", "FATAL", "OUT"};

    static std::array<std::string_view, 7> cols = {
        "\x1b[36m",     //DEBUG, light blue
        "\x1b[32m",     //INFO, green
        "\x1b[33m",     //WARNING, yellow
        "\x1b[31m",     //ERROR, red
        "\x1b[1;31m",   //CRITICAL, bold red
        "\x1b[4;1;31m", //FATAL bold underlined red
        "\x1b[34m"      //LOG, blue
    };

    std::string_view getLogLabel()
    {
        return levelLabels.at(static_cast<size_t>(currentLevel));
    }

    std::string_view getLogColor()
    {
        return doColor ? cols.at(static_cast<size_t>(currentLevel)) : "";
    }

    namespace details {

        LogLevel requiredLevel() noexcept
        {
            return minLogLevel;
        }

        void setLogLevel(LogLevel level) noexcept
        {
            currentLevel = level;
        }

//TODO: why does MSVC accept this?
//We disable -Wsign-conversion here because std::string_view::size() returns an unsigned std::size_t
//but std::ostream::write() takes a std::streamsize which is signed. We cannot do anything about that :(
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"

        void printWithoutLabel(std::string_view msg) noexcept
        {
            if (doColor) {
                const auto col = getLogColor();
                outStream.write(col.data(), col.size()).write(msg.data(), msg.size()).write(reset_col.data(), reset_col.size());
            } else {
                outStream.write(msg.data(), msg.size());
            }
        }

        void print(std::string_view msg) noexcept
        {
            if (doColor) {
                const auto col = getLogColor();
                const auto label = getLogLabel();

                outStream.write(col.data(), col.size())
                    .write("[", 1)
                    .write(label.data(), label.size())
                    .write("] ", 2)
                    .write(reset_col.data(), reset_col.size());
            } else {
                const auto label = getLogLabel();

                outStream.write("[", 1).write(label.data(), label.size()).write("] ", 2);
            }
            printWithoutLabel(msg);
        }

#pragma GCC diagnostic pop

        void lockStream()
        {
            mtx.lock();
            mtx_engaged = true;
        }

        void unlockStream() noexcept
        {
            if (mtx_engaged) {
                mtx_engaged = false;
                mtx.unlock();
            }
        }

    } // namespace details

    void setLogLabel(LogLevel lv, std::string_view label) noexcept
    {
        RAYCHELLOGGER_LOCK_STREAM();

        levelLabels.at(static_cast<size_t>(lv)) = label;
    }

    void setLogColor(LogLevel lv, std::string_view color) noexcept
    {
        RAYCHELLOGGER_LOCK_STREAM();

        cols.at(static_cast<size_t>(lv)) = color;
    }

    void setOutStream(std::ostream& os)
    {
        RAYCHELLOGGER_LOCK_STREAM();
        outStream.rdbuf(os.rdbuf());
    }

    void disableColor() noexcept
    {
        doColor = false;
    }

    void enableColor() noexcept
    {
        doColor = true;
    }

    std::string startTimer(std::string_view label) noexcept
    {
        RAYCHELLOGGER_LOCK_STREAM();
        const auto tp = std::chrono::high_resolution_clock::now();

        std::string str{label};
        timePoints.insert_or_assign(str, tp);
        return str;
    }

    std::chrono::nanoseconds details::endTimer(const std::string& label) noexcept
    {
        using namespace std::chrono;

        RAYCHELLOGGER_LOCK_STREAM();

        const auto endPoint = high_resolution_clock::now();
        if (timePoints.find(label) != timePoints.end()) {
            const auto startTime = timePoints.extract(label).mapped();
            return duration_cast<std::chrono::nanoseconds>(endPoint - startTime);
        }

        error("Label ", label, " not found!\n");
        return std::chrono::nanoseconds{-1};
    }

    std::chrono::nanoseconds details::getTimer(const std::string& label) noexcept
    {
        using namespace std::chrono;

        RAYCHELLOGGER_LOCK_STREAM();

        const auto endPoint = high_resolution_clock::now();
        if (timePoints.find(label) != timePoints.end()) {
            const auto startTime = timePoints.at(label);
            return duration_cast<std::chrono::nanoseconds>(endPoint - startTime);
        }
        error("Label ", label, " not found!\n");
        return std::chrono::nanoseconds{-1};
    }

    LogLevel setMinimumLogLevel(LogLevel lv) noexcept
    {
        minLogLevel = lv;
        return minLogLevel;
    }

    void initLogFile(std::string_view directory, std::string_view filename)
    {
        const fs::path dir{directory};
        if (!directory.empty()) {
            std::error_code ec;
            fs::create_directories(dir, ec);
            if (ec) {
                error("failed to open log file '", directory, "/", filename, "': ", ec.message());
                return;
            }
        }

        if (logFile.is_open()) {
            dumpLogFile();
        }

        const auto filepath = dir / filename;

        logFile = std::ofstream{filepath};

        if (logFile.is_open()) {
            outStream.rdbuf(logFile.rdbuf());
            disableColor();
        }
    }

    void dumpLogFile() noexcept
    {
        if (logFile.is_open()) {
            if (outStream.rdbuf() == logFile.rdbuf()) {
                outStream.rdbuf(std::cout.rdbuf());
            }
            logFile.close();
        }
    }
} // namespace Logger
