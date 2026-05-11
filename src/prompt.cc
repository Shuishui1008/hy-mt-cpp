// SPDX-License-Identifier: MIT
#include "prompt.h"

#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace hymt {

namespace {

std::string ToLower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

const std::unordered_map<std::string, std::string>& LangTable() {
  static const std::unordered_map<std::string, std::string> kMap = {
      {"zh", "Chinese"},     {"zh-cn", "Chinese"},
      {"zh-tw", "Traditional Chinese"},
      {"en", "English"},     {"ja", "Japanese"},     {"ko", "Korean"},
      {"fr", "French"},      {"de", "German"},       {"es", "Spanish"},
      {"ru", "Russian"},     {"pt", "Portuguese"},   {"it", "Italian"},
      {"ar", "Arabic"},      {"th", "Thai"},         {"vi", "Vietnamese"},
      {"id", "Indonesian"},  {"ms", "Malay"},        {"tr", "Turkish"},
      {"nl", "Dutch"},       {"pl", "Polish"},       {"cs", "Czech"},
      {"hu", "Hungarian"},   {"fi", "Finnish"},      {"sv", "Swedish"},
      {"da", "Danish"},      {"no", "Norwegian"},    {"uk", "Ukrainian"},
      {"he", "Hebrew"},      {"hi", "Hindi"},        {"bn", "Bengali"},
      {"ur", "Urdu"},        {"fa", "Persian"},      {"ro", "Romanian"},
  };
  return kMap;
}

}  // namespace

std::string LangName(const std::string& tag) {
  if (tag.empty()) return "";
  auto key = ToLower(tag);
  const auto& tbl = LangTable();
  auto it = tbl.find(key);
  return it != tbl.end() ? it->second : tag;
}

std::string BuildUserPrompt(const std::string& text,
                            const std::string& source_lang,
                            const std::string& target_lang,
                            const std::string& terminology,
                            const std::string& context) {
  const std::string tgt = LangName(target_lang.empty() ? "en" : target_lang);
  const std::string src = LangName(source_lang);

  std::string prompt;
  prompt.reserve(text.size() + 256);

  if (!context.empty()) {
    prompt += "Context:\n";
    prompt += context;
    prompt += "\n\n";
  }
  if (!terminology.empty()) {
    prompt += "Terminology:\n";
    prompt += terminology;
    prompt += "\n\n";
  }

  if (src.empty()) {
    prompt += "Translate the following text into ";
    prompt += tgt;
    prompt += ".\n\n";
  } else {
    prompt += "Translate the following text from ";
    prompt += src;
    prompt += " into ";
    prompt += tgt;
    prompt += ".\n\n";
  }
  prompt += text;

  return prompt;
}

}  // namespace hymt
