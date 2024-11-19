#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "jsonParser.h"


static json_t *jsonParseBase(const char **string, enum jsonStatus *err);

json_t *jsonParse(const char *str, enum jsonStatus *err) {
    const char *jsonString = str;
    return jsonParseBase(&jsonString, err);
}

static const char *skipSpaces(const char *str) {
    assert(str);
    while(*str && isspace(*str))
        str++;
    return str;
}

/// @brief Read string that starts and ends with ", move str to position after end of string
/// Parses escape characters //TODO
/// @return Allocated copy of string
static char *readString(const char **str) {
    if (**str != '"') {
        fprintf(stderr, "String doesn't start with '\"', can't read\n");
        fprintf(stderr, "'%.5s'\n" ,*str);
        return NULL;
    }
    //TODO: escape characters
    size_t strSize = 0;
    (*str)++;
    const char *strStart = *str;
    while (**str && **str != '"') {
        (*str)++;
        strSize++;
    }

    if (!**str) {
        fprintf(stderr, "Unexpected null-terminator, can't read string\n");
        return NULL;
    }
    (*str)++;

    char *resultString = (char *) calloc(strSize+1, sizeof(char));
    strncpy(resultString, strStart, strSize);
    JSONlog("read string '%s'\n", resultString);
    return resultString;
}

static enum jsonStatus jsonRealloc(json_t *jsonObj, size_t *capacity) {
    assert(jsonObj);
    assert(capacity);

    json_t *newJson = realloc(jsonObj, 2 * (*capacity) * sizeof(*jsonObj));
    if (!newJson) {
        fprintf(stderr, "Failed to reallocate data of json[%p]\n", jsonObj);
        return JSON_MEMORY_ERROR;
    }
}

static enum jsonObjType getObjType(const char *str) {
    assert(str);

    switch(*str) {
        case '"':
            return jsonString;
        case '{':
            return jsonObject;
        //TODO:Maybe array support
        default:
            break;
    }

    if (*str == '+' || *str == '-') str++;
    if (!isdigit(*str)) return jsonNone;
    // , } and space characters separate values
    while (*str && (*str != ',') && (*str != '}') && !isspace(*str)) {
        if (*str == '.') //float must have dot
            return jsonFloat;

        if (isdigit(*str)) {
            str++;
            continue;
        }
        //any other character means syntax error
        return jsonNone;
    }
    // we did not find any wrong symbols, so this is integer
    return jsonInt;
}

static enum jsonStatus parseObjectValue(json_t *jsonObj, const char **string, enum jsonStatus *err) {
    #define HANDLE_ERROR(valueType)                                                          \
        fprintf(stderr, "Failed to read " valueType " value of field %s\n", jsonObj->name);  \
        *err = JSON_SYNTAX_ERROR;

    switch(jsonObj->type) {
        case jsonNone:
            fprintf(stderr, "Can't deduce type of object %10s...", *string);
            *err = JSON_TYPE_ERROR;
            break;
        case jsonString: {
            char *strValue = readString(string);
            if (!strValue) {
                HANDLE_ERROR("string")
            }
            jsonObj->val.str_ = strValue;
            break;
        }
        case jsonInt: {
            long scannedChars = 0;
            sscanf(*string, "%d%ln", &jsonObj->val.int_, &scannedChars);
            if (scannedChars <= 0) {
                HANDLE_ERROR("integer")
            }
            *string += scannedChars;
            break;
        }
        case jsonFloat: {
            long scannedChars = 0;
            sscanf(*string, " %f%ln", &jsonObj->val.float_, &scannedChars);
            if (scannedChars <= 0) {
                HANDLE_ERROR("float")
            }
            JSONlog("Scanned float %g\n", jsonObj->val.float_);
            *string += scannedChars;
            break;
        }
        case jsonObject: {
            json_t *child = jsonParseBase(string, err);
            if (*err != JSON_OK) {
                HANDLE_ERROR("object")
            }
            jsonObj->val.json_ = child;
            break;
        }
        default:
            fprintf(stderr, "AHTUNG\n");
            *err = JSON_TYPE_ERROR;
            break;
    }

    return *err;
    #undef HANDLE_ERROR
}

json_t *jsonParseBase(const char **string, enum jsonStatus *err) {
    assert(string);
    assert(err);

    if (*err != JSON_OK) return NULL;

    *string = skipSpaces(*string);
    if (**string != '{') {
        fprintf(stderr, "Json must start with '{'\n");
        *err = JSON_SYNTAX_ERROR;
        return NULL;
    }
    (*string)++;

    *string = skipSpaces(*string);
    size_t capacity = JSON_START_FIELD_COUNT;
    json_t *jsonObj = (json_t *) calloc(capacity, sizeof(json_t));

    while (*err == JSON_OK) {

        if (**string == '}') {
            (*string)++;
            break;
        }

        *string = skipSpaces(*string);
        char *name = readString(string);
        if (!name) {
            fprintf(stderr, "Failed to read name of field\n");
            *err = JSON_SYNTAX_ERROR;
            return NULL;
        }

        jsonObj[jsonObj->fieldCount].name = name;

        *string = skipSpaces(*string);
        if (**string != ':') {
            fprintf(stderr, "No ':' after name '%s', syntaxError\n", name);
            *err = JSON_SYNTAX_ERROR;
            return NULL;
        } else (*string)++;

        *string = skipSpaces(*string);

        jsonObj[jsonObj->fieldCount].type = getObjType(*string);
        JSONlog("Object type = %d\n", jsonObj[jsonObj->fieldCount].type);

        if (parseObjectValue(jsonObj + jsonObj->fieldCount, string, err) != JSON_OK) {
            jsonDtor(jsonObj);
            return NULL;
        }

        jsonObj->fieldCount++;
        if (jsonObj->fieldCount == capacity)
            jsonRealloc(jsonObj, &capacity);

        *string = skipSpaces(*string);
        if (**string == ',') (*string)++;
    }

    return jsonObj;
}

enum jsonStatus jsonDtor(json_t *jsonObj) {
    assert(jsonObj);

    for (size_t fieldIdx = 0; fieldIdx < jsonObj->fieldCount; fieldIdx++) {
        free(jsonObj[fieldIdx].name);

        if (jsonObj[fieldIdx].type == jsonString) {
            free(jsonObj[fieldIdx].val.str_);
        }
        else if (jsonObj[fieldIdx].type == jsonObject) {
            jsonDtor(jsonObj[fieldIdx].val.json_);
        }
    }

    free(jsonObj);
    return JSON_OK;
}

json_t *jsonGet(json_t *jsonObj, const char *field) {
    assert(jsonObj);
    assert(field);

    for (size_t fieldIdx = 0; fieldIdx < jsonObj->fieldCount; fieldIdx++) {
        if (strncmp(jsonObj[fieldIdx].name, field, JSON_MAX_NAME_LEN) == 0)
            return jsonObj + fieldIdx;
    }
    return NULL;
}
