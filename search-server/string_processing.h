#pragma once
#include <algorithm>
#include <execution>
#include <list>
#include <string>
#include <vector>

//проверка слова на "правильность"
bool IsValidWord(const std::string_view& word);

//получение из строки контейнера отдельных слов
std::vector<std::string> SplitIntoWords(const std::string& text);

//получение из строки контейнера отдельных слов string_view
std::vector<std::string_view> SplitIntoWordsView(const std::string_view& text);