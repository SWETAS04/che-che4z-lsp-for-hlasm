#ifndef HLASMPLUGIN_PARSERLIBARY_DIAGNOSABLE_CTX_TEST_H
#define HLASMPLUGIN_PARSERLIBARY_DIAGNOSABLE_CTX_TEST_H
#include "common_testing.h"

TEST(diagnosable_ctx, one_file_diag)
{
	std::string input = 
R"( MACRO
 M2
 lr 1,
 MEND

 MACRO
 M1
 lr 1,1
 M2
 anop
 mend

 lr 1,1
 M1
)";
	analyzer a(input);
	a.analyze();

	a.collect_diags();
	ASSERT_EQ(a.diags().size(), (size_t)1);
	
	EXPECT_EQ(a.diags()[0].diag_range.start.line, (position_t) 2);
	EXPECT_EQ(a.diags()[0].related.size(), (size_t) 2);
	EXPECT_EQ(a.diags()[0].related[0].location.rang.start.line, (position_t) 8);
	EXPECT_EQ(a.diags()[0].related[1].location.rang.start.line, (position_t) 13);
}




#endif
