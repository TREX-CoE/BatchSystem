#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

namespace cw {
namespace batch {
namespace internal {

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

}	
}
}
