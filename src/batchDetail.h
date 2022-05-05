#ifndef __CW_BATCH_UTIL_H__
#define __CW_BATCH_UTIL_H__

#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

namespace cw {
namespace batch {
namespace detail {

// delta function for trim
static inline bool trim_delta(unsigned char ch){
        return std::isprint(ch) && !std::isblank(ch);
}
// trim from start (in place)
static inline void ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), trim_delta));
}
// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), trim_delta).base(), s.end());
}

// trim from start (copying)
static inline std::string trim_copy(std::string s) {
    ltrim(s);
    rtrim(s);
    return s;
}

static inline void toLowerCase(std::string& str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
}

template <typename F>
static inline void splitString(const std::string& str, const std::string& delimiter, F cb) {
	size_t delim_len = delimiter.length();
    size_t start = 0;
	size_t end;

    while ((end = str.find(delimiter, start)) != std::string::npos) {
        if (!cb(start, end-start)) return;
        start = end + delim_len;
    }
	cb(start, end);
} 

static inline std::stringstream runCommand(std::function<cmd_execute_f> _func, const std::string& cmd, const std::vector<std::string>& args) {
	std::stringstream commandResult;
	int ret = _func(commandResult, cmd, args);
	if (ret != 0) throw CommandFailed("Command failed", cmd, ret);
	return commandResult;
}

template<typename T>
static inline bool stream_cast(const std::string& s, T& out)
{
    std::istringstream ss(s);
    if ((ss >> out).fail() || !(ss >> std::ws).eof()) return false;
    return true;
}

static inline bool timeToEpoch(const std::string& isoDatetime, std::time_t& epoch, const char* format) {
	std::istringstream ss(isoDatetime);
	std::tm t{};
	ss >> std::get_time(&t, format);
	if (ss.fail()) return false;
	epoch = mktime(&t);
	return true;
}

static inline bool si_byte_parse(const std::string& s, uint64_t& out, std::istringstream& ss) {
    uint64_t val = 0;
    ss.str(s);
    if ((ss >> val).fail()) return false;
    std::string unit;
    ss >> unit;
    std::transform(unit.begin(), unit.end(), unit.begin(), [](unsigned char c){ return std::tolower(c); });
    if (unit == "" || unit == "b") {
        out = val;
        return true;
    } else if (unit == "k" || unit == "kb") {
        out = val * 1000;
        return true;
    } else if (unit == "m" || unit == "mb") {
        out = val * 1000000;
        return true;
    } else if (unit == "g" || unit == "gb") {
        out = val * 1000000000;
        return true;
    } else if (unit == "t" || unit == "tb") {
        out = val * 1000000000000;
        return true;
    } else if (unit == "p" || unit == "pb") {
        out = val * 1000000000000000;
        return true;
    }
    return false;
}
static inline bool si_byte_parse(const std::string& s, uint64_t& out) {
    std::istringstream ss;
    return si_byte_parse(s, out, ss);
}

}	
}
}

#endif /* __CW_BATCH_UTIL_H__ */
