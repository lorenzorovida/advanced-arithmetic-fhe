#pragma once

#include <iostream>
#include <chrono>
#include <string>

class Logger {
    int current_level;

    // ANSI color codes
    static constexpr const char* RESET   = "\033[0m";
    static constexpr const char* CYAN    = "\033[36m";
    static constexpr const char* GREEN   = "\033[32m";
    static constexpr const char* YELLOW  = "\033[33m";
    static constexpr const char* RED     = "\033[31m";
    static constexpr const char* MAGENTA = "\033[35m";
    static constexpr const char* WHITE   = "\033[37m";

public:
    explicit Logger(int v) : current_level(v) {}

    class LogStream {
        bool enabled;

    public:
        LogStream(bool e, const char* color)
                : enabled(e)
        {
#ifndef LOGGER_DISABLE
            if (enabled)
                std::cout << color;
#endif
        }

        ~LogStream() {
#ifndef LOGGER_DISABLE
            if (enabled)
                std::cout << RESET;
#endif
        }

        template<typename T>
        LogStream& operator<<(const T& value) {
#ifndef LOGGER_DISABLE
            if (enabled) std::cout << value;
#endif
            return *this;
        }

        LogStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
#ifndef LOGGER_DISABLE
            if (enabled) manip(std::cout);
#endif
            return *this;
        }
    };

    // ---------- Semantic Logging ----------

    LogStream operator()(int lvl) {
        return LogStream(current_level >= lvl, RESET);   // default behavior = info
    }

    LogStream info(int lvl) {
        return LogStream(current_level >= lvl, CYAN);
    }

    LogStream success(int lvl) {
        return LogStream(current_level >= lvl, GREEN);
    }

    LogStream warn(int lvl) {
        return LogStream(current_level >= lvl, YELLOW);
    }

    LogStream error(int lvl) {
        return LogStream(current_level >= lvl, RED);
    }

    LogStream debug(int lvl) {
        return LogStream(current_level >= lvl, MAGENTA);
    }

    LogStream plain(int lvl) {
        return LogStream(current_level >= lvl, WHITE);
    }

    LogStream green_bold(int lvl) {
        return LogStream(current_level >= lvl, "\033[1;32m"); // bold green
    }

    LogStream yellow_bold(int lvl) {
        return LogStream(current_level >= lvl, "\033[1;33m"); // bold yellow
    }

    LogStream red_bold(int lvl) {
        return LogStream(current_level >= lvl, "\033[1;31m"); // bold red
    }

    LogStream cyan_bold(int lvl) {
        return LogStream(current_level >= lvl, "\033[1;36m"); // bold cyan
    }
};