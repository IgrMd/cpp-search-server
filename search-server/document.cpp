#include <iostream>
#include "document.h"

using namespace std;

Document::Document(int id, double relevance, int rating)
	: id(id), relevance(relevance), rating(rating) {
}

void PrintDocument(const Document& document) {
	cout << "{ "s
		 << "document_id = "s << document.id << ", "s
		 << "relevance = "s << document.relevance << ", "s
		 << "rating = "s << document.rating << " }"s << std::endl;
}

void PrintDocumentNoEndl(const Document& document) {
	cout << "{ "s
		 << "document_id = "s << document.id << ", "s
		 << "relevance = "s << document.relevance << ", "s
		 << "rating = "s << document.rating << " }"s;
}
