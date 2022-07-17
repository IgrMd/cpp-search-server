#include "string_processing.h"

bool IsValidWord(const std::string_view& word) {
	// A valid word must not contain special characters
	return std::none_of(word.begin(), word.end(), [](char c) {
		return c >= '\0' && c < ' ';
		});
}

std::vector<std::string> SplitIntoWords(const std::string& text) {
	std::vector<std::string> words;
	std::string word;
	for (const char c : text) {
		if (c == ' ') {
			if (!word.empty()) {
				words.push_back(word);
				word.clear();
			}
		} else {
			word += c;
		}
	}
	if (!word.empty()) {
		words.push_back(word);
	}
	return words;
}
std::vector<std::string_view> SplitIntoWordsView(const std::string_view& str) {
	std::vector<std::string_view> words;
	int64_t pos = str.find_first_not_of(" ");
	const int64_t pos_end = str.npos;
	while (pos != pos_end) {
		int64_t space = str.find(' ', pos);
		words.push_back(space == pos_end ? str.substr(pos) : str.substr(pos, space - pos));
		pos = str.find_first_not_of(" ", space);
	}
	return words;
}
