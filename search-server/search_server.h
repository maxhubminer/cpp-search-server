#pragma once

#include "string_processing.h"
#include "document.h"
#include "log_duration.h"
#include "concurrent_map.h"

#include <vector>
#include <string>
#include <string_view>
#include <tuple>
#include <set>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <list>
#include <execution>
#include <deque>
#include <type_traits>

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words);

	explicit SearchServer(const std::string &stop_words_text);
	explicit SearchServer(std::string_view stop_words_text);

	void AddDocument(int document_id, std::string_view document, DocumentStatus status,
		const std::vector<int>& ratings);

	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(const std::string_view raw_query,
		DocumentPredicate document_predicate) const;

	std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

	std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

	template <typename DocumentPredicate, typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query,
		DocumentPredicate document_predicate) const;

	template <typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentStatus status) const;
	template <typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query) const;

	int GetDocumentCount() const;

	std::set<int>::iterator begin();

	std::set<int>::iterator end();

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const;

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
		const std::execution::sequenced_policy& policy, const std::string_view raw_query, int document_id) const;
	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
		const std::execution::parallel_policy& policy, const std::string_view raw_query, int document_id) const;

	const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

	void RemoveDocument(int document_id);

	template<typename ExecutionPolicy>
	void RemoveDocument(ExecutionPolicy&& policy, int document_id);

private:
	struct DocumentData {
		int rating;
		DocumentStatus status;
		std::map<std::string_view, double> words_freq;
	};
	const std::set<std::string, std::less<>> stop_words_;
	std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
	std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;    
	std::map<int, DocumentData> documents_;
	std::set<int> document_ids_;

	std::deque<std::string> string_storage;

	bool IsStopWord(std::string_view word) const;

	static bool IsValidWord(std::string_view word);

	std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

	static int ComputeAverageRating(const std::vector<int>& ratings);

	struct QueryWord {
		std::string_view data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(std::string_view text) const;

	struct Query {
		std::vector<std::string_view> plus_words;
		std::vector<std::string_view> minus_words;
	};

	Query ParseQueryCore(const std::string_view text) const;
	Query ParseQuery(const std::string_view text) const;

	// Existence required
	double ComputeWordInverseDocumentFreq(std::string_view word) const;

	template <typename DocumentPredicate, typename ExecutionPolicy>
	std::vector<Document> FindAllDocuments(const ExecutionPolicy& policy, const Query& query,
		DocumentPredicate document_predicate) const;

};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
	: stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
	using namespace std::string_literals;
	if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
		throw std::invalid_argument("Some of stop words are invalid"s);
	}
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(const ExecutionPolicy& policy, const Query& query,
	DocumentPredicate document_predicate) const {
	
	const int thread_count = 8;
	ConcurrentMap<int, double> document_to_relevance(thread_count);
	
	std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [&](const std::string_view word) {
		if (word_to_document_freqs_.count(word) == 0) {
			return;
		}
		const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
		for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
			const auto& document_data = documents_.at(document_id);
			if (document_predicate(document_id, document_data.status, document_data.rating)) {
				document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
			}
		}
	});	

	std::for_each(policy, query.minus_words.begin(), query.minus_words.end(), [&](const std::string_view word) {
		if (word_to_document_freqs_.count(word) == 0) {
			return;
		}
		for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
			document_to_relevance.Erase(document_id);
		}
	});

	std::vector<Document> matched_documents;
	for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
		matched_documents.push_back(
			{ document_id, relevance, documents_.at(document_id).rating });
	}
	return matched_documents;
}

template<typename ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id)
{
	const auto& words_map = document_to_word_freqs_[document_id];

	std::vector<const std::string*> words_to_delete_(words_map.size());
	std::transform(policy, words_map.cbegin(), words_map.cend(), words_to_delete_.begin(), [&](const auto& pair) {
		return &pair.first;
		});

	std::for_each(policy, words_to_delete_.cbegin(), words_to_delete_.cend(), [&](const auto word)
		{
			word_to_document_freqs_[*word].erase(document_id);
            if(!word_to_document_freqs_.count(*word)) {
                word_to_document_freqs_.erase(*word);
            }
		});	
    
    document_to_word_freqs_.erase(document_id);
    
    documents_.erase(document_id);

	//const auto it = find(policy, document_ids_.begin(), document_ids_.end(), document_id);
	document_ids_.erase(document_id);
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentStatus status) const {
	return FindTopDocuments(policy,
		raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
			return document_status == status;
		});
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query) const {
	return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query,
	DocumentPredicate document_predicate) const {

	//LOG_DURATION_STREAM("Operation time", std::cout);
	const auto query = ParseQuery(raw_query);

	auto matched_documents = FindAllDocuments(policy, query, document_predicate);

	std::sort(policy, matched_documents.begin(), matched_documents.end(),
		[](const Document& lhs, const Document& rhs) {
			if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
				return lhs.rating > rhs.rating;
			}
			else {
				return lhs.relevance > rhs.relevance;
			}
		});
	if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
		matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
	}

	return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query,
	DocumentPredicate document_predicate) const {

	return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}