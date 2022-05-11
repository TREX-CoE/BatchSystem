#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

namespace cw {
namespace batch {
namespace internal {

template <typename Iterator>
static inline std::string joinString(Iterator begin, Iterator end, const std::string& del) {
    return std::accumulate(begin, end, std::string(), [&](const std::string &s1, const std::string &s2){
        // join string with , separator
        return s1.empty() ? s2 : (s1 + del + s2);
    });
}

}	
}
}
