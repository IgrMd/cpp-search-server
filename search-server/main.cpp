//Sprint 3. Cpp Search server v0.3.1.
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double RELEVANCE_TRESHOLD = 1e-6;

struct Document {
	Document() = default;
	Document(int id, double relevance, int rating)
	: id(id), relevance(relevance), rating(rating)
	{
	}
    int id = 0;
    double relevance = 0;
    int rating = 0;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
    	: stop_words_(MakeSetOfStopWords(stop_words))
	{
    }

    explicit SearchServer(const string& stop_words)
    	: SearchServer(SplitIntoWords(stop_words))
    {
    }

    void SetStopWords(const string& text) {
    	const vector<string> stop_words = SplitIntoWords(text);
        for (const string& word : MakeSetOfStopWords(stop_words)) {
            stop_words_.insert(word);
        }
    }


    void AddDocument(int document_id, const string& document, DocumentStatus status,
                                   const vector<int>& ratings) {
    	if (document_id < 0) { //id must be greater than 0
    		throw invalid_argument("negative doc's id"s);
    	} else if (documents_.count(document_id)) { //new doc id must be new
    		throw invalid_argument("doc's id is already exists"s);
    	}
    	const vector<string> words = SplitIntoWordsNoStop(document);
    	for (const string& word : words) {
    		if (!IsValidWord(word)) {
    			throw invalid_argument("control character in document"s);
    		}
    	}
    	document_ids_.push_back(document_id);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
            DocumentData{
                ComputeAverageRating(ratings),
                status
            });
    }

    //Метод обрабатывает запрос, первый аргумент которого - строка, второй - функция-предикат
    template<typename Predic>
    vector<Document> FindTopDocuments(const string& raw_query, Predic predic) const {

    	const Query query = ParseQuery(raw_query);
    	IsValidQuery(raw_query, query);
    	vector<Document> result = FindAllDocuments(query, predic);
        sort(result.begin(), result.end(),
             [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < RELEVANCE_TRESHOLD) {
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

    //Метод обрабатывает запрос, состоящий из строки со статусом
    vector<Document> FindTopDocuments(const string& raw_query, const DocumentStatus filter_status) const {
    	return FindTopDocuments(raw_query,
    			[filter_status](int document_id, DocumentStatus status, int rating)
				{ return status == filter_status ;});
    }

    //Метод обрабатывает запрос, состоящий из строки
    vector<Document> FindTopDocuments(const string& raw_query) const {
    	return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    int GetDocumentId(int number) const {
		if (number < 0 || number >= static_cast<int>(documents_.size())) {
			throw out_of_range("doc's id is out of [0; documents count)"s);
		}
		return document_ids_[number];
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
    	const Query query = ParseQuery(raw_query);
    	IsValidQuery(raw_query, query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        tuple<vector<string>, DocumentStatus>result = {matched_words, documents_.at(document_id).status};
        return result;
    }

private:
    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    //структура данных документа (средний рейтинг, статус)
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;								//контейнер стоп-слов
    map<string, map<int, double>> word_to_document_freqs_;	//контейнер map<слово, map<id документа, inverse document frequency(IDF)>>
    map<int, DocumentData> documents_; 						//контейнер map<id документа, рейтинг-статус>
    vector<int> document_ids_; 								//контейнер id документов в порядек их обавления

    //Проверка слова на вхожение в перечень стоп-слов
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    //любой контейнер преобразуем в set<string> и проверяем на корректность содержания
    template <typename StringContainer>
    static set<string> MakeSetOfStopWords(const StringContainer& container) {
    	set<string> valid_stop_words;
    	for (const string& str : container) {
    		if (!str.empty()) {
    			IsValidWord(str)?
    				valid_stop_words.insert(str):
					throw invalid_argument("control character in stop-words"s);
    		}
    	}
    	return valid_stop_words;
    }

    //получение из строки контейнера отдельных слов
    static vector<string> SplitIntoWords(const string& text) {
        vector<string> words;
        string word;
        for (const char c : text) {
            if (c == ' ') {
                if (!word.empty()) {
                    words.push_back(word);
                    word.clear();
                }
            } else {
                word += c;
            }
        }
        if (!word.empty()) {
            words.push_back(word);
        }

        return words;
    }

    //получение из строки контейнера отдельных слов, исключая стоп-слова
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    //вычисление среднего рейтинга документов
    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        const int rating_sum = accumulate(ratings.begin(), ratings.end(),0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
            text,
            is_minus,
            IsStopWord(text)
        };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    //парсинг запроса на минус- и плюс-слова
    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
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

    //проверка запроса на "правильность"
    static void IsValidQuery(const string& raw_query, const Query& query) {
    	if (raw_query[raw_query.size()-1] == '-')
    		throw invalid_argument("no characters after \"-\" in query"s);

        for (const string& word : query.plus_words) {
        	if (!IsValidWord(word))
        		throw invalid_argument("control character in query plus-words"s);
        }

        for (const string& word : query.minus_words) {
        	if (!IsValidWord(word))
        		throw invalid_argument("control character in query minus-words"s);
        	else if  (word[0] == '-')
        		throw invalid_argument("double \"-\" in query minus-words"s);
        }
    }

    //проверка слова на "правильность"
    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template<typename Predic>
    vector<Document> FindAllDocuments(const Query& query, Predic predic) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
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

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
            });
        }
        return matched_documents;
    }
};


// ------------ Пример использования ----------------

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}

void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status,
                 const vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const exception& e) {
        cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const string& raw_query) {
    cout << "Результаты поиска по запросу: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const exception& e) {
        cout << "Ошибка поиска: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const string& query) {
    try {
        cout << "Матчинг документов по запросу: "s << query << endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const exception& e) {
        cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
    }
}

int main() {
    SearchServer search_server("и в на"s);

    AddDocument(search_server, 1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, -1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 3, "большой пёс скво\x12рец евгений"s, DocumentStatus::ACTUAL, {1, 3, 2});
    AddDocument(search_server, 4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, {1, 1, 1});

    FindTopDocuments(search_server, "пушистый -пёс"s);
    FindTopDocuments(search_server, "пушистый --кот"s);
    FindTopDocuments(search_server, "пушистый -"s);

    MatchDocuments(search_server, "пушистый пёс"s);
    MatchDocuments(search_server, "модный -кот"s);
    MatchDocuments(search_server, "модный --пёс"s);
    MatchDocuments(search_server, "пушистый - хвост"s);
}
