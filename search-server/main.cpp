#include <algorithm>
//#include <cassert> //do not need anymore
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <vector>
#include <utility>

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double RELEVANCE_TRESHOLD = 1e-6;

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
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


    //Метод обрабатывает запрос, состоящий из строки или из строки со статусом
    vector<Document> FindTopDocuments(const string& raw_query, const DocumentStatus filter_status = DocumentStatus::ACTUAL) const {
    	return FindTopDocuments(raw_query,
    			[filter_status](int document_id =0, DocumentStatus status = DocumentStatus::ACTUAL, int rating = 0)
				{ return status == filter_status ;});
    }

    //Метод обрабатывает запрос, первый аргумент которого - строка, второй - функция-предикат
    template<typename Predic>
    vector<Document> FindTopDocuments(const string& raw_query, Predic predic) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predic);
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < RELEVANCE_TRESHOLD) {
                    return lhs.rating > rhs.rating;
                } else {
                    return lhs.relevance > rhs.relevance;
                }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }


    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
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
        return {matched_words, documents_.at(document_id).status};
    }

private:

    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

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

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

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
        // Word shouldn't be empty
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
/*   Подставьте сюда вашу реализацию макросов
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST*/
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}


#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

/*
Разместите код остальных тестов здесь
*/
// Тест проверяет, что документы, содержащие минус-слова поискового запроса, не включются в результаты поиска.
void TestExcludeDocsWithMinusWords() {
	SearchServer server;
    const int doc0_id = 777;
    const string content0 = "happiness for everyone for free and let no one leave offended"s;
    const vector<int> ratings0 = {1, 2, 3};
    server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
    const int doc1_id = 13;
    const string content1 = "funny office joke"s;
    const vector<int> ratings1 = {5, 6, 7};
    server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
	const auto found_docs = server.FindTopDocuments("-leave office"s);
	ASSERT_EQUAL(found_docs.size(), 1u);
    const Document& doc = found_docs[0];
    ASSERT_EQUAL_HINT(doc.id, doc1_id, "Doc 13 don't contains minus word"s);
}

// Тест проверяет, что добавленный документ находится по поисковому запросу, который содержит слова из документа.
void TestFindAddedDocumentByWord() {
    const int doc0_id = 777;
    const string content0 = "happiness for everyone for free and let no one leave offended"s;
    const vector<int> ratings0 = {1, 2, 3};

    SearchServer server;
    {	//Сервер пуст, ничего не найдено
        const auto found_docs = server.FindTopDocuments("some words"s);
        ASSERT_EQUAL_HINT(found_docs.size(), size_t(0), "Server is empty, no match"s);
    }

    server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);

    {	//Искомых слов в сервере не найдено
    	const auto found_docs = server.FindTopDocuments("some words"s);
    	ASSERT_EQUAL_HINT(found_docs.size(), size_t(0), "No some worsd in doc 777"s);
    }

    {	//Запрос содержит слова из добавленных документов
    	const auto found_docs = server.FindTopDocuments("leave office"s);
    	ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc0_id);
    }
    const int doc1_id = 13;
    const string content1 = "funny office joke"s;
    const vector<int> ratings1 = {5, 6, 7};
    server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
    {	//Запрос содержит слова из добавленных документов
    	const auto found_docs = server.FindTopDocuments("leave office"s);
    	ASSERT_EQUAL(found_docs.size(), 2u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL_HINT(doc0.id, doc1_id, "First founded doc is 13"s); //Первым вернулся 2й документ
        const Document& doc1 = found_docs[1];
        ASSERT_EQUAL_HINT(doc1.id, doc0_id, "First founded doc is 13"s); //Вторым вернулся первый документ
    }
}
// Тест проверяет матчинг слов
void TestMatchingWordsAndMinusWords() {
	SearchServer server;
    const int doc0_id = 777;
    const string content0 = "happiness for everyone for free and let no one leave offended"s;
    const vector<int> ratings0 = {1, 2, 3};
    server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
    {
    	const auto [matched_words, doc_status] = server.MatchDocument("let happiness leave for ever"s, doc0_id);
    	ASSERT_EQUAL(matched_words[0], "for"s);
    	ASSERT_EQUAL(matched_words[1], "happiness"s);
    	ASSERT_EQUAL(matched_words[2], "leave"s);
    	ASSERT_EQUAL(matched_words[3], "let"s);
    }
    {
    	const auto [matched_words, doc_status] = server.MatchDocument("let happiness -leave for ever"s, doc0_id);
    	ASSERT_HINT(matched_words.empty(), "Any matched minus word - no matched doc"s);
    }
}

