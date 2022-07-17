#include "test_example_functions.h"
using namespace std;



void TestIterator() {
	SearchServer search_server("and in at"s);
	search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
	search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
	search_server.AddDocument(777, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
	search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
	search_server.AddDocument(666, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});

	assert(*search_server.begin() == 1);
	assert(*--search_server.end() == 666);
	assert(std::distance(search_server.begin(), search_server.end()) == 5);
	std::ostringstream str;
	for (const int i : search_server) {
		str << i;
	}
	assert(str.str() == "127774666"s);
	std::cerr << "TestIterators OK"s << std::endl;
}

void TestGetWordFrequencies() {
	std::string s("and in at"s);
	std::string_view sv(s);
	SearchServer search_server(sv);
	search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
	{
		std::map<std::string_view, double> test_map = search_server.GetWordFrequencies(5);
		assert(test_map.empty());
	}

	{
		std::map<std::string_view, double> test_map = search_server.GetWordFrequencies(1);
		assert(test_map.at("cat"s) == 1./4);
		assert(test_map.at("curly"s) == 2./4);
		assert(test_map.at("tail"s) == 1./4);

	}
	std::cerr << "TestGetWordFrequencies OK"s << std::endl;
}

void TestRemoveDocument() {
	SearchServer search_server("and in at"s);
	search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
	search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
	search_server.AddDocument(777, "big cat fancy collar UNIC_TEXT"s, DocumentStatus::ACTUAL, {1, 2, 8});
	search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
	search_server.AddDocument(666, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});
	{
		assert(!search_server.FindTopDocuments("UNIC_TEXT"s).empty());
	}
	search_server.RemoveDocument(777);
	{
		std::map<std::string_view, double> test_map = search_server.GetWordFrequencies(777);
		assert(test_map.empty());
	}

	{
		assert(find(search_server.begin(), search_server.end(), 777) == search_server.end());
	}

	{
		assert(search_server.FindTopDocuments("UNIC_TEXT"s).empty());
	}

	std::cout << "TestRemoveDocument OK"s << std::endl;
}

