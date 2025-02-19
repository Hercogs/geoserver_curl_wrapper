
#include <iostream>
#include <curl/curl.h>
#include <string.h>
#include <string>





size_t write_callback(char* content, size_t size, size_t nmemb, void* userdata)
{
    ((std::string*)userdata)->append((char*)content, nmemb);
    return size * nmemb;
}

int main(int argc, char** argv)
{
    const char* url {"http://localhost:8080/geoserver/rest/"\
        "workspaces/forestAI/datastores/postgis/featuretypes"}; //featuretype
    int timeout {5};

    // headers = {"Content-type": "application/xml"}

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-type: application/xml");

    const char* data = 
        "<featureType>"
            "<name>C++ layer</name>"
            "<nativeName>density_test</nativeName>"
            "<title>c+++_layer</title>"
            "<srs>EPSG:3059</srs>"
        "</featureType>";



    CURL* curl = curl_easy_init();
    if (!curl)
    {
        printf("curl_easy_init(): failed\n");
        return -1;
    }

    CURLcode res;
    std::string result;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1l);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
    curl_easy_setopt(curl, CURLOPT_USERPWD, "admin:geoserver");

    res = curl_easy_perform(curl);
    res = curl_easy_perform(curl);
    res = curl_easy_perform(curl);
    if (res) 
    {
        printf("curl_easy_perform(): failed\n");
        return -1;
    }


    printf("Output: \n %s\n", result.c_str());
    long http_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    printf("Http code: %ld\n", http_code);

   


    curl_easy_cleanup(curl);
    return 0;
}