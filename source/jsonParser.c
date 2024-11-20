#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "jsonParser.h"


static json_t *jsonParseBase(const char **string, enum jsonStatus *err);

json_t *jsonParse(const char *str, enum jsonStatus *err) {
    const char *JSON_STRING = str;
    return jsonParseBase(&JSON_STRING, err);
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

static enum jsonStatus jsonRealloc(json_t **jsonObj, size_t *capacity) {
    assert(jsonObj);
    assert(capacity);

    json_t *newJson = (json_t *) realloc(*jsonObj, 2 * (*capacity) * sizeof(**jsonObj));
    if (!newJson) {
        fprintf(stderr, "Failed to reallocate data of json[%p]\n", *jsonObj);
        return JSON_MEMORY_ERROR;
    }

    *capacity *= 2;
    *jsonObj = newJson;
    return JSON_OK;
}

static enum jsonObjType getObjType(const char *str) {
    assert(str);

    switch(*str) {
        case '"':
            return JSON_STRING;
        case '{':
            return JSON_OBJECT;
        //TODO:Maybe array support
        default:
            break;
    }

    if (*str == '+' || *str == '-') str++;
    if (!isdigit(*str)) return jsonNone;
    // , } and space characters separate values
    while (*str && (*str != ',') && (*str != '}') && !isspace(*str)) {
        if (*str == '.') //float must have dot
            return JSON_FLOAT;

        if (isdigit(*str)) {
            str++;
            continue;
        }
        //any other character means syntax error
        return jsonNone;
    }
    // we did not find any wrong symbols, so this is integer
    return JSON_INT;
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
        case JSON_STRING: {
            char *strValue = readString(string);
            if (!strValue) {
                HANDLE_ERROR("string")
            }
            jsonObj->val.str_ = strValue;
            break;
        }
        case JSON_INT: {
            long scannedChars = 0;
            sscanf(*string, "%d%ln", &jsonObj->val.int_, &scannedChars);
            if (scannedChars <= 0) {
                HANDLE_ERROR("integer")
            }
            *string += scannedChars;
            break;
        }
        case JSON_FLOAT: {
            long scannedChars = 0;
            sscanf(*string, " %f%ln", &jsonObj->val.float_, &scannedChars);
            if (scannedChars <= 0) {
                HANDLE_ERROR("float")
            }
            JSONlog("Scanned float %g\n", jsonObj->val.float_);
            *string += scannedChars;
            break;
        }
        case JSON_OBJECT: {
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
            jsonRealloc(&jsonObj, &capacity);

        *string = skipSpaces(*string);
        if (**string == ',') (*string)++;
    }

    return jsonObj;
}

enum jsonStatus jsonDtor(json_t *jsonObj) {
    assert(jsonObj);

    for (size_t fieldIdx = 0; fieldIdx < jsonObj->fieldCount; fieldIdx++) {
        free(jsonObj[fieldIdx].name);

        if (jsonObj[fieldIdx].type == JSON_STRING) {
            free(jsonObj[fieldIdx].val.str_);
        }
        else if (jsonObj[fieldIdx].type == JSON_OBJECT) {
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

int *jsonGetInt(json_t *jsonObj, const char *field) {
    assert(jsonObj);
    assert(field);

    json_t *foundField = jsonGet(jsonObj, field);
    return (foundField && foundField->type == JSON_INT) ? &foundField->val.int_ : NULL;
}

float   *jsonGetFloat  (json_t *jsonObj, const char *field) {
    assert(jsonObj);
    assert(field);

    json_t *foundField = jsonGet(jsonObj, field);
    return (foundField && foundField->type == JSON_FLOAT) ? &foundField->val.float_ : NULL;
}

char   **jsonGetString (json_t *jsonObj, const char *field) {
    assert(jsonObj);
    assert(field);

    json_t *foundField = jsonGet(jsonObj, field);
    return (foundField && foundField->type == JSON_STRING) ? &foundField->val.str_ : NULL;
}

json_t **jsonGetObject (json_t *jsonObj, const char *field) {
    assert(jsonObj);
    assert(field);

    json_t *foundField = jsonGet(jsonObj, field);
    return (foundField && foundField->type == JSON_OBJECT) ? &foundField->val.json_ : NULL;
}

static void printTabs(unsigned tabulation) {
    for (unsigned tabIdx = 0; tabIdx < tabulation; tabIdx++)
        putchar('\t');
}

enum jsonStatus jsonPrint(json_t *jsonObj, unsigned tabulation) {
    printTabs(tabulation);
    printf("{\n");

    for (unsigned idx = 0; idx < jsonObj->fieldCount; idx++) {
        printTabs(tabulation+1);
        printf("\"%s\" : ", jsonObj[idx].name);
        switch(jsonObj[idx].type) {
            case JSON_INT:
                printf("%d,\n", jsonObj[idx].val.int_);
                break;
            case JSON_FLOAT:
                printf("%f,\n", jsonObj[idx].val.float_);
                break;
            case JSON_STRING:
                printf("\"%s\",\n", jsonObj[idx].val.str_);
                break;
            case JSON_OBJECT:
                putchar('\n');
                jsonPrint(jsonObj[idx].val.json_, tabulation + 1);
                printf(",\n");
                break;
            default:
                printf("TYPE ERROR,\n");
                break;
        }
    }
    printTabs(tabulation);
    printf("}");

    return JSON_OK;
}
