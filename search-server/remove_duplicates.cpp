#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
	std::set<int> duplicates;
	for (auto it_i = search_server.begin(); it_i != search_server.end(); ++it_i) {
		std::set<std::string> set_of_keys_i;
		for (const auto& [key, value] : search_server.GetWordFrequencies(*it_i)) {
			set_of_keys_i.insert(key);
		}
		for (auto it_j = it_i+1; it_j != search_server.end(); ++it_j) {
			std::set<std::string> set_of_keys_j;
			for (const auto& [key, value] : search_server.GetWordFrequencies(*it_j)) {
				set_of_keys_j.insert(key);
			}
			if (set_of_keys_i == set_of_keys_j) {
				duplicates.insert(*it_j);
			}
		}
	}
	for (int i : duplicates) {
		std::cout << "Found duplicate document id "s << i << std::endl;
		search_server.RemoveDocument(i);
	}
}
