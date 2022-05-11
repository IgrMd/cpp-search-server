#pragma once
#include <algorithm>
#include <deque>
#include <string>
#include <vector>

#include "document.h"
#include "search_server.h"

class RequestQueue {
public:
	explicit RequestQueue(const SearchServer& search_server);

	template <typename DocumentPredicate>
	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus filter_status);

	std::vector<Document> AddFindRequest(const std::string& raw_query);

	int GetNoResultRequests() const;

private:
	const SearchServer& search_server_;

	struct QueryResult {
		uint64_t timestamp;
		int results;
	};

	std::deque<QueryResult> requests_;

	const static int min_in_day_ = 1440;

	int no_results_requests_;

	uint64_t current_time_;

	void AddRequest(int results_num);
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
	const auto foud_docs = search_server_.FindTopDocuments(raw_query, document_predicate);
	AddRequest(foud_docs.size());
	return foud_docs;
}
