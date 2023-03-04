#include "remove_duplicates.h"

// O(wN(logN+logW)), где w — максимальное количество слов в документе
void RemoveDuplicates(SearchServer &search_server)
{
    std::set<std::set<std::string>> words_to_docs;
    std::set<int> docs_to_del;

    for (int doc_id : search_server)
    {                                                                     // N
        const auto &word_freq = search_server.GetWordFrequencies(doc_id); // N*logN
        std::vector<std::string> just_words;
        std::vector<int> temp_;
        temp_.reserve(word_freq.size());
        std::transform(word_freq.begin(), word_freq.end(), temp_.begin(), [&just_words](const auto &map_elem)
                           {
                                just_words.push_back(map_elem.first);
                                return 0;
                            });
        std::set set_key(just_words.begin(), just_words.end());

        if (words_to_docs.count(set_key))
        {
            docs_to_del.insert(doc_id);
        }
        else
        {
            words_to_docs.insert(set_key);
        }
    }

    for (const auto doc_id : docs_to_del)
    {
        search_server.RemoveDocument(doc_id);
        std::cout << "Found duplicate document id " << doc_id << std::endl;
    }
}