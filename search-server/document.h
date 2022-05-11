#pragma once


struct Document {
	Document() = default;
	Document(int id, double relevance, int rating);
	int id = 0;
	double relevance = 0;
	int rating = 0;
};

enum class DocumentStatus { ACTUAL, IRRELEVANT, BANNED,	REMOVED };

void PrintDocument(const Document& document);

void PrintDocumentNoEndl(const Document& document);
