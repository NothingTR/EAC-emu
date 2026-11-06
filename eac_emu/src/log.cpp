#include "../include/log.h"
#include <cstdio>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <windows.h>

Log& Log::get() { static Log l; return l; }
void Log::setFile(const std::string& p) { m_file = p; }
void Log::setVerbose(bool v) { m_verbose = v; }
void Log::d(const std::string& m) { w_(Lv::Dbg, m); }
void Log::i(const std::string& m) { w_(Lv::Inf, m); }
void Log::w(const std::string& m) { w_(Lv::Wrn, m); }
void Log::e(const std::string& m) { w_(Lv::Err, m); }

static const char* tagOf(Lv l) {
    switch (l) {
        case Lv::Dbg: return "DBG";
        case Lv::Inf: return "INF";
        case Lv::Wrn: return "WRN";
        default:      return "ERR";
    }
}

void Log::w_(Lv l, const std::string& m) {
    if (l == Lv::Dbg && !m_verbose) return;

    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()).count() % 1000;
    std::tm bt{};
    localtime_s(&bt, &t);

    std::ostringstream ss;
    ss << std::put_time(&bt, "%H:%M:%S") << '.'
       << std::setfill('0') << std::setw(3) << ms
       << " [" << tagOf(l) << "] " << m << '\n';

    std::lock_guard<std::mutex> g(m_mx);
    std::fputs(ss.str().c_str(), stdout);
    if (!m_file.empty()) {
        FILE* f = nullptr;
        if (fopen_s(&f, m_file.c_str(), "a") == 0 && f) {
            std::fputs(ss.str().c_str(), f);
            std::fclose(f);
        }
    }
}