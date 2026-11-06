#pragma once
#include <string>
#include <mutex>

enum class Lv { Dbg = 0, Inf, Wrn, Err };

class Log {
public:
    static Log& get();
    void setFile(const std::string& p);
    void setVerbose(bool v);
    void d(const std::string& m);
    void i(const std::string& m);
    void w(const std::string& m);
    void e(const std::string& m);
private:
    void w_(Lv l, const std::string& m);
    std::mutex m_mx;
    bool m_verbose = false;
    std::string m_file;
};