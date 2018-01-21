//
// Created by Chad Russell on 1/19/18.
//

#ifndef SE_DIRECTORY_SEARCH_H
#define SE_DIRECTORY_SEARCH_H

#include <stdint.h>

int8_t
fuzzy_match(char const *pattern, char const *str, int *outScore, uint8_t *matches, int maxMatch);

#endif //SE_DIRECTORY_SEARCH_H
