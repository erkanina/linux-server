//------------------------------------
//C++11 uyumlu

#pragma once

#include <iostream>
#include <mutex>
#include <sstream>

class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    template<typename... Args>
    void log(Args&&... args) {
        std::ostringstream oss;
        buildString(oss, std::forward<Args>(args)...);
        oss << "\n";

        std::lock_guard<std::mutex> lock(mtx_);
        std::cout << oss.str() << std::flush;
    }

private:
    // Base case — tek argüman
    template<typename T>
    void buildString(std::ostringstream& oss, T&& arg) {
        oss << std::forward<T>(arg);
    }

    // Recursive case — birden fazla argüman
    template<typename T, typename... Args>
    void buildString(std::ostringstream& oss, T&& first, Args&&... rest) {
        oss << std::forward<T>(first);
        buildString(oss, std::forward<Args>(rest)...);
    }

    Logger() {
        std::cout.setf(std::ios::unitbuf);
    }

    std::mutex mtx_;
};

#define LOG(...) Logger::instance().log(__VA_ARGS__)

/*

//------------------------------------
// C++17 hatası veriyor

#pragma once


#include <iostream>
#include <mutex>
#include <sstream>

// ─────────────────────────────────────────────
//  Thread-safe logger
//  Kullanım: LOG() << "mesaj: " << fd;
// ─────────────────────────────────────────────
class Logger {
public:
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    template<typename... Args>
    void log(Args&&... args) {
        std::ostringstream oss;
        (oss << ... << std::forward<Args>(args));
        oss << "\n";

        std::lock_guard<std::mutex> lock(mtx_);
        std::cout << oss.str() << std::flush;
    }

private:
    Logger() {
        // Stdout'u unbuffered yap
        std::cout.setf(std::ios::unitbuf);
    }
    std::mutex mtx_;
};

// Kısa kullanım makrosu
#define LOG(...) Logger::instance().log(__VA_ARGS__)*/
