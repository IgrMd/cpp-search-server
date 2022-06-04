#include "search_server.h"

SearchServer::SearchServer(const std::string& stop_words)
	: SearchServer(SplitIntoWords(stop_words)) {
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
									const std::vector<int>& ratings) {
	if (document_id < 0) { //id must be greater than 0
		throw std::invalid_argument("doc's id ("s + std::to_string(document_id) + ") is negative"s );
	}
	if (documents_.count(document_id)) { //new doc id must be new
		throw std::invalid_argument("doc's id ("s + std::to_string(document_id) + ") is already exists"s);
	}
	std::vector<std::string> words;
	try {
		words = SplitIntoWordsNoStop(document);
	} catch (const std::invalid_argument& error) {
		throw std::invalid_argument("doc's id ("s + std::to_string(document_id) + ") - "s + error.what());
	}
	document_ids_.push_back(document_id);
	const double inv_word_count = 1.0 / words.size();
	for (const std::string& word : words) {
		word_to_document_freqs_[word][document_id] += inv_word_count;
		document_to_word_freqs_[document_id][word] += inv_word_count;
	}
	documents_.emplace(document_id,
		DocumentData{
			ComputeAverageRating(ratings),
			status
		});
}


int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
	if (ratings.empty()) {
		return 0;
	}
	const int rating_sum = accumulate(ratings.begin(), ratings.end(),0);
	return rating_sum / static_cast<int>(ratings.size());
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
	return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

int SearchServer::GetDocumentCount() const {
	return documents_.size();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const{
	static const std::map<std::string, double> empty_result;
	return document_to_word_freqs_.count(document_id) ?
			document_to_word_freqs_.at(document_id) :
			empty_result;
}

/*int SearchServer::GetDocumentId(int number) const {
	return document_ids_.at(number);
}*/

bool SearchServer::IsStopWord(const std::string& word) const {
	return stop_words_.count(word) > 0;
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
	Query query;
	for (const std::string& word : SplitIntoWords(text)) {
		const QueryWord query_word = ParseQueryWord(word);
		if (!query_word.is_stop) {
			if (query_word.is_minus) {
				query.minus_words.insert(query_word.data);
			} else {
				query.plus_words.insert(query_word.data);
			}
		}
	}
	return query;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
	if (text.empty()) {
		throw std::invalid_argument("empty word in query"s);
	}
	if (!IsValidWord(text)) {
		throw std::invalid_argument("control character in query words"s);
	}
	bool is_minus = false;
	if (text[0] == '-') {
		is_minus = true;
		text = text.substr(1);
		if (text.empty()) {
			throw std::invalid_argument("no characters after \"-\" in query"s);
		}
		if (text[0] == '-') {
			throw std::invalid_argument("double \"-\" in query minus-word"s);
		}
	}
	return {
		text,
		is_minus,
		IsStopWord(text)
	};
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
	const Query query = ParseQuery(raw_query);
	std::vector<std::string> matched_words;
	for (const std::string& word : query.plus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.push_back(word);
		}
	}
	for (const std::string& word : query.minus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.clear();
			break;
		}
	}
	std::tuple<std::vector<std::string>, DocumentStatus>result = {matched_words, documents_.at(document_id).status}; // @suppress("Field cannot be resolved") // @suppress("Invalid arguments")
	return result;
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, const DocumentStatus filter_status) const {
	return FindTopDocuments(raw_query,
			[filter_status](int document_id, DocumentStatus status, int rating)
			{ return status == filter_status ;});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
	return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

void SearchServer::SetStopWords(const std::string& text) {
	const std::vector<std::string> stop_words = SplitIntoWords(text);
	for (const std::string& word : MakeSetOfStopWords(stop_words)) {
		stop_words_.insert(word);
	}
}

void SearchServer::RemoveDocument(int document_id) {
	if (!document_to_word_freqs_.count(document_id)) {
		return;
	}

	for (auto& [key, map] : word_to_document_freqs_) {
		if (map.count(document_id)) {
			map.erase(document_id);
		}
	}
	document_to_word_freqs_.erase(document_id);
	documents_.erase(document_id);
	document_ids_.erase(find(document_ids_.begin(), document_ids_.end(), document_id));
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
	std::vector<std::string> words;
	for (const std::string& word : SplitIntoWords(text)) {
		if (!IsValidWord(word)) {
			throw std::invalid_argument("control character in word \""s + word + "\""s);
		}
		if (!IsStopWord(word)) {
			words.push_back(word);
		}
	}
	return words;
}
