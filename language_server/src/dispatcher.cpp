#include <iostream>
#include <string>
#include <sstream>
#include <memory>

#include "dispatcher.h"
#include "logger.h"
#include "json.hpp"

namespace hlasm_plugin {
namespace language_server {

dispatcher::dispatcher(std::istream& in, std::ostream& out, server& server) :
	server_(server), in_(in), out_(out)
{
	server_.set_send_message_provider(this);
}

static const std::string content_length_string_ = "Content-Length: ";

void dispatcher::write_message(const std::string & in)
{
	LOG_INFO(in);
	std::lock_guard<std::mutex> guard(mtx_);
	out_.write(content_length_string_.c_str(), content_length_string_.size());
	std::string size = std::to_string(in.size());
	out_.write(size.c_str(), size.size());
	out_.write("\r\n\r\n", 4);
	out_.write(in.c_str(), in.size());
}

void dispatcher::reply(const json & message)
{
	write_message(message.dump());
}

bool dispatcher::read_message(std::string & out)
{
	// A Language Server Protocol message starts with a set of HTTP headers,
	// delimited  by \r\n, and terminated by an empty line (\r\n).
	std::streamsize content_length = 0;
	std::string line;
	size_t content_len = std::string(content_length_string_).size();
	for(;;)
	{
		if (in_.eof() || in_.fail())
			return false;

		in_ >> line;

		// Content-Length is a mandatory header, and the only one we handle.
		if (line.substr(0, content_len) == content_length_string_)
		{
			if (content_length != 0)
			{
				LOG_WARNING("Duplicate Content-Length header received. The first one is ignored.");
			}

			std::stringstream str(line.substr(content_len));

			str >> content_length;
			continue;
		}
		else if (line == "\r")
		{
			// An empty line indicates the end of headers.
			// Go ahead and read the JSON.

			// The >> function left \n as the next character, we need to remove it
			// before we use in_.read(). When we use in_ >> line, all whitespace is
			// ignored, so it is only needed in this case
			in_.ignore();

			break;
		}
		else
		{
			// It's another header, ignore it.
		}
		
	}

	if (content_length > 1 << 30)
	{ // 1024M
		std::ostringstream ss;
		ss << "Refusing to read message with long Content-Length" << content_length << ". ";
		LOG_WARNING(ss.str());
		return false;
	}
	if (content_length == 0)
	{
		std::ostringstream ss;
		ss << "Warning: Missing Content-Length header, or zero-length message.";
		LOG_WARNING(ss.str());
		return false;
	}

	std::streamsize pos = 0;

	std::streamsize read;
	out.resize((size_t)content_length);
	for (; pos < content_length; pos += read) {

		in_.read(&out[(size_t)pos], content_length - pos);
		read = in_.gcount();
		if (read == 0)
		{
			std::ostringstream ss;
			ss << "Input was aborted. Read only " << pos << " bytes of expected " << content_length;
			LOG_WARNING(ss.str());
			return false;
		}
	}

	return true;
}

int dispatcher::run_server_loop()
{
	std::string message;
	for (;;)
	{
		if (in_.fail())
		{
			LOG_ERROR("IO error");
			return 1;
		}
		if (in_.eof())
		{
			LOG_ERROR("IO: unexpected end of file");
			return 1;
		}
		
		message.clear();

		if (read_message(message))
		{
			if(message.size() < 500)
				LOG_INFO(message);

			json message_json = 0;
			try {
				message_json = nlohmann::json::parse(message);
			}
			catch (nlohmann::json::exception)
			{
				LOG_WARNING("Could not parse received JSON: " + message);
				continue;
			}
			server_.message_received(message_json);
		}

		//if exit notification came without prior shutdown request, return error 1
		if (server_.is_exit_notification_received())
		{
			if (server_.is_shutdown_request_received())
				return 0;
			else
				return 1;
		}
	}
}

} //namespace language_server
} //namespace hlasm_plugin
