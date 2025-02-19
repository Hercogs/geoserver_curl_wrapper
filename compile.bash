
g++ --std=c++17 -o main exmaple.cpp geoserver_curl_wrapper.cpp  -lcurl \
    `pkg-config --cflags --libs libxml-2.0`