// Тест проверяет сортировку по релевантности
void TestRelevanceSorting() {
	SearchServer server;
    const int doc0_id = 777;
    const string content0 = "happiness for everyone for free and let no one leave offended"s;
    const vector<int> ratings0 = {1, 2, 3};
    server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
    const int doc1_id = 13;
    const string content1 = "funny office joke"s;
    const vector<int> ratings1 = {5, 6, 7};
    server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
	const auto found_docs = server.FindTopDocuments("let happiness leave the office"s);
	ASSERT(found_docs[0].relevance > found_docs[1].relevance);
}

// Тест проверяет вычисление рейтинга документов
void TestMediumRatingCalculation() {
	SearchServer server;
    const int doc0_id = 777;
    const string content0 = "happiness for everyone for free and let no one leave offended"s;
    const vector<int> ratings0 = {1, 2, 3};
    server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
    const int doc1_id = 13;
    const string content1 = "funny office joke must connect the people around"s;
    const vector<int> ratings1 = {-865, 4, 5, 6, 100500};
    server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
	const auto found_docs = server.FindTopDocuments("let happiness leave the office"s);
	ASSERT_EQUAL(found_docs[0].rating, 2);
	ASSERT_EQUAL(found_docs[1].rating, 19930);
}

// Тест проверяет фильтрацию результатов поиска с использованием предиката, задаваемого пользователем
void TestFiltrationWithUserPridicate() {
	SearchServer server;
    const int doc0_id = 776;
    const string content0 = "happiness for everyone for free and let no one leave offended"s;
    const vector<int> ratings0 = {1, 2, 3};
    server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
    const int doc1_id = 13;
    const string content1 = "funny office joke"s;
    const vector<int> ratings1 = {5, 6, 7};
    server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
	const auto found_docs = server.FindTopDocuments("let happiness leave the office"s,
			[](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
	ASSERT_EQUAL(found_docs[0].id, 776);
}

// Тест проверяет поиск документов, имеющих заданный статус
void TestFiltrationWitStatus() {
	SearchServer server;
    const int doc0_id = 777;
    const string content0 = "happiness for everyone for free and let no one leave offended"s;
    const vector<int> ratings0 = {1, 2, 3};
    server.AddDocument(doc0_id, content0, DocumentStatus::REMOVED, ratings0);
    const int doc1_id = 13;
    const string content1 = "funny office joke"s;
    const vector<int> ratings1 = {5, 6, 7};
    server.AddDocument(doc1_id, content1, DocumentStatus::BANNED, ratings1);
	const auto found_docs = server.FindTopDocuments("let happiness leave the office"s, DocumentStatus::BANNED);
	ASSERT_EQUAL_HINT(found_docs[0].id, 13, "Only 13th doc is BANNED"s);
}

// Тест проверяет корректное вычисление релевантности найденных документов
void TestRelevanceCalculation() {
	SearchServer server;
    const int doc0_id = 777;
    const string content0 = "happiness for everyone for free and let no one leave offended"s;
    const vector<int> ratings0 = {1, 2, 3};
    server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
    const int doc1_id = 13;
    const string content1 = "funny office joke"s;
    const vector<int> ratings1 = {-865, 4, 5, 6, 100500};
    server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
	const auto found_docs = server.FindTopDocuments("let happiness leave the office"s);
	ASSERT_HINT(abs(found_docs[0].relevance - 0.231049) < RELEVANCE_TRESHOLD, "Don't forget threshold"s);
	ASSERT_HINT(abs(found_docs[1].relevance - 0.18904) < RELEVANCE_TRESHOLD, "Don't forget threshold"s);
}

template <typename Function>
void RunTestImpl(Function function, const string& function_name) {
    function();
    cerr << function_name << " OK"s << endl;
}

#define RUN_TEST(func)  RunTestImpl(func, #func)

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    // Не забудьте вызывать остальные тесты здесь
    RUN_TEST(TestFindAddedDocumentByWord);
    RUN_TEST(TestExcludeDocsWithMinusWords);
    RUN_TEST(TestMatchingWordsAndMinusWords);
    RUN_TEST(TestRelevanceSorting);
    RUN_TEST(TestMediumRatingCalculation);
    RUN_TEST(TestFiltrationWithUserPridicate);
    RUN_TEST(TestFiltrationWitStatus);
    RUN_TEST(TestRelevanceCalculation);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}
