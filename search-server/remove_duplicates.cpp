#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
	std::set<int> duplicates;
	std::set<std::set<std::string>> check_dulicates;
	for (int i : search_server) {
		std::set<std::string> set_of_keys;
		for (const auto& [key, value] : search_server.GetWordFrequencies(i)) {
			set_of_keys.insert(key);
		}
		auto [it, is_inserted] = check_dulicates.insert(set_of_keys);
		if (!is_inserted) {
			duplicates.insert(i);
		}
	}
	for (int i : duplicates) {
		std::cout << "Found duplicate document id "s << i << std::endl;
		search_server.RemoveDocument(i);
	}
}

