#include "gmock/gmock.h"
#include "../include/shared/parser_library.h"
#include "context_test.h"

using namespace hlasm_plugin;
using namespace parser_library;

TEST(Hello, World)
{
	EXPECT_EQ(1, 1);
}


int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}