#pragma once
#include <algorithm>
#include <string>
#include <vector>

//проверка слова на "правильность"
bool IsValidWord(const std::string& word);

//получение из строки контейнера отдельных слов
std::vector<std::string> SplitIntoWords(const std::string& text);
