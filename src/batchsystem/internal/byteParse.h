#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

namespace cw {
namespace batch {
namespace internal {

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
