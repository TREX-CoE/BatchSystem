#include <catch2/catch_test_macros.hpp>

#include "batchsystem/internal/byteParse.h"
#include "batchsystem/internal/joinString.h"

TEST_CASE("si_byte_parse", "[internal]") {
    using namespace cw::batch::internal;
    uint64_t bytes = 0;
    REQUIRE(si_byte_parse("1000kb", bytes));
    REQUIRE(bytes==1000000);
    REQUIRE(si_byte_parse("1000", bytes));
    REQUIRE(bytes==1000);
    REQUIRE(si_byte_parse("1000mb", bytes));
    REQUIRE(bytes==1000000000);
    REQUIRE(!si_byte_parse("1000mib", bytes));
}

TEST_CASE("joinString", "[internal]") {
    using namespace cw::batch::internal;

    std::vector<std::string> a{"1", "2", "3"};
    std::vector<std::string> b;
    std::vector<std::string> c{"1"};
    CHECK("1,2,3" == joinString(a.begin(), a.end(), ","));
    CHECK("" == joinString(b.begin(), b.end(), ","));
    CHECK("1" == joinString(c.begin(), c.end(), ","));
}

