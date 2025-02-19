#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "geoserver_curl_wrapper.hpp"
#include "geoserver_custom_structs.hpp"

namespace geoserver_api
{
    // global variables
    static char geoserver_url[128] = {0};
    static CURL* curl = {NULL};
    static struct data_clb_pointer<char> curl_response_body;

    bool init(const char* hostname, const int port,
        const char* username, const char* password, const int timeout_s)
    {
        int j; // for snprintf

        if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
            fprintf(stderr, "curl_global_init() failed\n");
            return 1;
        }

        curl = curl_easy_init();
        if (!curl)
        {
            fprintf(stderr, "curl_easy_init(): failed\n");
            return false;
        }

        j = snprintf(
            geoserver_url, sizeof(geoserver_url),
            "http://%s:%d/geoserver/rest", hostname, port
        );
        if (j >= sizeof(geoserver_url) || j < 0)
        {
            fprintf(stderr, "snprintf(): geoserver_url failed\n");
            return false;
        }

        // Set global url options
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_s);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout_s);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_body_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_response_body);
        int len = strlen(username) + strlen(password) + 1 + 1;
        char user_admin[len] = {0};
        j = snprintf(user_admin, len, "%s:%s", username, password);
        if (j >= len || j < 0)
        {
            fprintf(stderr, "snprintf(): user_admin failed '%d'\n", j);
            return false;
        }
        curl_easy_setopt(curl, CURLOPT_USERPWD, user_admin);


        return true;
    }

    void cleanup()
    {   
        if (curl)
        {
            curl_easy_cleanup(curl);
            curl = NULL;
        }
        if (curl_response_body.p)
        {
            free(curl_response_body.p);
            curl_response_body.p = NULL;
            curl_response_body.size = 0;
        }        
    }

    CURL* get_curl_handle()
    {
        return curl;
    }

    long get_http_response_code()
    {
        long http_code;
        CURLcode code = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (code != CURLE_OK)
            fprintf(stderr, "Getting http code failed\n");
        return http_code;
    }

    char* get_http_response_body()
    {
        return curl_response_body.p;
    }

    size_t curl_body_callback(const char* const content, size_t size, size_t nmemb, void* user_data)
    {
        size_t received_size = size * nmemb;
        struct data_clb_pointer<char>* storage = (struct data_clb_pointer<char>*)user_data;

        size_t current_content_len = 0;
        if (storage->p)
        {
            current_content_len = strlen(storage->p);
        }

        size_t needed_string_len = current_content_len + received_size + 1;
        if (needed_string_len > storage->size) 
        {
            char* tmp = (char*)realloc(storage->p, needed_string_len);
            if (tmp == NULL) return 0;
            storage->p = tmp;
            storage->size = needed_string_len;
            storage->reset(current_content_len);
        }

        // Append data
        memcpy(storage->p + current_content_len, content, received_size);

        return received_size;
    }

    bool create_layer(const char* layer_name, const char* layer_title,
        const char* postgis_table_name, const char* filter,
        const char* workspace, const char* datastore,
        const bool advertised)
    {
        size_t j;
        char request_url[256] = {0};
        struct curl_slist* curl_header = NULL;

        curl_response_body.reset(); // Reset storage

        j = snprintf(request_url, sizeof(request_url),
            "%s/workspaces/%s/datastores/%s/featuretypes", geoserver_url, workspace,
            datastore
        );
        if (j < 0 || j > sizeof(request_url))
        {
            fprintf(stderr, "create_layer() requets url too short, need %ld bytes\n", j);
            return false;
        }
        
        curl_easy_setopt(curl, CURLOPT_URL, request_url);

        // Code data
        const char* data_template = 
        "<featureType>"
            "<name>%s</name>"
            "<nativeName>%s</nativeName>"
            "<title>%s</title>"
            "<srs>EPSG:3059</srs>"
            "%s" // for filter tag
            "<advertised>%s</advertised>"
        "</featureType>";

        char filter_tag[512] = {0};
        if (filter)
        {
            j = snprintf(filter_tag, sizeof(filter_tag),
            "<cqlFilter>%s</cqlFilter>", filter);
        }
        if(j < 0 || j >= sizeof(filter_tag))
        {
            fprintf(stderr, "create_layer() filter_tag too short, need %ld bytes\n", j);
            return false;
        }

        char data[1024] = {0};
        j = snprintf(data, sizeof(data), data_template, layer_name,
            postgis_table_name, layer_title, filter_tag,
            (advertised) ? "true" : "false"
        );

        if(j < 0 || j >= sizeof(data))
        {
            fprintf(stderr, "create_layer() data too short, need %ld bytes\n", j);
            return false;
        }

        const char* header="Content-type: application/xml";
        curl_header = curl_slist_append(curl_header, header); // Remeber to free
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_header);

        // fprintf(stderr, "SEND: %s\n", data);
        curl_easy_setopt(curl, CURLOPT_POST, 1l); // Set as POST request
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

        CURLcode res;
        res = curl_easy_perform(curl);

        if (curl_header)
        {
            curl_slist_free_all(curl_header);
        }

        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed with code: %d\n", res);
        }
        return !res; 
    }

    bool add_style(const char* layer_name, const char* style_name,
        const char* layer_workspace, const char* style_workspace)
    {
        size_t j;
        char request_url[256] = {0};
        struct curl_slist* curl_header = NULL;

        curl_response_body.reset(); // Reset storage

        j = snprintf(request_url, sizeof(request_url),
            "%s/layers/%s:%s.xml", geoserver_url,
            layer_workspace, layer_name
        );
        if (j < 0 || j > sizeof(request_url))
        {
            fprintf(stderr, "add_style() requets url too short, need %ld bytes\n", j);
            return false;
        }
        curl_easy_setopt(curl, CURLOPT_URL, request_url);

        // Code data
        const char* data_template = 
        "<layer>"
            "<defaultStyle>"
                "<name>%s:%s</name>"
            "</defaultStyle>"
        "</layer>";

        char data[1024] = {0};
        j = snprintf(data, sizeof(data), data_template,
            style_workspace, style_name
        );

        if(j < 0 || j >= sizeof(data))
        {
            fprintf(stderr, "add_style() data too short, need %ld bytes\n", j);
            return false;
        }

        const char* header="Content-type: application/xml";
        curl_header = curl_slist_append(curl_header, header); // Remeber to free
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_header);

        // curl_easy_setopt(curl, CURLOPT_PUT, 1l); // Set as POST request
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

        CURLcode res;
        res = curl_easy_perform(curl);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, NULL); // Reset CURLOPT_CUSTOMREQUEST

        if (curl_header)
        {
            curl_slist_free_all(curl_header);
        }

        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed with code: %d\n", res);
        }
        return !res; 
    }

    char* prepare_layer_group(const char* workspace, int number_layers, ...)
    {
        bool success{false};
        va_list args;
        va_start(args, number_layers);

        const char layer_template[] =
            "<published type=\"layer\">"
                "<name>%s:%s</name>"
            "</published>";
                
        size_t layer_size = (sizeof(layer_template) + 64) * number_layers + 1; // +1 for \0
        char single_layer[sizeof(layer_template) + 64] = {0}; // for single layer

        char* layers = (char*)malloc(layer_size); // For layer group
        if (layers == NULL) 
        {
            fprintf(stdout, "prepare_layer_group(): malloc failed\n");
            goto end;
        }
        
        memset(layers, 0, layer_size); // Reset to \0 all array

        for(size_t i=0; i < number_layers; i++)
        {
            size_t j = snprintf(single_layer, sizeof(single_layer), layer_template,
                workspace, va_arg(args, char*));
            if (j < 0 || j >= sizeof(single_layer))
            {
                fprintf(stderr, "prepare_layer_group() single_layer too short, need %ld bytes\n", j);
                goto end;
            }
            strcat(layers, single_layer);
        }
        
        success = true;
        end:
            va_end(args);
            if (success) return layers;
            if (layers) free(layers);
            return NULL;
    }

    bool create_layer_group(const char* layer_group_name, const char* layer_title,
        const char* const layer_structure, const char* workspace, const bool advertised)
    {
        size_t j;
        char request_url[256] = {0};
        struct curl_slist* curl_header = NULL;

        curl_response_body.reset(); // Reset storage

        j = snprintf(request_url, sizeof(request_url),
            "%s/workspaces/%s/layergroups", geoserver_url,
            workspace
        );
        if (j < 0 || j > sizeof(request_url))
        {
            fprintf(stderr, "create_layer_group() requets url too short, need %ld bytes\n", j);
            return false;
        }
        curl_easy_setopt(curl, CURLOPT_URL, request_url);

        // Code data
        const char data_template[] = 
        "<layerGroup>"
            "<name>%s</name>"
            "<title>%s</title>"
            "<advertised>%s</advertised>"
            "<workspace>"
                "<name>%s</name>"
            "</workspace>"
            "<publishables>%s</publishables>"
        "</layerGroup>";

        char data[1024] = {0};
        j = snprintf(data, sizeof(data), data_template, layer_group_name,
            layer_title, (advertised) ? "true" : "false", workspace,
            layer_structure  
        );

        if(j < 0 || j >= sizeof(data))
        {
            fprintf(stderr, "create_layer_group() data too short, need %ld bytes\n", j);
            return false;
        }

        const char* header="Content-type: application/xml";
        curl_header = curl_slist_append(curl_header, header); // Remeber to free
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_header);

        // fprintf(stderr, "SEND: %s\n", data);
        curl_easy_setopt(curl, CURLOPT_POST, 1l); // Set as POST request
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

        CURLcode res;
        res = curl_easy_perform(curl);

        if (curl_header)
        {
            curl_slist_free_all(curl_header);
        }

        if (res != CURLE_OK)
        {
            fprintf(stderr, "create_layer_group() curl_easy_perform failed: %d\n", res);
        }
        return !res; 
    }

    bool get_layers(const char* workspace, int* num_layers, char*** layer_names)
    {
        bool status {false};
        // Check inputs
        if (*layer_names != NULL)
        {
            fprintf(stderr, "get_layers() layer_names parameter not NULL");
            return false;
        }
        size_t j;
        char request_url[256] = {0};
        struct curl_slist* curl_header = NULL;

        curl_response_body.reset(); // Reset storage

        j = snprintf(request_url, sizeof(request_url),
            "%s/layers.xml", geoserver_url
        );
        if (j < 0 || j > sizeof(request_url))
        {
            fprintf(stderr, "get_layers() requets url too short, need %ld bytes\n", j);
            return false;
        }

        CURLcode res;
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL);
        res = curl_easy_setopt(curl, CURLOPT_URL, request_url);
        res = curl_easy_setopt(curl, CURLOPT_POST, 0l); // Set as GET request
        // res = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            fprintf(stderr, "get_layers() curl_easy_perform failed: %d\n", res);
            return !res;
        }

        // Parse XML response
        xmlDocPtr doc = xmlReadMemory(curl_response_body.p, strlen(curl_response_body.p),
            NULL, NULL, 0);

        if (doc == NULL)
        {
            fprintf(stderr, "get_layers() xmlReadMemory failed");
            return false;
        }

        xmlNodePtr root = xmlDocGetRootElement(doc);
        if (root == NULL)
        {
            fprintf(stderr, "get_layers() xmlDocGetRootElement failed");
            xmlFreeDoc(doc);
            return false;
        }  

        *num_layers = 0;

        // Lambda for filtering content
        auto filter_content_lambde = [](const char* in, char* out) -> bool
        {
            int i = 0;
            while(*in != 0)
            {
                if (isalnum(*in)|| (*in)==':')
                {
                    i++;
                    if (i >= NAME_MAX) return false;
                    *out = *in;
                    out++;
                }
                in++;
            }
            return true;
        };

        for(xmlNodePtr node = root->children; node; node = node->next)
        {
            if(node->type == XML_ELEMENT_NODE)
            {
                char* content = (char*)xmlNodeGetContent(node);
                // char* name = (char*)node->name;
                char filter_content[NAME_MAX] = {0};

                if(!content)
                {
                    fprintf(stderr, "get_layers(): xmlNodeGetContent failed\n");
                    goto cleanup;
                }

                bool filt = filter_content_lambde(content, filter_content);
                xmlFree(content);  // Important to free
                if(!filt)
                {
                    fprintf(stderr, "get_layers(): filter_content_lambde failed\n");
                    goto cleanup;
                }

                if (workspace)
                {
                    if (strncmp(workspace, filter_content, strlen(workspace)) != 0)
                    {
                        continue;
                    }
                }

                // For first layer dimension
                char** tmp_1_dim = (char**)realloc(*layer_names, sizeof(char*) * ((*num_layers)+1));
                if (!tmp_1_dim)
                {
                    // free all resource
                    fprintf(stderr, "Failed realloc\n");
                    goto cleanup;
                }
                *layer_names = tmp_1_dim;
                
                // For second layer dimension
                char* tmp_2_dim = (char*)malloc(strlen(filter_content)+1);
                if(!tmp_2_dim)
                {
                    // free all resource
                    fprintf(stderr, "Failed malloc\n");
                    goto cleanup;
                }
                memset(tmp_2_dim, 0 , strlen(filter_content)+1);
                strcpy(tmp_2_dim, filter_content);
                (*layer_names)[*num_layers] = tmp_2_dim;
                (*num_layers)++;
            }
        }
        status = true;
        if (doc) xmlFreeDoc(doc);
        xmlCleanupParser();

        return status;
        
        cleanup:
            if (doc) xmlFreeDoc(doc);
            xmlCleanupParser();

            if (layer_names)
            {
                for(size_t i=0; i < *num_layers; i++)
                {
                    if (layer_names[i]) free(layer_names[i]);
                }
                free(layer_names);
            }

            return status;
    }

} // end: namespace geoserver_api




