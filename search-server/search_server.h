#pragma once

#include "concurrent_map.h"
#include "document.h"
#include "log_duration.h"
#include "string_processing.h"

#include <algorithm>
#include <execution>
#include <functional>
#include <future>
#include <cmath>
#include <iostream>
#include <list>
#include <map>
#include <numeric>
#include <string>
#include <stdexcept>
#include <set>
#include <vector>
#include <unordered_set>
#include <utility>

using namespace std::string_literals;

constexpr int MAX_THREADS = 8;
constexpr int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double RELEVANCE_TRESHOLD = 1e-6;

class SearchServer {
public:
	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words);

	explicit SearchServer(const std::string& stop_words);

	explicit SearchServer(const std::string_view& stop_words);

	void AddDocument(int document_id, const std::string_view& document, DocumentStatus status,
		const std::vector<int>& ratings);

	std::vector<int>::const_iterator begin() const;

	std::vector<int>::const_iterator end() const;

	//Метод обрабатывает запрос, состоящий из строки
	std::vector<Document> FindTopDocuments(const std::string_view& raw_query) const;

	//Метод обрабатывает запрос, состоящий из строки + политика
	template<typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(ExecutionPolicy policy,
		const std::string_view& raw_query) const;

	//Метод обрабатывает запрос, состоящий из строки со статусом
	std::vector<Document> FindTopDocuments(const std::string_view& raw_query,
		const DocumentStatus filter_status) const;

	//Метод обрабатывает запрос, состоящий из строки со статусом + политика
	template<typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(ExecutionPolicy policy,
		const std::string_view& raw_query, const DocumentStatus filter_status) const;

	//Метод обрабатывает запрос, первый аргумент которого - строка, второй - функция-предикат
	template<typename Predic>
	std::vector<Document> FindTopDocuments(const std::string_view& raw_query, Predic predic) const;

	//Метод обрабатывает запрос, первый аргумент которого - строка, второй - функция-предикат + политика
	template<typename Predic, typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(ExecutionPolicy policy,
		const std::string_view& raw_query, Predic predic) const;

	size_t GetDocumentCount() const;

	const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view& raw_query,
		int document_id) const;

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy seq,
		const std::string_view& raw_query, int document_id) const;

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy par,
		const std::string_view& raw_query, int document_id) const;

	void RemoveDocument(int document_id);

	void RemoveDocument(std::execution::sequenced_policy seq, int document_id);

	void RemoveDocument(std::execution::parallel_policy par, int document_id);

	void SetStopWords(const std::string_view& text);

private:

	//структура данных документа (средний рейтинг, статус)
	struct DocumentData {
		int rating;
		DocumentStatus status;
	};
	//контейнер стоп-слов
	std::set<std::string, std::less<>> stop_words_;
	//контейнер слов
	std::set<std::string> words_;
	//контейнер std::map<слово, std::map<id документа, inverse document frequency(IDF)>>
	std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
	//контейнер std::map<id документа, std::map<слово, inverse document frequency(IDF)>>	
	std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
	//контейнер std::map<id документа, рейтинг-статус>
	std::map<int, DocumentData> documents_;
	//контейнер id документов в порядек их обавления
	std::vector<int> document_ids_;

	//вычисление среднего рейтинга документов
	static int ComputeAverageRating(const std::vector<int>& ratings);

	//вычисление Inverse Document Frequency
	double ComputeWordInverseDocumentFreq(const std::string_view& word) const;
	
	void EraseOther(int document_id);

	//Проверка слова на вхожение в перечень стоп-слов
	bool IsStopWord(const std::string_view& word) const;

	//любой контейнер преобразуем в std::set<string> и проверяем на корректность содержания
	template <typename StringContainer>
	void MakeSetOfStopWords(const StringContainer& container);

	//получение из строки контейнера отдельных слов, исключая стоп-слова
	std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view& text) const;

	//Структура поскового запроса (плюс- и минус-слова)
	struct Query {
		std::set<std::string_view> plus_words;
		std::set<std::string_view> minus_words;
	};

	//Структура поскового запроса (плюс- и минус-слова) с итераторами произвольного доступа
	struct QueryPar {
		std::vector<std::string_view> plus_words;
		std::vector<std::string_view> minus_words;
		void Normalize() {
			Normilize_(plus_words);
			Normilize_(minus_words);
		}
	private:
		void Normilize_(std::vector<std::string_view>& words) {
			std::sort(words.begin(), words.end());
			auto last = std::unique(words.begin(), words.end());
			words.erase(last, words.end());
		}

	};

	//парсинг запроса на минус- и плюс-слова
	Query ParseQuery(const std::string_view& text) const;

	//парсинг запроса на минус- и плюс-слова с параллелизацией
	QueryPar ParseQueryPar(const std::string_view& text) const;

	//Структура для идентификации слова поскового запроса (минус/плюс- или стоп-слово)
	struct QueryWord {
		std::string_view data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(std::string_view text) const;

	template<typename Predic, typename ExecutionPolicy>
	std::vector<Document> FindAllDocuments(ExecutionPolicy policy,
		const QueryPar& query, Predic predic) const;

};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words) {
	MakeSetOfStopWords(stop_words);
}

