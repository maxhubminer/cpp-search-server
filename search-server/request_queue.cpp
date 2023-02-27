#include "request_queue.h"

using namespace std;

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    vector<Document> result = search_server_.FindTopDocuments(raw_query, status);
    AddRequest({ result.size() });
    return result;
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    vector<Document> result = search_server_.FindTopDocuments(raw_query);
    AddRequest({ result.size() });
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return no_result_requests_;
}

void RequestQueue::AddRequest(const QueryResult& req) {
    if (req.doc_amount == 0) {
        ++no_result_requests_;
    }
    requests_.push_back(req);
    if (requests_.size() > RequestQueue::min_in_day_) {
        if (requests_.front().doc_amount == 0) {
            --no_result_requests_;
        }
        requests_.pop_front();
    }
}