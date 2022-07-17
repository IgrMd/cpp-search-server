#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
	: search_server_(search_server)
	, no_results_requests_(0)
	, current_time_(0) {
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus filter_status) {
	return AddFindRequest(raw_query,
			[filter_status](int document_id, DocumentStatus status, int rating)
			{ return status == filter_status ;});
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
	return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const {
	return std::count_if(requests_.begin(), requests_.end(),
			[](const auto& request_result)
			{return request_result.results == 0;});
}

void RequestQueue::AddRequest(int results_num) {
	++current_time_;
	if (requests_.size() < min_in_day_) {
		requests_.push_back({current_time_, results_num});
	} else {
		requests_.pop_front();
		requests_.push_back({current_time_, results_num});
	}
}