template<typename Predic, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy policy,
	const QueryPar& query, Predic predic) const {
	ConcurrentMap<int, double> document_to_relevance(MAX_THREADS);
	for_each(policy,
		query.plus_words.begin(), query.plus_words.end(),
		[&document_to_relevance, &query, predic, this, policy](auto& plus_word) {
			if (plus_word != "" && word_to_document_freqs_.count(plus_word)) {
				const double inverse_document_freq = ComputeWordInverseDocumentFreq(plus_word);
				for (const auto [document_id, term_freq] : word_to_document_freqs_.at(plus_word)) {
					bool is_contains_stopword = std::any_of(
						query.minus_words.begin(), query.minus_words.end(),
						[this, document_id](auto& minus_word) { return document_to_word_freqs_.at(document_id).count(minus_word); }
					);
					const DocumentData& doc = documents_.at(document_id);
					if (!is_contains_stopword && predic(document_id, doc.status, doc.rating)) {
						document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
					}
				}
			}
		}
	);

	std::vector<Document> matched_documents;
	matched_documents.reserve(document_to_relevance.Size());
	for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
		matched_documents.push_back({
			document_id,
			relevance,
			documents_.at(document_id).rating
			});
	}
	return matched_documents;
}


template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy,
	const std::string_view& raw_query, const DocumentStatus filter_status) const {
	return FindTopDocuments(policy, raw_query,
		[filter_status](int document_id, DocumentStatus status, int rating) { return status == filter_status; });
}

template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy,
	const std::string_view& raw_query) const {
	return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template<typename Predic>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, Predic predic) const {
	return FindTopDocuments(std::execution::seq, raw_query, predic);
}

template<typename Predic, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy,
	const std::string_view& raw_query, Predic predic) const {
	QueryPar query = ParseQueryPar(raw_query);
	query.Normalize();
	std::vector<Document> result = FindAllDocuments(policy, query, predic);
	std::sort(result.begin(), result.end(),
		[](const Document& lhs, const Document& rhs) {
			if (std::abs(lhs.relevance - rhs.relevance) < RELEVANCE_TRESHOLD) {
				return lhs.rating > rhs.rating;
			} else {
				return lhs.relevance > rhs.relevance;
			}
		});
	if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
		result.resize(MAX_RESULT_DOCUMENT_COUNT);
	}
	return result;
}

template <typename StringContainer>
void SearchServer::MakeSetOfStopWords(const StringContainer& container) {
	for (const std::string_view& str : container) {
		if (!str.empty()) {
			if (IsValidWord(str)) {
				stop_words_.insert(std::string{str});
			} else {
				throw std::invalid_argument("control character in stop-words"s);
			}
		}
	}
}
