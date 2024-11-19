#ifndef JSON_PARSER_H__
#define JSON_PARSER_H__

const size_t JSON_MAX_NAME_LEN = 128;
const size_t JSON_START_FIELD_COUNT = 1;

#if defined(JSON_LOG_MAX) && !defined(NDEBUG)
    #define JSONlog(...) fprintf(stderr, __VA_ARGS__)
#else
    #define JSONlog(...)
#endif

enum jsonObjType {
    jsonNone,
    jsonInt,
    jsonFloat,
    jsonString,
    jsonObject
};

struct json_t;

typedef union {
    int int_;
    float float_;
    char *str_;
    json_t *json_;
} jsonVal_t;

typedef struct json_t {
    size_t fieldCount;          ///< Number of fields in object
                                ///< NOTE: true only for first json object in sequence
    char *name;                 ///< Name of field (string)

    enum jsonObjType type;      ///< Type of value
    jsonVal_t val;              ///< Value of field
} json_t;

enum jsonStatus {
    JSON_OK             = 0,    ///< Success
    JSON_SYNTAX_ERROR   = 1,    ///< Syntax error (no '{', '"', etc)
    JSON_TYPE_ERROR     = 2,    ///< Unsupported value type
    JSON_MEMORY_ERROR   = 3     ///< Can't allocate memory
};

/// @brief
json_t *jsonParse(const char *jsonString, enum jsonStatus *err);

enum jsonStatus jsonDtor(json_t *jsonObj);

json_t *jsonGet(json_t *jsonObj, const char *field);

enum jsonStatus jsonPrint(json_t *jsonObj, unsigned tabulation);

#endif
