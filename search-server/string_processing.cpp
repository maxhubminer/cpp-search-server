#include "string_processing.h"

#include <algorithm>

using namespace std;

vector<string_view> SplitIntoWords(string_view text) {
    /*vector<string_view> words;
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

    return words;*/
    
    
    // According to https://www.cppstories.com/2018/07/string-view-perf-followup/    

    std::string_view delims = " ";
    std::vector<std::string_view> words;

    for (auto first = text.data(), second = text.data(), last = first + text.size(); second != last && first != last; first = second + 1) {
        second = std::find_first_of(first, last, std::cbegin(delims), std::cend(delims));
        if (first != second)
            words.emplace_back(first, second - first);
    }

    return words;

}