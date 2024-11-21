#ifndef JSON_PARSER_H__
#define JSON_PARSER_H__

const size_t JSON_MAX_NAME_LEN = 128;
const size_t JSON_START_FIELD_COUNT = 4;

#if defined(JSON_LOG_MAX) && !defined(NDEBUG)
    #define JSONlog(...) fprintf(stderr, __VA_ARGS__)
#else
    #define JSONlog(...)
#endif

enum jsonObjType {
    jsonNone,
    JSON_INT,
    JSON_FLOAT,
    JSON_STRING,
    JSON_OBJECT
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

/// @brief Parse json from given string
json_t *jsonParse(const char *str, enum jsonStatus *err);

/// @brief Parse json from file
json_t *jsonParseFromFile(const char *fileName, enum jsonStatus *err);

enum jsonStatus jsonDtor(json_t *jsonObj);

/// @brief get json object with name field
json_t *jsonGet(json_t *jsonObj, const char *field);

/// @brief get pointer to stored value of type Int, float, string or object
/// Return NULL if jsonObj[field] doesn't exists
int     *jsonGetInt    (json_t *jsonObj, const char *field);
float   *jsonGetFloat  (json_t *jsonObj, const char *field);
char   **jsonGetString (json_t *jsonObj, const char *field);
json_t **jsonGetObject (json_t *jsonObj, const char *field);

enum jsonStatus jsonPrint(json_t *jsonObj, unsigned tabulation);

#endif
