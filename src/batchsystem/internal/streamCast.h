#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

namespace cw {
namespace batch {
namespace internal {

template<typename T>
static inline bool stream_cast(const std::string& s, T& out)
{
    std::istringstream ss(s);
    if ((ss >> out).fail() || !(ss >> std::ws).eof()) return false;
    return true;
}

}	
}
}
