#pragma once

#include <algorithm>
#include <map>
#include <mutex>
#include <vector>


using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
	static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

	explicit ConcurrentMap(size_t bucket_count)
		: bucket_count_(bucket_count), buckets_(std::vector<Bucket>{bucket_count}) {}

	struct Bucket {
		std::map<Key, Value> map;
		std::mutex map_mutex;
	};
	struct Access {
		Access(Bucket& bucket, const Key& key)
			: guard(std::lock_guard(bucket.map_mutex)), ref_to_value(bucket.map[key]) {}
		std::lock_guard<std::mutex> guard;
		Value& ref_to_value;
	};
	Access operator[](const Key& key) {
		const size_t index = static_cast<uint64_t>(key) % bucket_count_;
		return { buckets_[index], key };
	}
	size_t Size() const noexcept {
		 size_t size = 0;
		 std::for_each(
			buckets_.begin(), buckets_.end(), 
			[&size] (const Bucket& b) {return size += b.map.size(); }
		);
		 return size;
	}
	std::map<Key, Value> BuildOrdinaryMap() {
		std::map<Key, Value> result;
		for (auto& bucket : buckets_) {
			std::lock_guard g(bucket.map_mutex);
			result.insert(bucket.map.begin(), bucket.map.end());
		}
		return result;
	}

private:
	size_t bucket_count_;
	std::vector<Bucket> buckets_;
};