# Simple json parser

**Doesn't support arrays (yet)**

As simple as
```c++
json_t *yourJson = jsonParse(yourJSON_STRING, parseStatus);
```

## Compiling
```
make static BUILD=RELEASE
```
If something goes wrong, do `make clean`

It will give you `libjsonParser.a` in `build/` directory

Link it with `-ljsonParser`
