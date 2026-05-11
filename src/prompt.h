// SPDX-License-Identifier: MIT
#pragma once
#include <string>

namespace hymt {

std::string BuildUserPrompt(const std::string& text,
                            const std::string& source_lang,
                            const std::string& target_lang,
                            const std::string& terminology,
                            const std::string& context);

std::string LangName(const std::string& tag);

}  // namespace hymt
