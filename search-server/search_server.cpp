#include "search_server.h"

using namespace std;

SearchServer::SearchServer(std::string_view stop_words_text)
	: SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}

SearchServer::SearchServer(const std::string& stop_words_text)
	: SearchServer(SplitIntoWords(stop_words_text)) // Invoke delegating constructor from string container
{
}

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status,
	const vector<int>& ratings) {
	if ((document_id < 0) || (documents_.count(document_id) > 0)) {
		throw invalid_argument("Invalid document_id"s);
	}
	string_storage.push_back(string(document));
	const auto words = SplitIntoWordsNoStop(string_storage.back());
    
    std::map<std::string_view, double> words_freq;
	const double inv_word_count = 1.0 / words.size();
	for (std::string_view word : words) {
        
        word_to_document_freqs_[word][document_id] += inv_word_count;
		words_freq[word] = word_to_document_freqs_[word][document_id];

		document_to_word_freqs_[document_id][word] += inv_word_count;
	}

	documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, words_freq });
	document_ids_.insert(document_id);
}

vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
	return FindTopDocuments(std::execution::seq, raw_query, status);
}

vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
	return FindTopDocuments(std::execution::seq, raw_query);
}

int SearchServer::GetDocumentCount() const {
	return documents_.size();
}

std::set<int>::iterator SearchServer::begin() {
	return document_ids_.begin();
}

std::set<int>::iterator SearchServer::end() {
	return document_ids_.end();
}

// O(logN) is a MUST
const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
	// O(logN)
	if (document_ids_.find(document_id) != document_ids_.end()) {
		static const map<string_view, double> result;
		return result;
	}

	// O(1)
	const auto& doc_data = documents_.at(document_id);

	return doc_data.words_freq;
}

// O(w(logN+logW)), где w — количество слов в удаляемом документе
void SearchServer::RemoveDocument(int document_id)
{

	// O(1)
	const auto& doc_data = documents_.at(document_id);
    
    document_to_word_freqs_.erase(document_id);

	// O(W)
	for (auto [word, freq] : doc_data.words_freq) {
		word_to_document_freqs_[word].erase(document_id);
		if (word_to_document_freqs_[word].empty()) {
			word_to_document_freqs_.erase(word);
		}
	}

	// O(N)
	documents_.erase(document_id);
	// O(N)
	//const auto it = find(document_ids_.begin(), document_ids_.end(), document_id);
	//document_ids_.erase(it);
    document_ids_.erase(document_id);
}

bool SearchServer::IsStopWord(std::string_view word) const {
	return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
	// A valid word must not contain special characters
	return none_of(word.begin(), word.end(), [](char c) {
		return c >= '\0' && c < ' ';
		});
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
	vector<string_view> words;
	for (const string_view word : SplitIntoWords(text)) {
		if (!IsValidWord(word)) {
			throw invalid_argument("Word "s + string(word) + " is invalid"s);
		}
		if (!IsStopWord(word)) {
			words.push_back(word);
		}
	}
	return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
	if (ratings.empty()) {
		return 0;
	}
	int rating_sum = 0;
	for (const int rating : ratings) {
		rating_sum += rating;
	}
	return rating_sum / static_cast<int>(ratings.size());
}



SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
	if (text.empty()) {
		throw invalid_argument("Query word is empty"s);
	}
	string_view word = text;
	bool is_minus = false;
	if (word[0] == '-') {
		is_minus = true;
		word = word.substr(1);
	}
	if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
		throw invalid_argument("Query word "s + string(text) + " is invalid");
	}

	return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQueryCore(string_view text) const {
	Query result;
	for (string_view word : SplitIntoWords(text)) {
		const auto query_word = ParseQueryWord(word);
		if (!query_word.is_stop) {
			if (query_word.is_minus) {
				result.minus_words.push_back(query_word.data);
			}
			else {
				result.plus_words.push_back(query_word.data);
			}
		}
	}

	return result;
}

SearchServer::Query SearchServer::ParseQuery(string_view text) const {

	Query result = ParseQueryCore(text);

	std::sort(result.minus_words.begin(), result.minus_words.end());
	auto new_end_minus = std::unique(result.minus_words.begin(), result.minus_words.end());
	result.minus_words.erase(new_end_minus, result.minus_words.end());

	std::sort(result.plus_words.begin(), result.plus_words.end());
	auto new_end_plus = std::unique(result.plus_words.begin(), result.plus_words.end());
	result.plus_words.erase(new_end_plus, result.plus_words.end());

	return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(string_view word) const {
	return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query,
	int document_id) const
{
	
    if (document_ids_.find(document_id) == document_ids_.end()) {
        throw out_of_range("No such document_id");
    }
    /*
    if (document_to_word_freqs_.count(document_id) == 0) {
        throw out_of_range("No such document_id");
    }*/
    
    //LOG_DURATION_STREAM("Operation time"s, std::cout);
	const auto query = ParseQuery(raw_query);
    
    for (string_view word : query.minus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			//matched_words.clear();
			//break;
			std::vector<std::string_view> matched_words(0);
            return {matched_words, documents_.at(document_id).status};
		}
	}

	vector<string_view> matched_words;

	for (string_view word : query.plus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.push_back(word);
		}
	}
    
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy& policy, std::string_view raw_query,
	int document_id) const {
	return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy& policy, std::string_view raw_query,
	int document_id) const
{
    //LOG_DURATION_STREAM("Parse query time", std::cout);
	/*
    if (document_to_word_freqs_.count(document_id) == 0) {
        throw out_of_range("No such document_id");
    }*/
    if (document_ids_.find(document_id) == document_ids_.end()) {
        throw out_of_range("No such document_id");
    }
    
    const auto query = ParseQueryCore(raw_query);
    
	//const auto& words_map = document_to_word_freqs_.at(document_id);

	if (std::any_of(policy, query.minus_words.cbegin(), query.minus_words.cend(), [&](const auto& word) {
		if (word_to_document_freqs_.count(word) == 0) {
			return false;
		}
        return word_to_document_freqs_.at(word).count(document_id) > 0;
		})) {
		std::vector<std::string_view> matched_words(0);
        return {matched_words, documents_.at(document_id).status};
	}

	std::vector<std::string_view> matched_words(query.plus_words.size());

	auto pred = [&](auto word) {
        //const auto& v = docs_words_.at(document_id);
        //return std::find(v.begin(), v.end(), word) != v.end();		
        if (word_to_document_freqs_.count(word) == 0) return false;
        return word_to_document_freqs_.at(word).count(document_id) > 0;
        //return document_to_word_freqs_.at(document_id).count(word) > 0;
        //return words_map.count(word) > 0;
        /*
        return std::any_of(policy, words_map.cbegin(), words_map.cend(), [&](const auto pair) {
			return pair.first == word;
		});        */
        
	};

	auto new_end = std::copy_if(policy, query.plus_words.cbegin(), query.plus_words.cend(), matched_words.begin(), pred);
	matched_words.erase(new_end, matched_words.end());    

	std::sort(policy, matched_words.begin(), matched_words.end());
	auto new_end2 = std::unique(policy, matched_words.begin(), matched_words.end());
	matched_words.erase(new_end2, matched_words.end());
    
    return { matched_words, documents_.at(document_id).status };
}