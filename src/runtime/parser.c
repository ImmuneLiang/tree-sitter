#include "parser.h"
#include <stdio.h>

#define TS_DEBUG_PARSE
#define TS_DEBUG_LEX

#ifdef TS_DEBUG_LEX
#define DEBUG_LEX(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG_LEX(...)
#endif

#ifdef TS_DEBUG_PARSE
#define DEBUG_PARSE(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG_PARSE(...)
#endif

static int INITIAL_STACK_SIZE = 100;

struct TSStackEntry {
    TSState state;
    TSTree *node;
};

TSParser TSParserMake(const char *input) {
    TSParser result = {
        .input = input,
        .position = 0,
        .lookahead_node = NULL,
        .lex_state = 0,
        .stack = calloc(INITIAL_STACK_SIZE, sizeof(TSStackEntry)),
        .stack_size = 0,
        .result = {
            .tree = NULL,
            .error = {
                .type = TSParseErrorTypeNone,
                .expected_inputs = NULL,
                .expected_input_count = 0
            },
        },
    };
    return result;
}

void TSParserShift(TSParser *parser, TSState parse_state) {
    DEBUG_PARSE("shift %d \n", parse_state);
    TSStackEntry *entry = (parser->stack + parser->stack_size);
    entry->state = parse_state;
    entry->node = parser->lookahead_node;
    parser->lookahead_node = NULL;
    parser->stack_size++;
}

void TSParserReduce(TSParser *parser, TSSymbol symbol, int child_count) {
    parser->stack_size -= child_count;

    TSTree **children = malloc(child_count * sizeof(TSTree *));
    for (int i = 0; i < child_count; i++) {
        children[i] = parser->stack[parser->stack_size + i].node;
    }
    
    parser->lookahead_node = TSTreeMake(symbol, child_count, children);
    DEBUG_PARSE("reduce: %ld, state: %u \n", symbol, TSParserParseState(parser));
}

void TSParserError(TSParser *parser, size_t count, const char **expected_inputs) {
    TSParseError *error = &parser->result.error;
    error->type = TSParseErrorTypeSyntactic;
    error->expected_input_count = count;
    error->expected_inputs = expected_inputs;
    error->lookahead_sym = TSParserLookaheadSym(parser);
}

void TSParserLexError(TSParser *parser, size_t count, const char **expected_inputs) {
    TSParseError *error = &parser->result.error;
    error->type = TSParseErrorTypeLexical;
    error->expected_input_count = count;
    error->expected_inputs = expected_inputs;
    error->lookahead_sym = TSParserLookaheadSym(parser);
}

void TSParserAdvance(TSParser *parser, TSState lex_state) {
    DEBUG_LEX("character: '%c' \n", TSParserLookaheadChar(parser));
    parser->position++;
    parser->lex_state = lex_state;
}

char TSParserLookaheadChar(const TSParser *parser) {
    return parser->input[parser->position];
}

long TSParserLookaheadSym(const TSParser *parser) {
    TSTree *node = parser->lookahead_node;
    return node ? node->value : -1;
}

void TSParserSetLookaheadSym(TSParser *parser, TSSymbol symbol) {
    DEBUG_LEX("token: %ld \n", symbol);
    parser->lookahead_node = TSTreeMake(symbol, 0, NULL);
}

TSState TSParserParseState(const TSParser *parser) {
    return parser->stack[parser->stack_size - 1].state;
}

TSState TSParserLexState(const TSParser *parser) {
    return parser->lex_state;
}

void TSParserSetLexState(TSParser *parser, TSState lex_state) {
    parser->lex_state = lex_state;
}

void TSParserAcceptInput(TSParser *parser) {
    parser->result.tree = parser->stack[parser->stack_size - 1].node;
}

TSParseResult TSParserResult(TSParser *parser) {
    return parser->result;
}