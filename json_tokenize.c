//
// Created by Chad Russell on 4/25/17.
//

#include "json_tokenize.h"

/**
 * Allocates a fresh unused token from the token pull.
 */
struct json_tok_t *
json_parser_alloc_token(struct json_parser_t *parser, struct json_tok_t *tokens, size_t num_tokens)
{
    struct json_tok_t *tok;
    if (parser->tok_next >= num_tokens) {
        return NULL;
    }
    tok = &tokens[parser->tok_next++];
    tok->start = tok->end = -1;
    tok->size = 0;
    return tok;
}

/**
 * Fills token type and boundaries.
 */
void
json_parser_fill_token(struct json_tok_t *token, enum JSON_TOK_TYPE type, uint32_t start, uint32_t end)
{
    token->type = type;
    token->start = start;
    token->end = end;
    token->size = 0;
}

/**
 * Fills next available token with JSON primitive.
 */
int
json_parser_parse_primitive(struct json_parser_t *parser, const char *js,
                            size_t len, struct json_tok_t *tokens, size_t num_tokens)
{
    struct json_tok_t *token;
    uint32_t start;

    start = parser->pos;

    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
        switch (js[parser->pos]) {
            case ':':
            case '\t':
            case '\r':
            case '\n':
            case ' ':
            case ',':
            case ']':
            case '}':
                goto found;
            default:break;
        }
        if (js[parser->pos] < 32 || js[parser->pos] >= 127) {
            parser->pos = start;
            return JSON_ERROR_INVAL;
        }
    }

    found:
    if (tokens == NULL) {
        parser->pos--;
        return 0;
    }
    token = json_parser_alloc_token(parser, tokens, num_tokens);
    if (token == NULL) {
        parser->pos = start;
        return JSON_ERROR_NOMEM;
    }
    json_parser_fill_token(token, JSON_TOK_TYPE_PRIMITIVE, start, parser->pos);
    parser->pos--;
    return 0;
}

/**
 * Fills next token with JSON string.
 */
int
json_parser_parse_string(struct json_parser_t *parser, const char *js,
                         size_t len, struct json_tok_t *tokens, size_t num_tokens)
{
    struct json_tok_t *token;

    uint32_t start = parser->pos;

    parser->pos++;

    /* Skip starting quote */
    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
        char c = js[parser->pos];

        /* Quote: end of string */
        if (c == '\"') {
            if (tokens == NULL) {
                return 0;
            }
            token = json_parser_alloc_token(parser, tokens, num_tokens);
            if (token == NULL) {
                parser->pos = start;
                return JSON_ERROR_NOMEM;
            }
            json_parser_fill_token(token, JSON_TOK_TYPE_STRING, start + 1, parser->pos);
            return 0;
        }

        /* Backslash: Quoted symbol expected */
        if (c == '\\' && parser->pos + 1 < len) {
            uint32_t i;
            parser->pos++;
            switch (js[parser->pos]) {
                /* Allowed escaped symbols */
                case '\"': case '/' : case '\\' : case 'b' :
                case 'f' : case 'r' : case 'n'  : case 't' :
                    break;
                    /* Allows escaped symbol \uXXXX */
                case 'u':
                    parser->pos++;
                    for(i = 0; i < 4 && parser->pos < len && js[parser->pos] != '\0'; i++) {
                        /* If it isn't a hex character we have an error */
                        if(!((js[parser->pos] >= 48 && js[parser->pos] <= 57) || /* 0-9 */
                             (js[parser->pos] >= 65 && js[parser->pos] <= 70) || /* A-F */
                             (js[parser->pos] >= 97 && js[parser->pos] <= 102))) { /* a-f */
                            parser->pos = start;
                            return JSON_ERROR_INVAL;
                        }
                        parser->pos++;
                    }
                    parser->pos--;
                    break;
                    /* Unexpected symbol */
                default:
                    parser->pos = start;
                    return JSON_ERROR_INVAL;
            }
        }
    }
    parser->pos = start;
    return JSON_ERROR_PART;
}

/**
 * Parse JSON string and fill tokens.
 */
uint32_t
json_parser_parse(struct json_parser_t *parser, const char *js, size_t len,
                  struct json_tok_t *tokens, uint32_t num_tokens)
{
    int32_t r;
    int32_t i;
    struct json_tok_t *token;
    int32_t count = parser->tok_next;

    for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
        char c;
        enum JSON_TOK_TYPE type;

        c = js[parser->pos];
        switch (c) {
            case '{': case '[':
                count++;
                if (tokens == NULL) {
                    break;
                }
                token = json_parser_alloc_token(parser, tokens, num_tokens);
                if (token == NULL)
                    return JSON_ERROR_NOMEM;
                if (parser->tok_super != -1) {
                    tokens[parser->tok_super].size++;
                }
                token->type = (c == '{' ? JSON_TOK_TYPE_OBJECT : JSON_TOK_TYPE_ARRAY);
                token->start = parser->pos;
                parser->tok_super = parser->tok_next - 1;
                break;
            case '}': case ']':
                if (tokens == NULL)
                    break;
                type = (c == '}' ? JSON_TOK_TYPE_OBJECT : JSON_TOK_TYPE_ARRAY);
                for (i = parser->tok_next - 1; i >= 0; i--) {
                    token = &tokens[i];
                    if (token->start != -1 && token->end == -1) {
                        if (token->type != type) {
                            return JSON_ERROR_INVAL;
                        }
                        parser->tok_super = -1;
                        token->end = parser->pos + 1;
                        break;
                    }
                }
                /* Error if unmatched closing bracket */
                if (i == -1) return JSON_ERROR_INVAL;
                for (; i >= 0; i--) {
                    token = &tokens[i];
                    if (token->start != -1 && token->end == -1) {
                        parser->tok_super = i;
                        break;
                    }
                }
                break;
            case '\"':
                r = json_parser_parse_string(parser, js, len, tokens, num_tokens);
                if (r < 0) return r;
                count++;
                if (parser->tok_super != -1 && tokens != NULL)
                    tokens[parser->tok_super].size++;
                break;
            case '\t' : case '\r' : case '\n' : case ' ':
                break;
            case ':':
                parser->tok_super = parser->tok_next - 1;
                break;
            case ',':
                if (tokens != NULL && parser->tok_super != -1 &&
                    tokens[parser->tok_super].type != JSON_TOK_TYPE_ARRAY &&
                    tokens[parser->tok_super].type != JSON_TOK_TYPE_OBJECT) {
                    for (i = parser->tok_next - 1; i >= 0; i--) {
                        if (tokens[i].type == JSON_TOK_TYPE_ARRAY || tokens[i].type == JSON_TOK_TYPE_OBJECT) {
                            if (tokens[i].start != -1 && tokens[i].end == -1) {
                                parser->tok_super = i;
                                break;
                            }
                        }
                    }
                }
                break;
            default:
                r = json_parser_parse_primitive(parser, js, len, tokens, num_tokens);
                if (r < 0) return r;
                count++;
                if (parser->tok_super != -1 && tokens != NULL)
                    tokens[parser->tok_super].size++;
                break;
        }
    }

    if (tokens != NULL) {
        for (i = parser->tok_next - 1; i >= 0; i--) {
            /* Unmatched opened object or array */
            if (tokens[i].start != -1 && tokens[i].end == -1) {
                return JSON_ERROR_PART;
            }
        }
    }

    return count;
}

/**
 * Creates a new parser based over a given  buffer with an array of tokens
 * available.
 */
void
json_parser_init(struct json_parser_t *parser)
{
    parser->pos = 0;
    parser->tok_next = 0;
    parser->tok_super = -1;
}