/*   Подставьте сюда вашу реализацию макросов
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST*/
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
	const std::string& func, unsigned line, const std::string& hint) {
	using namespace std;
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

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
	const std::string& hint) {
	using namespace std;
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

// -------- Начало модульных тестов поисковой системы (из 2го спринта) ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
	using namespace std;
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = { 1, 2, 3 };
	{
		SearchServer server(""s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("in"s);
		ASSERT_EQUAL(found_docs.size(), 1u);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}

	{
		SearchServer server(""s);
		server.SetStopWords("in the"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
	}
}

// Тест проверяет, что документы, содержащие минус-слова поискового запроса, не включются в результаты поиска.
void TestExcludeDocsWithMinusWords() {
	using namespace std;
	SearchServer server(""s);
	const int doc0_id = 777;
	const string content0 = "happiness for everyone for free and let no one leave offended"s;
	const vector<int> ratings0 = { 1, 2, 3 };
	server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
	const int doc1_id = 13;
	const string content1 = "funny office joke"s;
	const vector<int> ratings1 = { 5, 6, 7 };
	server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
	const auto found_docs = server.FindTopDocuments("-leave office"s);
	ASSERT_EQUAL(found_docs.size(), 1u);
	const Document& doc = found_docs[0];
	ASSERT_EQUAL_HINT(doc.id, doc1_id, "Doc 13 don't contains minus word"s);
}

// Тест проверяет, что добавленный документ находится по поисковому запросу, который содержит слова из документа.
void TestFindAddedDocumentByWord() {
	using namespace std;
	const int doc0_id = 777;
	const string content0 = "happiness for everyone for free and let no one leave offended"s;
	const vector<int> ratings0 = { 1, 2, 3 };

	SearchServer server(""s);
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
	const vector<int> ratings1 = { 5, 6, 7 };
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
	using namespace std;
	SearchServer server(""s);
	const int doc0_id = 777;
	const string content0 = "happiness for everyone for free and let no one leave offended"s;
	const vector<int> ratings0 = { 1, 2, 3 };
	server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
	{
		const auto& [matched_words, doc_status] = server.MatchDocument("let happiness leave for ever"s, doc0_id);
		ASSERT_EQUAL(matched_words[0], "for"sv); // @suppress("Invalid arguments")
		ASSERT_EQUAL(matched_words[1], "happiness"sv); // @suppress("Invalid arguments")
		ASSERT_EQUAL(matched_words[2], "leave"sv); // @suppress("Invalid arguments")
		ASSERT_EQUAL(matched_words[3], "let"sv); // @suppress("Invalid arguments")
	}
	{
		const auto [matched_words, doc_status] = server.MatchDocument("let happiness -leave for ever"s, doc0_id);
		ASSERT_HINT(matched_words.empty(), "Any matched minus word - no matched doc"s); // @suppress("Method cannot be resolved")
	}
}

// Тест проверяет сортировку по релевантности
void TestRelevanceSorting() {
	using namespace std;
	SearchServer server(""s);
	const int doc0_id = 777;
	const string content0 = "happiness for everyone for free and let no one leave offended"s;
	const vector<int> ratings0 = { 1, 2, 3 };
	server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
	const int doc1_id = 13;
	const string content1 = "funny office joke"s;
	const vector<int> ratings1 = { 5, 6, 7 };
	server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
	const auto found_docs = server.FindTopDocuments("let happiness leave the office"s);
	ASSERT(found_docs[0].relevance > found_docs[1].relevance);
}

// Тест проверяет вычисление рейтинга документов
void TestMediumRatingCalculation() {
	using namespace std;
	SearchServer server(""s);
	const int doc0_id = 777;
	const string content0 = "happiness for everyone for free and let no one leave offended"s;
	const vector<int> ratings0 = { 1, 2, 3 };
	server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
	const int doc1_id = 13;
	const string content1 = "funny office joke must connect the people around"s;
	const vector<int> ratings1 = { -865, 4, 5, 6, 100500 };
	server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
	const auto found_docs = server.FindTopDocuments("let happiness leave the office"s);
	ASSERT_EQUAL(found_docs[0].rating, 2);
	ASSERT_EQUAL(found_docs[1].rating, 19930);
}

// Тест проверяет фильтрацию результатов поиска с использованием предиката, задаваемого пользователем
void TestFiltrationWithUserPredicate() {
	using namespace std;
	SearchServer server(""s);
	const int doc0_id = 776;
	const string content0 = "happiness for everyone for free and let no one leave offended"s;
	const vector<int> ratings0 = { 1, 2, 3 };
	server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
	const int doc1_id = 13;
	const string content1 = "funny office joke"s;
	const vector<int> ratings1 = { 5, 6, 7 };
	server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
	const auto found_docs = server.FindTopDocuments("let happiness leave the office"s,
		[](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
	ASSERT_EQUAL(found_docs[0].id, 776);
}

// Тест проверяет поиск документов, имеющих заданный статус
void TestFiltrationWitStatus() {
	using namespace std;
	SearchServer server(""s);
	const int doc0_id = 777;
	const string content0 = "happiness for everyone for free and let no one leave offended"s;
	const vector<int> ratings0 = { 1, 2, 3 };
	server.AddDocument(doc0_id, content0, DocumentStatus::REMOVED, ratings0);
	const int doc1_id = 13;
	const string content1 = "funny office joke"s;
	const vector<int> ratings1 = { 5, 6, 7 };
	server.AddDocument(doc1_id, content1, DocumentStatus::BANNED, ratings1);
	const auto found_docs1 = server.FindTopDocuments("let happiness leave the office"s, DocumentStatus::BANNED);
	ASSERT_EQUAL_HINT(found_docs1[0].id, 13, "Only 13th doc is BANNED"s);
	const auto found_docs2 = server.FindTopDocuments("let happiness leave the office"s, DocumentStatus::REMOVED);
	ASSERT_EQUAL_HINT(found_docs2[0].id, 777, "Only 777th doc is REMOVED"s);
}

// Тест проверяет корректное вычисление релевантности найденных документов
void TestRelevanceCalculation() {
	using namespace std;
	SearchServer server(""s);
	const int doc0_id = 777;
	const string content0 = "happiness for everyone for free and let no one leave offended"s;
	const vector<int> ratings0 = { 1, 2, 3 };
	server.AddDocument(doc0_id, content0, DocumentStatus::ACTUAL, ratings0);
	const int doc1_id = 13;
	const string content1 = "funny office joke"s;
	const vector<int> ratings1 = { -865, 4, 5, 6, 100500 };
	server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, ratings1);
	const auto found_docs = server.FindTopDocuments("let happiness leave the office"s);
	ASSERT_HINT(abs(found_docs[0].relevance - 0.231049) < RELEVANCE_TRESHOLD, "Don't forget threshold"s);
	ASSERT_HINT(abs(found_docs[1].relevance - 0.18904) < RELEVANCE_TRESHOLD, "Don't forget threshold"s);
}

template <typename Function>
void RunTestImpl(Function function, const std::string& function_name) {
	using namespace std;
	function();
	cerr << function_name << " OK"s << endl;
}

#define RUN_TEST(func)  RunTestImpl(func, #func)

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(TestFindAddedDocumentByWord);
	RUN_TEST(TestExcludeDocsWithMinusWords);
	RUN_TEST(TestMatchingWordsAndMinusWords);
	RUN_TEST(TestRelevanceSorting);
	RUN_TEST(TestMediumRatingCalculation);
	RUN_TEST(TestFiltrationWithUserPredicate);
	RUN_TEST(TestFiltrationWitStatus);
	RUN_TEST(TestRelevanceCalculation);
}

void TestMachDocumentsPar() {
using namespace std;
	SearchServer search_server("and with"s);

	int id = 1;
	for (
		const string& text : {
			"funny pet and nasty rat"s,
			"funny pet with curly hair"s,
			"funny curly pet and not very nasty rat"s,
			"curly and funny pet curly with rat and rat and rat"s,
			"nasty rat with curly hair"s,
		}
		) {
		search_server.AddDocument(id, text, DocumentStatus::ACTUAL, { 1, 2 });
		++id;
	}

	const string query = "curly and funny -not"s;

	{
		const auto [words, status] = search_server.MatchDocument(query, 1);
		assert(words.size() == 1);
	}

	{
		const auto [words, status] = search_server.MatchDocument(execution::seq, query, 2);
		assert(words.size() == 2);
	}

	{
		const auto [words, status] = search_server.MatchDocument(execution::par, query, 3);
		assert(words.size() == 0);
	}

	{
		const auto [words, status] = search_server.MatchDocument(execution::par, query, 4);
		assert(words.size() == 2);
	}
	cout << "TestMachDocumentsPar OK"s << endl;
}

void TesdRemoveDocumentPar() {
	SearchServer search_server("and with"s);

	int id = 0;
	for (
		const string& text : {
			"funny pet and nasty rat"s,
			"funny pet with curly hair"s,
			"funny pet and not very nasty rat"s,
			"pet with rat and rat and rat"s,
			"nasty rat with curly hair"s,
		}
		) {
		search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
	}

	const string query = "curly and funny"s;

	assert(search_server.GetDocumentCount() == 5 && search_server.FindTopDocuments(query).size()==4);
	// однопоточная версия
	search_server.RemoveDocument(5);
	assert(search_server.GetDocumentCount() == 4 && search_server.FindTopDocuments(query).size() == 3);
	// однопоточная версия
	search_server.RemoveDocument(execution::seq, 1);
	assert(search_server.GetDocumentCount() == 3 && search_server.FindTopDocuments(query).size() == 2);
	// многопоточная версия
	search_server.RemoveDocument(execution::par, 2);
	assert(search_server.GetDocumentCount() == 2 && search_server.FindTopDocuments(query).size() == 1);
	cout << "TesdRemoveDocumentPar OK"s << endl;
}

void TestFindTopDocsPar() {
	using namespace std;
	SearchServer search_server("and with"s);

	int id = 0;
	for (
		const string& text : {
			"white cat and yellow hat"s,
			"curly cat curly tail"s,
			"nasty dog with big eyes"s,
			"nasty pigeon john"s,
		}
		) {
		search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
	}
	// последовательная версия
	{
		stringstream ss;
		ss << "{ document_id = 2, relevance = 0.866434, rating = 1 }\n";
		ss << "{ document_id = 4, relevance = 0.231049, rating = 1 }\n";
		ss << "{ document_id = 1, relevance = 0.173287, rating = 1 }\n";
		ss << "{ document_id = 3, relevance = 0.173287, rating = 1 }\n";
		stringstream ss1;
		for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
			PrintDocumentStringstream(document, ss1);
		}
		assert(ss.str() == ss1.str());
	}
	// последовательная версия
	{
		stringstream ss;
		stringstream ss1;
		for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
			PrintDocumentStringstream(document, ss1);
		}
		assert(ss.str() == ss1.str());
	}
	// параллельная версия
	{
		stringstream ss;
		ss << "{ document_id = 2, relevance = 0.866434, rating = 1 }\n";
		ss << "{ document_id = 4, relevance = 0.231049, rating = 1 }\n";
		stringstream ss1;
		for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
			PrintDocumentStringstream(document, ss1);
		}
		assert(ss.str() == ss1.str());
	}
	cout << "TestFindTopDocsPar OK"s << endl;
}