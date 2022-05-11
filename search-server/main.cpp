//Sprint 4. Cpp Search server v0.4.1.

#include <iostream>
#include <vector>
#include "document.h"
#include "paginator.h"
#include "read_input_functions.h"
#include "request_queue.h"
#include "search_server.h"

using namespace std;



template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
	return Paginator(begin(c), end(c), page_size); // @suppress("Type cannot be resolved")
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& page) {
	for (auto it = page.begin(); it != page.end(); ++ it) {
		PrintDocumentNoEndl(*it);
	}
	return out;
}

std::ostream& operator<<(std::ostream& out, const std::vector<Document>& docs) {
	for (auto it = docs.begin(); it != docs.end(); ++ it) {
		PrintDocument(*it); // @suppress("Invalid arguments")
	}
	return out;
}

int main() {
	SearchServer search_server("and in at"s);
	RequestQueue request_queue(search_server);

	search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
	search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
	search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
	search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
	search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});

	// 1439 запросов с нулевым результатом
	for (int i = 0; i < 1439; ++i) {
		request_queue.AddFindRequest("empty request"s);
	}
	// все еще 1439 запросов с нулевым результатом
	request_queue.AddFindRequest("curly dog"s);
	// новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
	request_queue.AddFindRequest("big collar"s);
	// первый запрос удален, 1437 запросов с нулевым результатом
	request_queue.AddFindRequest("sparrow"s);
	std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;

	SearchServer search_server1("и в на"s);

	search_server1.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
	search_server1.AddDocument(2, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2, 3});
	search_server1.AddDocument(3, "большой кот модный ошейник "s, DocumentStatus::ACTUAL, {1, 2, 8});
	search_server1.AddDocument(4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, {1, 3, 2});
	search_server1.AddDocument(5, "большой пёс скворец василий"s, DocumentStatus::ACTUAL, {1, 1, 1});
	search_server1.AddDocument(6, "пушистый кот пушистый хвост пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
	search_server1.AddDocument(7, "пушистый пёс и модный ошейник пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2, 3});
	search_server1.AddDocument(8, "большой кот модный ошейник большой кот модный ошейник"s, DocumentStatus::ACTUAL, {1, 2, 8});
	search_server1.AddDocument(9, "большой пёс скворец евгений большой пёс скворец евгений"s, DocumentStatus::ACTUAL, {1, 3, 2});
	search_server1.AddDocument(10, "большой пёс скворец василий большой пёс скворец василий"s, DocumentStatus::ACTUAL, {1, 1, 1});

	const auto search_results = search_server1.FindTopDocuments("пушистый пёс"s);
	int page_size = 2;
	const auto pages = Paginate(search_results, page_size); // @suppress("Invalid arguments")

	// Выводим найденные документы по страницам
	for (auto doc : search_results) {
		PrintDocument(doc);
	}

	for (auto page = pages.begin(); page != pages.end(); ++page) { // @suppress("Method cannot be resolved")
		std::cout << *page << std::endl; // @suppress("Invalid overload")
		std::cout << "Разрыв страницы"s << std::endl;
	}

	return 0;
}
