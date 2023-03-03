#include "search_server.h"

using namespace std;

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
        const vector<int>& ratings) {
        if ((document_id < 0) || (documents_.count(document_id) > 0)) {
            throw invalid_argument("Invalid document_id"s);
        }
        const auto words = SplitIntoWordsNoStop(document);

        std::map<std::string, double> words_freq;
        const double inv_word_count = 1.0 / words.size();
        for (const std::string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
            words_freq[word] = word_to_document_freqs_[word][document_id];
        }

        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status, words_freq});
        document_ids_.push_back(document_id);
    }

vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

    vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int SearchServer::GetDocumentCount() const {
        return documents_.size();
    }

    std::vector<int>::iterator SearchServer::begin() {
        return document_ids_.begin();
    }

    std::vector<int>::iterator SearchServer::end() {
        return document_ids_.end();
    }

    // O(logN) is a MUST
    const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const
    {
        // O(logN)
        if (!binary_search(document_ids_.begin(), document_ids_.end(), document_id)) {
            static const map<string, double> result;
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
        const auto &doc_data = documents_.at(document_id);

        // O(W)
        for (const auto& [word, freq] : doc_data.words_freq) {
            word_to_document_freqs_[word].erase(document_id);
        }

        // O(N)
        documents_.erase(document_id);
        // O(N)
        const auto it = find(document_ids_.begin(), document_ids_.end(), document_id);
        document_ids_.erase(it);
    }

    tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string &raw_query,
                                                                      int document_id) const
    {
        //LOG_DURATION_STREAM("Operation time"s, std::cout);
        const auto query = ParseQuery(raw_query);

        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

bool SearchServer::IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    bool SearchServer::IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                throw invalid_argument("Word "s + word + " is invalid"s);
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

    

    SearchServer::QueryWord SearchServer::ParseQueryWord(const string& text) const {
        if (text.empty()) {
            throw invalid_argument("Query word is empty"s);
        }
        string word = text;
        bool is_minus = false;
        if (word[0] == '-') {
            is_minus = true;
            word = word.substr(1);
        }
        if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
            throw invalid_argument("Query word "s + text + " is invalid");
        }

        return { word, is_minus, IsStopWord(word) };
    }

    SearchServer::Query SearchServer::ParseQuery(const string& text) const {
        Query result;
        for (const string& word : SplitIntoWords(text)) {
            const auto query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    result.minus_words.insert(query_word.data);
                }
                else {
                    result.plus_words.insert(query_word.data);
                }
            }
        }
        return result;
    }

    // Existence required
    double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    