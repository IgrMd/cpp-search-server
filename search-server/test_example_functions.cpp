#include "test_example_functions.h"

using  std::string_literals::operator ""s;

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
	SearchServer search_server("and in at"s);
	search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
	{
		std::map<std::string, double> test_map = search_server.GetWordFrequencies(5);
		assert(test_map.empty());
	}

	{
		std::map<std::string, double> test_map = search_server.GetWordFrequencies(1);
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
		std::map<std::string, double> test_map = search_server.GetWordFrequencies(777);
		assert(test_map.empty());
	}

	{
		assert(find(search_server.begin(), search_server.end(), 777) == search_server.end());
	}

	{
		assert(search_server.FindTopDocuments("UNIC_TEXT"s).empty());
	}

	std::cerr << "TestRemoveDocument OK"s << std::endl;
}


