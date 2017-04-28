//
// Created by Chad Russell on 4/25/17.
//

#ifndef SE_JSON_TOKENIZE_H
#define SE_JSON_TOKENIZE_H

#include <stddef.h>
#include <stdint.h>

/**
 * JSON type identifier. Basic types are:
 * 	- Object
 * 	- Array
 * 	- String
 * 	- Other primitive: number, boolean (true/false) or null
 */
enum JSON_TOK_TYPE {
    JSON_TOK_UNDEFINED = 0,
    JSON_TOK_TYPE_OBJECT = 1,
    JSON_TOK_TYPE_ARRAY = 2,
    JSON_TOK_TYPE_STRING = 3,
    JSON_TOK_TYPE_PRIMITIVE = 4
};

enum JSON_ERR_TYPE {
    /* Not enough tokens were provided */
            JSON_ERROR_NOMEM = -1,
    /* Invalid character inside JSON string */
            JSON_ERROR_INVAL = -2,
    /* The string is not a full JSON packet, more bytes expected */
            JSON_ERROR_PART = -3
};

/**
 * JSON token description.
 * type		type (object, array, string etc.)
 * start	start position in JSON data string
 * end		end position in JSON data string
 */
struct json_tok_t {
    enum JSON_TOK_TYPE type;
    uint32_t start;
    uint32_t end;
    uint32_t size;
};

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string
 */
struct json_parser_t {
    uint32_t pos;      // offset in the JSON string
    int32_t tok_next;  // next token to allocate
    int32_t tok_super; // superior token node, e.g parent object or array
};

/**
 * Create JSON parser over an array of tokens
 */
void json_parser_init(struct json_parser_t *parser);

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each describing
 * a single JSON object.
 */
uint32_t json_parser_parse(struct json_parser_t *parser,
                           const char *js, size_t len,
                           struct json_tok_t *tokens, uint32_t num_tokens);

#endif //SE_JSON_TOKENIZE_H
