//Sprint 5. Cpp Search server v0.5.1.
#include <iostream>
#include <vector>

#include "document.h"
#include "paginator.h"
#include "read_input_functions.h"
#include "remove_duplicates.h"
#include "request_queue.h"
#include "search_server.h"
#include "test_example_functions.h"

//using namespace std;
using  std::string_literals::operator ""s;

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

void SomeTests() {
	TestIterator(); // @suppress("Invalid arguments")
	TestGetWordFrequencies(); // @suppress("Invalid arguments")
	TestRemoveDocument(); // @suppress("Invalid arguments")
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
				 const std::vector<int>& ratings) {
	try {
		search_server.AddDocument(document_id, document, status, ratings);
	} catch (const std::exception& e) {
		std::cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << std::endl;
	}
}

int main() {
	SomeTests();
	SearchServer search_server("and with"s);

	AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
	AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

	// дубликат документа 2, будет удалён
	AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

	// отличие только в стоп-словах, считаем дубликатом
	AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});

	// множество слов такое же, считаем дубликатом документа 1
	AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

	// добавились новые слова, дубликатом не является
	AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

	// множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
	AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});

	// есть не все слова, не является дубликатом
	AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});

	// слова из разных документов, не является дубликатом
	AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

	std::cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
	RemoveDuplicates(search_server);
	std::cout << "After duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
	return 0;
}

