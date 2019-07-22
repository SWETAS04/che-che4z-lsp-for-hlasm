#include "../src/lsp/feature_text_synchronization.h"

#include "ws_mngr_mock.h"
#include "response_provider_mock.h"
#ifdef _WIN32

TEST(text_synchronization, did_open_file)
{
	using namespace ::testing;
	ws_mngr_mock ws_mngr;
	response_provider_mock response_mock;
	lsp::feature_text_synchronization f(ws_mngr, response_mock);
	std::map<std::string, method> notifs;
	f.register_methods(notifs);

	json params1 = R"({"textDocument":{"uri":"file:///c%3A/test/one/blah.txt","languageId":"plaintext","version":4,"text":"sad"}})"_json;

	EXPECT_CALL(ws_mngr, did_open_file(StrEq("c:\\test\\one\\blah.txt"), 4, StrEq("sad"), 3));

	notifs["textDocument/didOpen"]("", params1);
}

MATCHER_P2(PointerAndSizeEqArray, pointer, size, "") {
	(void)result_listener;
	const size_t actual_size = std::get<1>(arg);
	if (actual_size != size)
		return false;
	for (size_t i = 0; i < size; i++) {
		if (!(std::get<0>(arg)[i] == pointer[i]))
			return false;
	}
	return true;
}

TEST(text_synchronization, did_change_file)
{
	using namespace ::testing;
	ws_mngr_mock ws_mngr;
	response_provider_mock response_mock;
	lsp::feature_text_synchronization f(ws_mngr, response_mock);
	std::map<std::string, method> notifs;
	f.register_methods(notifs);

	json params1 = R"({"textDocument":{"uri":"file:///c%3A/test/one/blah.txt","version":7},"contentChanges":[{"range":{"start":{"line":0,"character":0},"end":{"line":0,"character":8}},"rangeLength":8,"text":"sad"}, {"range":{"start":{"line":1,"character":12},"end":{"line":1,"character":14}},"rangeLength":2,"text":""}]})"_json;

	document_change expected1[2]{ { { {0,0}, {0,8} }, "sad", 3 },  { { {1,12}, {1,14} }, "", 0 } };

	EXPECT_CALL(ws_mngr, did_change_file(StrEq("c:\\test\\one\\blah.txt"), 7, _, 2)).With(Args<2, 3>(PointerAndSizeEqArray(expected1, std::size(expected1))));
	notifs["textDocument/didChange"]("",params1);



	document_change expected2[1]{ { "sad", 3 } };
	json params2 = R"({"textDocument":{"uri":"file:///c%3A/test/one/blah.txt","version":7},"contentChanges":[{"text":"sad"}]})"_json;
	EXPECT_CALL(ws_mngr, did_change_file(StrEq("c:\\test\\one\\blah.txt"), 7, _, 1)).With(Args<2, 3>(PointerAndSizeEqArray(expected2, std::size(expected2))));

	notifs["textDocument/didChange"]("",params2);



	json params3 = R"({"textDocument":{"uri":"file:///c%3A/test/one/blah.txt"},"contentChanges":[{"range":{"start":{"line":0,"character":0},"end":{"line":0,"character":8}},"rangeLength":8,"text":"sad"}, {"range":{"start":{"line":1,"character":12},"end":{"line":1,"character":14}},"rangeLength":2,"text":""}]})"_json;

	EXPECT_THROW(notifs["textDocument/didChange"]("",params3), nlohmann::basic_json<>::exception);

}

TEST(text_synchronization, did_close_file)
{
	using namespace ::testing;
	ws_mngr_mock ws_mngr;
	response_provider_mock response_mock;
	lsp::feature_text_synchronization f(ws_mngr, response_mock);
	std::map<std::string, method> notifs;
	f.register_methods(notifs);

	json params1 = R"({"textDocument":{"uri":"file:///c%3A/test/one/blah.txt"}})"_json;
	EXPECT_CALL(ws_mngr, did_close_file(StrEq("c:\\test\\one\\blah.txt"))),

	notifs["textDocument/didClose"]("",params1);
}

TEST(feature, uri_to_path)
{
	using namespace hlasm_plugin::language_server;
	EXPECT_EQ(feature::uri_to_path("file://czprfs50/Public"), "\\\\czprfs50\\Public");
	EXPECT_EQ(feature::uri_to_path("file:///C%3A/Public"), "c:\\Public");
}
#endif // _WIN32
