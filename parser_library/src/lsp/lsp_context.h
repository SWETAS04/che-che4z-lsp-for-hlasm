/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#ifndef LSP_CONTEXT_H
#define LSP_CONTEXT_H

#include "file_info.h"

namespace hlasm_plugin::parser_library::lsp {

class lsp_context
{
    std::unordered_map<std::string, file_info> files_;
    std::unordered_map<context::id_index, macro_info_ptr> macros_;

public:
    void add_file(file_info file_i);
    void add_copy(context::copy_member_ptr copy);
    void add_macro(macro_info_ptr macro_i);

    void update_file_info(const std::string& name, const occurence_storage& occurences);

private:
    void distribute_macro_i(const macro_info& macro_i);
};

} // namespace hlasm_plugin::parser_library::lsp

#endif