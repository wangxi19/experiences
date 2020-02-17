#ifndef CURL_HPP
#define CURL_HPP
#include <stdio.h>
#include <string.h>
#include <curl.h>
#include <stdlib.h>
#include <vector>
#include <string>

namespace CURLNS {
typedef struct __Bucket {
    __Bucket(size_t size) {
        this->size = size;
        ptr = (char*) malloc(size);
    }

    ~__Bucket() {
        free(ptr);
    }

    char* ptr{nullptr};
    size_t size{0};
    size_t readsz{0};
    size_t wrtsz{0};
} Bucket;

size_t writeCallBack(char* ptr, size_t /*size*/, size_t nmemb, void* userdata) {
    if (userdata == NULL) {
        return nmemb;
    }
    auto bucket = (Bucket*)userdata;
    if (bucket->wrtsz + nmemb > bucket->size) {
        //buffer is full, drop incoming data
        return nmemb;
    }

    memcpy(bucket->ptr + bucket->wrtsz, ptr, nmemb);
    bucket->wrtsz += nmemb;
    return nmemb;
}

size_t readCallBack(char* buffer, size_t size, size_t nitems, void* userdata) {
    auto bucket = (Bucket*)userdata;
    if (size * nitems == 0) {
        return 0;
    }

    if (bucket->readsz >= bucket->wrtsz) {
        return 0;
    }

    auto sz = (size * nitems) > (bucket->wrtsz - bucket->readsz) ? (bucket->wrtsz - bucket->readsz) : (size * nitems);
    memcpy(buffer, bucket->ptr + bucket->readsz, sz);
    bucket->readsz += sz;

    return sz;
}

enum HTTPMETHOD {
    HTTPGET,
    HTTPPOST,
    HTTPPUT
};

int perform(const char* url, HTTPMETHOD httpMethod, Bucket* outBucket, const char* data, size_t datalen, const std::vector<std::string>& headers, int to) {
    auto curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, url);

    struct curl_slist * slist{NULL};
    for (int i = 0; i < (int)headers.size(); i++) {
        slist = curl_slist_append(slist, headers[i].c_str());
    }
    if (slist != NULL) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    }

    switch (httpMethod) {
    case HTTPMETHOD::HTTPGET:
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
        break;
    case HTTPMETHOD::HTTPPOST:
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        break;
    case HTTPMETHOD::HTTPPUT:
        curl_easy_setopt(curl, CURLOPT_PUT, 1);
        break;
    default:
        break;
    }

    if (data != NULL && datalen > 0) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)datalen);
#if 0
        Bucket bucketread(datalen);
        memcpy(bucketread.ptr, data, datalen);
        bucketread.wrtsz = datalen;
        curl_easy_setopt(curl, CURLOPT_READDATA, &bucketread);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, readCallBack);
#else
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
#endif
    }

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, outBucket);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallBack);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, to);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    auto res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_slist_free_all(slist);

    return res;
}

int POST(const char* url, const char* data, size_t datalen, Bucket* outBucket = NULL, const std::vector<std::string>& headers = std::vector<std::string>{"Content-Type: application/json"}, long to = 3) {
    return perform(url, HTTPMETHOD::HTTPPOST, outBucket, data, datalen, headers, to);
}

int GET(const char* url, Bucket* outBucket = NULL, const std::vector<std::string>& headers = std::vector<std::string>(), long to = 2) {
    return perform(url, HTTPMETHOD::HTTPGET, outBucket, NULL, 0, headers, to);
}

}
#endif // CURL_HPP
