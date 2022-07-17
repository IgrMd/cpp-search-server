#include "document.h"

using  std::string_literals::operator ""s;

Document::Document(int id, double relevance, int rating)
	: id(id), relevance(relevance), rating(rating) {
}

void PrintDocument(const Document& document) {
	std::cout << "{ "s
		 << "document_id = "s << document.id << ", "s
		 << "relevance = "s << document.relevance << ", "s
		 << "rating = "s << document.rating << " }"s << std::endl;
}

void PrintDocumentNoEndl(const Document& document) {
	std::cout << "{ "s
		 << "document_id = "s << document.id << ", "s
		 << "relevance = "s << document.relevance << ", "s
		 << "rating = "s << document.rating << " }"s;
}

void PrintDocumentStringstream(const Document& document, std::stringstream& ss) {
	ss << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating << " }"s << std::endl;
}