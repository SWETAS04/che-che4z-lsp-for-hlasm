#include "../include/shared/token_stream.h"
#include "../include/shared/lexer.h"

using namespace hlasm_plugin::parser_library;
using namespace antlr4;

token_stream::token_stream(antlr4::TokenSource* token_source) 
	: antlr4::BufferedTokenStream(token_source), enabled_cont_(false),enabled_hidden_(false) {}

void token_stream::enable_continuation() { enabled_cont_ = true; }

void token_stream::disable_continuation() { enabled_cont_ = false; }

void token_stream::enable_hidden() { enabled_hidden_ = true; }

void token_stream::disable_hidden() { enabled_hidden_ = false; }

void token_stream::rewind_input(lexer::stream_position pos, bool insert_EOLLN)
{
	auto& lexer_tmp = dynamic_cast<lexer&>(*_tokenSource);
	
	if (lexer_tmp.last_lln_end_position().offset == pos.offset) // no rewind needed
	{
		assert(!insert_EOLLN);
		return;
	}

	lexer_tmp.rewind_input(pos);

	if (_tokens.back()->getType() != lexer::EOLLN)
	{
		auto index_tmp = _tokens.back()->getTokenIndex();
		_tokens.pop_back();
		lexer_tmp.delete_token(index_tmp);
	}

	if (insert_EOLLN)
		lexer_tmp.insert_EOLLN();

	_fetchedEOF = false;
	sync(_p);
}

bool token_stream::consume_EOLLN()
{
	if (_tokens.back()->getType() == lexer::EOLLN)
	{
		consume();
		return true;
	}
	return false;
}

antlr4::Token * hlasm_plugin::parser_library::token_stream::LT(ssize_t k)
{
	lazyInit();
	if (k == 0)
		return nullptr;

	if (k < 0) 
		return LB(-k);

	size_t i = _p;

	for (ssize_t n = 1; n < k; ++n)
	{
		if (sync(i + 1)) 
			i = next_token_on_channel(i + 1);
	}

	return get(i);
}

std::string hlasm_plugin::parser_library::token_stream::getText(const antlr4::misc::Interval & interval)
{
	lazyInit();
	size_t start = interval.a;
	size_t stop = interval.b;
	if (start == INVALID_INDEX || stop == INVALID_INDEX) {
		return "";
	}
	if (stop >= _tokens.size()) {
		stop = _tokens.size() - 1;
	}

	sync(stop);

	std::stringstream ss;
	for (size_t i = start; i <= stop; i++) {
		Token *t = _tokens[i].get();
		if (t->getType() == Token::EOF) {
			break;
		}
		ss << t->getText();
	}
	return ss.str();
}

ssize_t hlasm_plugin::parser_library::token_stream::adjustSeekIndex(size_t i)
{
	return next_token_on_channel(i);
}

antlr4::Token * hlasm_plugin::parser_library::token_stream::LB(size_t k)
{
	if (k == 0 || _p < k || size() == 0) 
		return nullptr;

	size_t i = _p;
	size_t n = 0;

	while (n<k)
	{
		if (i == 0)
			return nullptr;
		else
			--i;

		antlr4::Token* token = get(i);

		if (is_on_channel(token)) 
			++n;
	}

	return get(i);
}

size_t hlasm_plugin::parser_library::token_stream::next_token_on_channel(size_t i)
{
	sync(i);

	if (i >= size())
		return size() - 1;

	auto token = get(i);

	while (!is_on_channel(token))
	{
		++i;
		sync(i);
		token = get(i);
	}
	return i;
}

antlr4::Token* hlasm_plugin::parser_library::token_stream::previous_token_on_channel(size_t i)
{
	sync(i);

	if (i >= size()) 
	{
		if (size() == 0)
			return nullptr;
		else
			return get(size() - 1);
	}

	while (true)
	{
		antlr4::Token* token = get(i);

		if (is_on_channel(token))
			return token;

		if (i == 0)
			return nullptr;
		else
			--i;
	}
}

bool hlasm_plugin::parser_library::token_stream::is_on_channel(antlr4::Token * token)
{
	return token->getChannel() == lexer::Channels::DEFAULT_CHANNEL 
		|| (enabled_hidden_ && token->getChannel() == lexer::Channels::HIDDEN_CHANNEL)
		|| (enabled_cont_ && token->getType() == lexer::CONTINUATION) 
		|| token->getType() == antlr4::Token::EOF;
}
