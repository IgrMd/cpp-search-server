#pragma once
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <stdexcept>
#include <set>
#include <vector>
#include <utility>

#include "document.h"
#include "string_processing.h"

using  std::string_literals::operator ""s;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double RELEVANCE_TRESHOLD = 1e-6;

class SearchServer {
public:
	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words);

	explicit SearchServer(const std::string& stop_words);

	void AddDocument(int document_id, const std::string& document, DocumentStatus status,
										const std::vector<int>& ratings);

	std::vector<int>::const_iterator begin() const;

	std::vector<int>::const_iterator end() const;

	//Метод обрабатывает запрос, первый аргумент которого - строка, второй - функция-предикат
	template<typename Predic>
	std::vector<Document> FindTopDocuments(const std::string& raw_query, Predic predic) const;

	//Метод обрабатывает запрос, состоящий из строки со статусом
	std::vector<Document> FindTopDocuments(const std::string& raw_query, const DocumentStatus filter_status) const;

	//Метод обрабатывает запрос, состоящий из строки
	std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

	int GetDocumentCount() const;

	const std::map<std::string, double>& GetWordFrequencies(int document_id) const;

	//int GetDocumentId(int number) const;

	std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

	void RemoveDocument(int document_id);

	void SetStopWords(const std::string& text);

private:

	//структура данных документа (средний рейтинг, статус)
	struct DocumentData {
		int rating;
		DocumentStatus status;
	};

	std::set<std::string> stop_words_;										//контейнер стоп-слов
	std::map<std::string, std::map<int, double>> word_to_document_freqs_;	//контейнер std::map<слово, std::map<id документа, inverse document frequency(IDF)>>
	std::map<int, std::map<std::string, double>> document_to_word_freqs_;	//контейнер std::map<id документа, std::map<слово, inverse document frequency(IDF)>>
	std::map<int, DocumentData> documents_;									//контейнер std::map<id документа, рейтинг-статус>
	std::vector<int> document_ids_; 										//контейнер id документов в порядек их обавления

	//вычисление среднего рейтинга документов
	static int ComputeAverageRating(const std::vector<int>& ratings);

	//вычисление Inverse Document Frequency
	double ComputeWordInverseDocumentFreq(const std::string& word) const;

	//Проверка слова на вхожение в перечень стоп-слов
	bool IsStopWord(const std::string& word) const;

	//любой контейнер преобразуем в std::set<string> и проверяем на корректность содержания
	template <typename StringContainer>
	static std::set<std::string> MakeSetOfStopWords(const StringContainer& container);

	//получение из строки контейнера отдельных слов, исключая стоп-слова
	std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

	//Структура поскового запроса (плюс- и минус-слова)
	struct Query {
		std::set<std::string> plus_words;
		std::set<std::string> minus_words;
	};

	//парсинг запроса на минус- и плюс-слова
	Query ParseQuery(const std::string& text) const;

	//Структура для идентификации слова поскового запроса (минус/плюс- или стоп-слово)
	struct QueryWord {
		std::string data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(std::string text) const;

	template<typename Predic>
	std::vector<Document> FindAllDocuments(const Query& query, Predic predic) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
	: stop_words_(MakeSetOfStopWords(stop_words)) {
}

template<typename Predic>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, Predic predic) const {
	std::map<int, double> document_to_relevance;
	for (const std::string& word : query.plus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

		for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
			const DocumentData& doc = documents_.at(document_id);
			if (predic(document_id, doc.status, doc.rating)) {
				document_to_relevance[document_id] += term_freq * inverse_document_freq;
			}
		}
	}

	for (const std::string& word : query.minus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
			document_to_relevance.erase(document_id);
		}
	}

	std::vector<Document> matched_documents;
	for (const auto [document_id, relevance] : document_to_relevance) {
		matched_documents.push_back({ // @suppress("Invalid arguments")
			document_id,
			relevance,
			documents_.at(document_id).rating
		});
	}
	return matched_documents;
}

template<typename Predic>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, Predic predic) const {
	const Query query = ParseQuery(raw_query);
	std::vector<Document> result = FindAllDocuments(query, predic);
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
std::set<std::string> SearchServer::MakeSetOfStopWords(const StringContainer& container) {
	std::set<std::string> valid_stop_words;
	for (const std::string& str : container) {
		if (!str.empty()) {
			if (IsValidWord(str)) {
				valid_stop_words.insert(str);
			} else {
				throw std::invalid_argument("control character in stop-words"s);
			}
		}
	}
	return valid_stop_words;
}
