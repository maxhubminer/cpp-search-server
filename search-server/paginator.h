#pragma once

#include <iostream>
#include <vector>

template <typename Iterator>
struct IteratorRange {
    Iterator it1, it2;

    Iterator begin() const {
        return it1;
    }

    Iterator end() const {
        return it2;
    }

    size_t size() const {
        return distance(it1, it2);
    }
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& os, const IteratorRange<Iterator>& range) {

    using namespace std::string_literals;
    
    for (auto it = range.begin(); it != range.end(); ++it) {
        os << "{ document_id = "s << (*it).id << ", relevance = "s << (*it).relevance << ", rating = "s << (*it).rating << " }"s;
    }

    return os;
}

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator range_begin, Iterator range_end, size_t page_size) {
        Iterator page_start = range_begin;
        int i = 1;
        for (Iterator it = range_begin; it != range_end; ++it) {
            Iterator next = it + 1;
            if (i % page_size == 0 || next == range_end) {
                pages_.push_back({ page_start, next });
                page_start = next;
            }
            i++;
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }


private:
    std::vector<IteratorRange<Iterator>> pages_;
};