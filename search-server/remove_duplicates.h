#pragma once

#include "search_server.h"

// O(wN(logN+logW)), где w — максимальное количество слов в документе
void RemoveDuplicates(SearchServer &search_server);