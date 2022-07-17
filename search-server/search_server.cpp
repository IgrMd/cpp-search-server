#include "search_server.h"

SearchServer::SearchServer(const std::string& stop_words) {
	MakeSetOfStopWords(SplitIntoWordsView(stop_words));
}

SearchServer::SearchServer(const std::string_view& stop_words) {
	MakeSetOfStopWords(SplitIntoWordsView(stop_words));
}

void SearchServer::AddDocument(int document_id, const std::string_view& document, DocumentStatus status,
	const std::vector<int>& ratings) {
	if (document_id < 0) { //id must be greater than 0
		throw std::invalid_argument("doc's id ("s + std::to_string(document_id) + ") is negative"s);
	}
	if (documents_.count(document_id)) { //new doc id must be new
		throw std::invalid_argument("doc's id ("s + std::to_string(document_id) + ") is already exists"s);
	}
	std::vector<std::string_view> words;
	try {
		words = SplitIntoWordsNoStop(document);
	} catch (const std::invalid_argument& error) {
		throw std::invalid_argument("doc's id ("s + std::to_string(document_id) + ") - "s + error.what());
	}
	document_ids_.push_back(document_id);
	const double inv_word_count = 1.0 / words.size();
	for (const std::string_view& word : words) {
		const auto& [it, is_inserted] = words_.insert(std::string{ word });
		word_to_document_freqs_[*it][document_id] += inv_word_count;
		document_to_word_freqs_[document_id][*it] += inv_word_count;
	}
	documents_.emplace(document_id,
		DocumentData{
			ComputeAverageRating(ratings),
			status
		});
}

std::vector<int>::const_iterator SearchServer::begin() const {
	return document_ids_.begin();
}

std::vector<int>::const_iterator SearchServer::end() const {
	return document_ids_.end();
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
	if (ratings.empty()) {
		return 0;
	}
	const int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
	return rating_sum / static_cast<int>(ratings.size());
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view& word) const {
	return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

size_t SearchServer::GetDocumentCount() const {
	return documents_.size();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
	static const std::map<std::string_view, double> empty_result;
	return document_to_word_freqs_.count(document_id) ?
		document_to_word_freqs_.at(document_id) :
		empty_result;
}

bool SearchServer::IsStopWord(const std::string_view& word) const {
	return stop_words_.count(word) > 0;
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view& text) const {
	Query query;
	for (const std::string_view& word : SplitIntoWordsView(text)) {
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

SearchServer::QueryPar SearchServer::ParseQueryPar(const std::string_view& text) const {
	QueryPar query;
	const std::vector<std::string_view> words = SplitIntoWordsView(text);
	std::for_each(
		words.begin(), words.end(),
		[&](auto& word){
			QueryWord query_word = ParseQueryWord(word);
			if (!query_word.is_stop) {
				if (query_word.is_minus) {
					query.minus_words.push_back(move(query_word.data));
				} else {
					query.plus_words.push_back(move(query_word.data));
				}
			}
		}
	);
	return query;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
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

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view& raw_query,
	int document_id) const {
	const Query query = ParseQuery(raw_query);
	std::vector<std::string_view> matched_words;
	for (const std::string_view& word : query.minus_words) {
		if (!word_to_document_freqs_.count(word)) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			return { matched_words, documents_.at(document_id).status };
		}
	}
	for (const std::string_view& word : query.plus_words) {
		if (!word_to_document_freqs_.count(word)) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.push_back(word_to_document_freqs_.find(word)->first);
		}
	}
	return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy seq,
	const std::string_view& raw_query, int document_id) const {
	return SearchServer::MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy par,
	const std::string_view& raw_query, int document_id) const {
	const QueryPar query = ParseQueryPar(raw_query);
	std::vector<std::string_view> matched_words;
	for (const std::string_view& word : query.minus_words) {
		if (word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id)) {
			return { matched_words, documents_.at(document_id).status };
		}
	}
	matched_words.resize(query.plus_words.size());
	std::transform(par,
		query.plus_words.begin(), query.plus_words.end(),
		matched_words.begin(),
		[&matched_words, this, document_id](auto& word) {
			return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id) ?
				word : "";
		}
	);
	std::sort(matched_words.begin(), matched_words.end());
	auto it = std::unique(matched_words.begin(), matched_words.end());
	matched_words.erase(it, matched_words.end());
	if (*matched_words.begin() == "") {
		matched_words.erase(matched_words.begin());
	}
	return { matched_words, documents_.at(document_id).status };
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, const DocumentStatus filter_status) const {
	return FindTopDocuments(raw_query,
		[filter_status](int document_id, DocumentStatus status, int rating) { return status == filter_status; });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query) const {
	return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

void SearchServer::SetStopWords(const std::string_view& text) {
	const std::vector<std::string_view> stop_words = SplitIntoWordsView(text);
	MakeSetOfStopWords(stop_words);
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const {
	std::vector<std::string_view> words;
	for (const std::string_view& word : SplitIntoWordsView(text)) {
		if (!IsValidWord(word)) {
			throw std::invalid_argument("control character in word \""s + std::string{word} + "\""s);
		}
		if (!IsStopWord(word)) {
			words.push_back(word);
		}
	}
	return words;
}

void SearchServer::RemoveDocument(int document_id) {
	if (!document_to_word_freqs_.count(document_id)) {
		return;
	}
	const auto& delete_collection = document_to_word_freqs_.at(document_id);
	for (const auto& [str, d] : delete_collection) {
		word_to_document_freqs_.at(str).erase(document_id);
	}
	SearchServer::EraseOther(document_id);
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy seq, int document_id) {
	SearchServer::RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy par, int document_id) {
	if (!document_to_word_freqs_.count(document_id)) {
		return;
	}
	std::vector<std::string_view> words(document_to_word_freqs_.at(document_id).size());
	std::transform(par,
		document_to_word_freqs_.at(document_id).begin(), document_to_word_freqs_.at(document_id).end(),
		words.begin(),
		[](auto& word_freqs) {return std::string_view{word_freqs.first}; }
	);
	std::for_each(par,
		words.begin(), words.end(),
		[document_id, this](auto& word) {
			word_to_document_freqs_.at(std::string{ word }).erase(document_id);
		}
	);
	SearchServer::EraseOther(document_id);
}

void SearchServer::EraseOther(int document_id) {
	document_to_word_freqs_.erase(document_id);
	documents_.erase(document_id);
	document_ids_.erase(std::find(document_ids_.begin(), document_ids_.end(), document_id));
}
