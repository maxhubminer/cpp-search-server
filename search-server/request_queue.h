#pragma once

#include "document.h"
#include "search_server.h"

#include <string>
#include <vector>
#include <deque>

class RequestQueue {
public:
	explicit RequestQueue(const SearchServer& search_server) : search_server_(search_server) {

	}

	template <typename DocumentPredicate>
	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

	std::vector<Document> AddFindRequest(const std::string& raw_query);

	int GetNoResultRequests() const;

private:
	struct QueryResult {
		size_t docAmount;
	};
	std::deque<QueryResult> requests_;
	const static int min_in_day_ = 1440;
	// возможно, здесь вам понадобится что-то ещё
	const SearchServer& search_server_;
	int no_result_requests_ = 0;

	void AddRequest(const QueryResult& req);
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
	std::vector<Document> result = search_server_.FindTopDocuments(raw_query, document_predicate);

	AddRequest({ result.size() });

	return result;
}