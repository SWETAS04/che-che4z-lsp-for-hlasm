#ifndef HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_IMPL_H
#define HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_IMPL_H

#include "shared/workspace_manager.h"
#include "workspace.h"
#include "file_manager_impl.h"

namespace hlasm_plugin::parser_library
{

class workspace_manager::impl : public diagnosable_impl
{
public:
	impl() : implicit_workspace_({ file_manager_ }) {}
	impl(const impl &) = delete;
	impl & operator= (const impl &) = delete;

	impl(impl &&) = default;
	impl & operator= (impl &&) = default;

	size_t get_workspaces(ws_id * workspaces, size_t max_size)
	{
		size_t size = 0;
		
		for (auto it = workspaces_.begin(); size < max_size && it != workspaces_.end(); ++size, ++it)
		{
			workspaces[size] = &it->second;
		}
		return size;
	}

	size_t get_workspaces_count()
	{
		return workspaces_.size();
	}

	void add_workspace(std::string name, std::string uri)
	{
		auto ws = workspaces_.emplace(name, workspace { uri, name, file_manager_ });
		ws.first->second.open();

		notify_diagnostics_consumers();
	}
	void remove_workspace(std::string uri)
	{
		auto it = workspaces_.find(uri);
		if (it == workspaces_.end())
			return; //erase does no action, if the key does not exist
		workspaces_.erase(uri);
		notify_diagnostics_consumers();
	}
	
	void did_open_file(const std::string & document_uri, version_t version, std::string text)
	{
		file_manager_.did_open_file(document_uri, version, std::move(text));
		workspace & ws = ws_path_match(document_uri);
		ws.did_open_file(document_uri);
		notify_highlighting_consumers();
		notify_diagnostics_consumers();
	}
	void did_change_file(const std::string document_uri, version_t version, const document_change * changes, size_t ch_size)
	{
		file_manager_.did_change_file(document_uri, version, changes, ch_size);
		workspace & ws = ws_path_match(document_uri);
		ws.did_change_file(document_uri, changes, ch_size);
		notify_highlighting_consumers();
		notify_diagnostics_consumers();
	}

	void did_close_file(const std::string document_uri)
	{
		file_manager_.did_close_file(document_uri);
	}

	void register_highlighting_consumer(highlighting_consumer * consumer)
	{
		hl_consumers_.push_back(consumer);
	}

	void register_diagnostics_consumer(diagnostics_consumer * consumer)
	{
		diag_consumers_.push_back(consumer);
	}
	
	semantics::position_uri_s found_position;
	position_uri definition(std::string document_uri, const position pos)
	{
		auto file = file_manager_.find(document_uri);
		if (dynamic_cast<processor_file*>(file) != nullptr)
			found_position = file_manager_.find_processor_file(document_uri)->get_lsp_info().go_to_definition(pos);
		else
			found_position = { document_uri,pos };
		return found_position;
	}

	std::vector<semantics::position_uri_s> found_refs;
	position_uris references(std::string document_uri, const position pos)
	{
		auto file = file_manager_.find(document_uri);
		if (dynamic_cast<processor_file*>(file) != nullptr)
			found_refs = file_manager_.find_processor_file(document_uri)->get_lsp_info().references(pos);
		else
			found_refs.clear();
		return { found_refs.data(), found_refs.size() };
	}

	std::vector<std::string> output;
	std::vector<const char*> coutput;
	const string_array hover(const char * document_uri, const position pos)
	{
		auto file = file_manager_.find(document_uri);
		if (dynamic_cast<processor_file*>(file) != nullptr)
			output = file_manager_.find_processor_file(document_uri)->get_lsp_info().hover(pos);
		else
			output.clear();
		coutput.clear();
		for (const auto & str : output)
			coutput.push_back(str.c_str());
		
		return { coutput.data(), coutput.size() };
	}

	semantics::completion_list_s completion_result;
	completion_list completion(const char* document_uri, const position pos, const char trigger_char, int trigger_kind)
	{
		auto file = file_manager_.find(document_uri);
		if (dynamic_cast<processor_file*>(file) != nullptr)
			completion_result = file_manager_.find_processor_file(document_uri)->get_lsp_info().completion(pos, trigger_char, trigger_kind);
		else
			completion_result = semantics::completion_list_s();
		return completion_result;
	}

private:

	virtual void collect_diags() const override
	{
		collect_diags_from_child(file_manager_);

		for (auto & it : workspaces_)
			collect_diags_from_child(it.second);
	}

	void notify_highlighting_consumers()
	{
		auto file_list = file_manager_.list_updated_files();
		all_highlighting_info hl_info(file_list.data(), file_list.size());
		for (auto consumer : hl_consumers_)
		{
			consumer->consume_highlighting_info(hl_info);
		}
	}

	void notify_diagnostics_consumers()
	{
		diags().clear();
		collect_diags();
		diagnostic_list l(diags().data(), diags().size());
		for (auto consumer : diag_consumers_)
		{
			consumer->consume_diagnostics(l);
		}
	}

	size_t prefix_match(const std::string & first, const std::string & second)
	{
		size_t match = 0;
		size_t i1 = 0;
		size_t i2 = 0;
		while (first[i1] == second[i2] && i1 < first.size() && i2 < second.size())
		{
			++i1;
			++i2;
			++match;
		}
		return match;
	}

	//returns implicit workspace, if the file does not belong to any workspace
	workspace & ws_path_match(const std::string & document_uri)
	{
		size_t max = 0;
		workspace * max_ws = nullptr;
		for (auto & ws : workspaces_)
		{
			size_t match = prefix_match(document_uri, ws.second.uri());
			if (match > max && match >= ws.first.size())
			{
				max = match;
				max_ws = &ws.second;
			}
		}
		if (max_ws == nullptr)
			return implicit_workspace_;
		else
			return *max_ws;
	}

	std::unordered_map<std::string, workspace> workspaces_;
	file_manager_impl file_manager_;

	workspace implicit_workspace_;

	std::vector<highlighting_consumer *> hl_consumers_;
	std::vector<diagnostics_consumer *> diag_consumers_;

};
}

#endif // !HLASMPLUGIN_PARSERLIBRARY_WORKSPACE_MANAGER_IMPL_H
