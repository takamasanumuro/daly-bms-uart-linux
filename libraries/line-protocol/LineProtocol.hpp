#include "stdio.h"
#include "string.h"
#include "stdbool.h"
#include "ctype.h"
#include "time.h"

typedef struct _InfluxDBContext {
    char bucket[32];
    char org[32];
    char token[128];
} InfluxDBContext;

void setBucket(char* buffer, const char* bucket);
void addTag(char* buffer, const char* tagKey, const char* tagValue);
void addField(char* buffer, const char* fieldKey, double fieldValue);
long getEpochSeconds();
void addTimestamp(char* buffer, long timestamp);
bool buildInfluxWritePath(char* url, int max_size, const char *org, const char *bucket);