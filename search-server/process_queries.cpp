#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {

    std::vector<std::vector<Document>> documents_lists(queries.size());
    std::transform(std::execution::par, queries.cbegin(), queries.cend(), documents_lists.begin(), [&](const std::string_view query)
        {
            return search_server.FindTopDocuments(query);
        });

    return documents_lists;
}

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {

    std::list<Document> result;

    std::vector<std::vector<Document>> documents_lists = ProcessQueries(search_server, queries);

    for (const auto& doc_list : documents_lists) {

        result.insert(result.cend(), doc_list.cbegin(), doc_list.cend());
    }

    return result;
}

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    std::vector<std::string_view> queries) {

    std::vector<std::vector<Document>> documents_lists(queries.size());
    std::transform(std::execution::par, queries.cbegin(), queries.cend(), documents_lists.begin(), [&](auto query)
        {
            return search_server.FindTopDocuments(query);
        });

    return documents_lists;
}

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    std::vector<std::string_view> queries) {

    std::list<Document> result;

    std::vector<std::vector<Document>> documents_lists = ProcessQueries(search_server, queries);

    for (const auto& doc_list : documents_lists) {

        result.insert(result.cend(), doc_list.cbegin(), doc_list.cend());
    }

    return result;
}