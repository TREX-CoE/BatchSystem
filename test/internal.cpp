#include <catch2/catch_test_macros.hpp>

#include "batchsystem/internal/byteParse.h"

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
