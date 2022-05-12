#pragma once
#include <cassert>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
	IteratorRange(Iterator begin, Iterator end)
		: begin_(begin),
		  end_(end) {
	}

	Iterator begin() const {
		return begin_;
	}
	Iterator end() const {
		return end_;
	}
	size_t size() const {
		return distance(begin_, end_);
	}
private:
	Iterator begin_;
	Iterator end_;
};

template <typename Iterator>
class Paginator {
public:
	explicit Paginator(const Iterator begin, const Iterator end, int page_size);

	auto begin() const {
		return pages_.begin();
	}
	auto end() const {
		return pages_.end();
	}

private:
	std::vector<IteratorRange<Iterator>> pages_;
};


template <typename Iterator>
Paginator<Iterator>::Paginator(const Iterator begin, const Iterator end, int page_size) {
	Iterator current_page_begin = begin;
	Iterator current_page_end = next(current_page_begin, page_size);
	assert(end >= begin && page_size > 0);
	while (distance(current_page_begin, end) > page_size) {
		pages_.push_back({current_page_begin, current_page_end});
		current_page_begin = current_page_end;
		current_page_end = next(current_page_begin, page_size);
	}
	pages_.push_back({current_page_begin, end});
}
