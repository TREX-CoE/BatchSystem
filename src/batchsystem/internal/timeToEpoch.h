#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>

namespace cw {
namespace batch {
namespace internal {

static inline bool timeToEpoch(const std::string& isoDatetime, std::time_t& epoch, const char* format) {
	std::istringstream ss(isoDatetime);
	std::tm t{};
	ss >> std::get_time(&t, format);
	if (ss.fail()) return false;
	epoch = mktime(&t);
	return true;
}

}	
}
}
