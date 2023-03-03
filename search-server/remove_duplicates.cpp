#include "remove_duplicates.h"

// O(wN(logN+logW)), где w — максимальное количество слов в документе
void RemoveDuplicates(SearchServer &search_server)
{
    std::map<std::set<std::string>, std::vector<int>> words_to_docs;
    std::set<int> docs_to_del;

    for (int doc_id : search_server)
    {                                                                     // N
        const auto &word_freq = search_server.GetWordFrequencies(doc_id); // N*logN
        std::vector<std::string> just_words;
        for (const auto &[word, freq] : word_freq)
        {
            just_words.push_back(word);
        }
        std::set set_key(just_words.begin(), just_words.end());

        if (!words_to_docs[set_key].empty())
        {
            docs_to_del.insert(doc_id);
        }
        else
        {
            words_to_docs[set_key].push_back(doc_id);
        }
    }

    for (const auto doc_id : docs_to_del)
    {
        search_server.RemoveDocument(doc_id);
        std::cout << "Found duplicate document id " << doc_id << std::endl;
    }
}