#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#include "jsonParser.h"

char *readFile(const char *fileName) {
    FILE *file = fopen(fileName, "r");
    if (!file) {
        fprintf(stderr, "Can't open file %s\n", fileName);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *str = (char *) calloc(fileSize+1, sizeof(char));
    if (fread(str, sizeof(char), fileSize, file) != fileSize) {
        free(str);
        fprintf(stderr, "Failed to read file %s\n", fileName);
        return NULL;
    }

    return str;
}

int main() {
    // setlocale(LC_ALL, "ru_RU.UTF-8");
    char *str = readFile("config.json");
    fprintf(stderr, "%s\n",str);
    enum jsonStatus err = JSON_OK;
    json_t *json = jsonParse(str, &err);

    printf("America: %s\n", jsonGet(json, "America")->val.str_);
    printf("Россия: %s\n", jsonGet(json, "Россия")->val.str_);
    printf("e: %f\n", jsonGet(json, "euler")->val.float_);

    printf("%s\n", json[1].name);
    json_t *hello = jsonGet(json, "hello")->val.json_;
    printf("hallo-ru: %s\n", jsonGet(hello, "ru")->val.str_);
    printf("hallo-de: %s\n", jsonGet(hello, "de")->val.str_);

    printf("e: %f\n", *jsonGetFloat(json, "euler"));
    jsonPrint(json, 0);
    free(str);
    jsonDtor(json);
}
