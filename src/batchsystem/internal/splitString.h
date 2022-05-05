#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

namespace cw {
namespace batch {
namespace internal {

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

}	
}
}
