//
// Created by Chad Russell on 1/19/18.
//

#include <stddef.h>
#include <ctype.h>
#include <string.h>
#include "directory_search.h"

int8_t fuzzy_match_recursive(const char *pattern,
                             const char *str,
                             int32_t *outScore,
                             const char *strBegin,
                             uint8_t const *srcMatches,
                             uint8_t *matches,
                             int32_t maxMatches,
                             int32_t nextMatch,
                             int32_t *recursionCount,
                             int32_t recursionLimit)
{
    // Count recursions
    *recursionCount += 1;
    if (*recursionCount >= recursionLimit)
        return 0;

    // Detect end of strings
    if (*pattern == '\0' || *str == '\0')
        return 0;

    // Recursion params
    int8_t recursiveMatch = 0;
    uint8_t bestRecursiveMatches[256];
    int bestRecursiveScore = 0;

    // Loop through pattern and str looking for a match
    int8_t first_match = 1;
    while (*pattern != '\0' && *str != '\0') {

        // Found match
        if (tolower(*pattern) == tolower(*str)) {

            // Supplied matches buffer was too short
            if (nextMatch >= maxMatches)
                return 0;

            // "Copy-on-Write" srcMatches into matches
            if (first_match && srcMatches) {
                memcpy(matches, srcMatches, nextMatch);
                first_match = 0;
            }

            // Recursive call that "skips" this match
            uint8_t recursiveMatches[256];
            int recursiveScore;
            if (fuzzy_match_recursive(pattern, str + 1, &recursiveScore, strBegin, matches, recursiveMatches, sizeof(recursiveMatches), nextMatch, recursionCount, recursionLimit)) {

                // Pick best recursive score
                if (!recursiveMatch || recursiveScore > bestRecursiveScore) {
                    memcpy(bestRecursiveMatches, recursiveMatches, 256);
                    bestRecursiveScore = recursiveScore;
                }
                recursiveMatch = 1;
            }

            // Advance
            matches[nextMatch++] = (uint8_t)(str - strBegin);
            ++pattern;
        }

        // todo(chad): worth considering utf8 things here?
        str += 1;
    }

    // Determine if full pattern was matched
    int8_t matched = (int8_t) (*pattern == '\0' ? 1 : 0);

    // Calculate score
    if (matched) {
        const int sequential_bonus = 15;            // bonus for adjacent matches
        const int separator_bonus = 30;             // bonus if match occurs after a separator
        const int camel_bonus = 30;                 // bonus if match is uppercase and prev is lower
        const int first_letter_bonus = 15;          // bonus if the first letter is matched

        const int leading_letter_penalty = -5;      // penalty applied for every letter in str before the first match
        const int max_leading_letter_penalty = -15; // maximum penalty for leading letters
        const int unmatched_letter_penalty = -1;    // penalty for every letter that doesn't matter

        // Iterate str to end
        while (*str != '\0')
            ++str;

        // Initialize score
        *outScore = 100;

        // Apply leading letter penalty
        int penalty = leading_letter_penalty * matches[0];
        if (penalty < max_leading_letter_penalty)
            penalty = max_leading_letter_penalty;
        *outScore += penalty;

        // Apply unmatched penalty
        int unmatched = (int)(str - strBegin) - nextMatch;
        *outScore += unmatched_letter_penalty * unmatched;

        // Apply ordering bonuses
        for (int i = 0; i < nextMatch; ++i) {
            uint8_t currIdx = matches[i];

            if (i > 0) {
                uint8_t prevIdx = matches[i - 1];

                // Sequential
                if (currIdx == (prevIdx + 1))
                    *outScore += sequential_bonus;
            }

            // Check for bonuses based on neighbor character value
            if (currIdx > 0) {
                // Camel case
                char neighbor = strBegin[currIdx - 1];
                char curr = strBegin[currIdx];
                if (islower(neighbor) && isupper(curr))
                    *outScore += camel_bonus;

                // Separator
                int8_t neighborSeparator = neighbor == '_' || neighbor == ' ';
                if (neighborSeparator)
                    *outScore += separator_bonus;
            }
            else {
                // First letter
                *outScore += first_letter_bonus;
            }
        }
    }

    // Return best result
    if (recursiveMatch && (!matched || bestRecursiveScore > *outScore)) {
        // Recursive score is better than "this"
        memcpy(matches, bestRecursiveMatches, maxMatches);
        *outScore = bestRecursiveScore;
        return 1;
    }
    else if (matched) {
        // "this" score is better than recursive
        return 1;
    }
    else {
        // no match
        return 0;
    }
}

int8_t
fuzzy_match(char const *pattern, char const *str, int *outScore, uint8_t *matches, int maxMatches)
{
    int recursionCount = 0;
    int recursionLimit = 10;

    return fuzzy_match_recursive(pattern, str, outScore, str, NULL, matches, maxMatches, 0, &recursionCount, recursionLimit);
}

