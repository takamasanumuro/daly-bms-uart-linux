//Module to take a data set and convert it to a line protocol message for InfluxDB

#include "LineProtocol.hpp"



void setMeasurement(char* buffer, const char* bucket) {
    sprintf(buffer, "%s", bucket);
}

void addTag(char* buffer, const char* tagKey, const char* tagValue) {
    sprintf(buffer, "%s,%s=%s", buffer, tagKey, tagValue);
}

void addField(char* buffer, const char* fieldKey, double fieldValue) {
    bool hasWhiteSpace = false;

    for (int i = 0; i < strlen(buffer); i++) {
        if (isspace(buffer[i])) {
            hasWhiteSpace = true;
            break;
        }
    }

    if (hasWhiteSpace) {
        sprintf(buffer, "%s,%s=%lf", buffer, fieldKey, fieldValue);
    } else {
        sprintf(buffer, "%s %s=%lf", buffer, fieldKey, fieldValue);
    }
}

long getEpochSeconds() {
    time_t epoch_seconds;
    time(&epoch_seconds);
    return epoch_seconds;
}

//Add timestamp using epoch in seconds
void addTimestamp(char* buffer, long timestamp) {
    sprintf(buffer, "%s %ld", buffer, timestamp);
}

bool buildInfluxWritePath(char* url, int max_size, const char *org, const char *bucket) {
    char buffer[256] = {0};
    int result = sprintf(buffer, "/api/v2/write?org=%s&bucket=%s&precision=s", org, bucket);
    strcat(url, buffer);
    return result < max_size;
}

bool buildInfluxAuthHeader(char* auth_header, int max_size, const char *token) {
    int result = sprintf(auth_header, "Authorization: Token %s", token);
    return result < max_size;
}

#ifdef CURL
void CurlInfluxDB(InfluxDBContext dbContext, char* lineProtocol, char* ip, int port) {
	
	CURL* curl_handle;
	CURLcode result;

	struct MemoryStruct chunk;
	chunk.memory = malloc(1);
	chunk.size = 0;

	curl_handle = curl_easy_init();
	if (curl_handle) {
	
		char url[256];
		sprintf(url, "http://%s:%d/api/v2/write?org=%s&bucket=%s&precision=s", ip, port, dbContext.org, dbContext.bucket);

		char auth_header[256];
		sprintf(auth_header, "Authorization: Token %s", dbContext.token);
		struct curl_slist *header = NULL;
		header = curl_slist_append(header, auth_header);
			

		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, header);
		curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
		curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, lineProtocol);
		//curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

		result = curl_easy_perform(curl_handle);
		if (result != CURLE_OK) {
			fprintf(stderr, "CURL error: %s\n", curl_easy_strerror(result));
		} else {
			//printf("%s", "Data sent to InfluxDB\n");
			//printf ("Data: %s\n", chunk.memory);
		}

		curl_easy_cleanup(curl_handle);
		free(chunk.memory);
	}
}
#endif

//Macro for unit test enable disable

int line_protocol_test() {
    char buffer[256];
    char bucket[10] = "myBucket";
    char tagKey[10] = "sensor";
    char tagValue[10] = "A0";
    char fieldKey[10] = "value";
    double fieldValue = 10.0;

    setMeasurement(buffer, bucket);
    addTag(buffer, tagKey, tagValue);
    addTag(buffer, "source", "instrumentacao");
    addField(buffer, fieldKey, fieldValue);
    addField(buffer, "tensao", 24.0f);
    addField(buffer, "corrente", 0.5f);
    addTimestamp(buffer, getEpochSeconds());
    
    printf("Line protocol message: %s\n", buffer);
    return 0;
}

