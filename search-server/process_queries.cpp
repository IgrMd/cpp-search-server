#include "process_queries.h"


std::vector<std::vector<Document>> ProcessQueries(
	const SearchServer& search_server,
	const std::vector<std::string>& queries) {
	std::vector<std::vector<Document>> documents_lists(queries.size());
	std::transform(std::execution::par, queries.begin(), queries.end(), 
		documents_lists.begin(),
		[&search_server](const std::string query) { return search_server.FindTopDocuments(query); });
	return documents_lists;
}

std::list<Document> ProcessQueriesJoined(
	const SearchServer& search_server,
	const std::vector<std::string>& queries) {
	std::list<Document> documents_list;
	for (const auto& Documents : ProcessQueries(search_server, queries)) {
		for (const auto& document : Documents) {
			documents_list.push_back(document);
		}
	}
	return documents_list;
}