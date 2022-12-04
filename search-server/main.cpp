#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);

        for (const auto& word : words) {
            word_to_document_freqs_[word][document_id] += 1.0 / words.size();
        }

        ++document_count_;

    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_struct = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_struct);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    struct Query {
        set<string> minus_words;
        set<string> plus_words;
    };

    int document_count_ = 0;

    map<string, map<int, double>> word_to_document_freqs_;

    set<string> stop_words_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const {
        Query query_struct;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-') {
                query_struct.minus_words.insert(word.substr(1));
            }
            else {
                query_struct.plus_words.insert(word);
            }
        }
        return query_struct;
    }

    vector<Document> FindAllDocuments(const Query& query_struct) const {

        vector<Document> matched_documents;
        map<int, double> result_docs;

        for (const string& plus_word : query_struct.plus_words) {
            
            if (word_to_document_freqs_.count(plus_word) == 0) continue;
            
            const auto word_docs = word_to_document_freqs_.at(plus_word);
            double idf = log(static_cast<double>(document_count_) / word_docs.size());
            
            for (const auto& [doc_id, doc_tf] : word_docs) {
                double rel = doc_tf * idf;
                result_docs[doc_id] += rel;
            }

        }

        for (const string& minus_word : query_struct.minus_words) {
            if (word_to_document_freqs_.count(minus_word) == 0) continue;
            for (const auto& doc_info : word_to_document_freqs_.at(minus_word)) {
                result_docs.erase(doc_info.first);
            }
        }

        for (const auto& [id, rel] : result_docs) {
            matched_documents.push_back({ id, rel });
        }

        return matched_documents;
    }

};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}