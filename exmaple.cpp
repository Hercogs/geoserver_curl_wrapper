#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "geoserver_curl_wrapper.hpp"
#include "geoserver_custom_structs.hpp"

int main(int argc, char** argv)
{
    const char* hostname = "localhost";
    const int port = 8080;
    const char* username = "admin";
    const char* password = "geoserver";
    const int timeout_s = 3;

    bool status;

    // initlize "geoserver_curl_wrapper"
    status = geoserver_api::init("localhost", 8080, "admin", "geoserver", 3);
    if (!status) exit(EXIT_FAILURE);  // If failes, exit

    // 1. Create layer
    {
        char* http_response_body;
        long http_response_code;
    
        const char* layer_name = "example_layer1";
        const char* layer_title = "example_layer1";
        const char* postgis_table_name = "density_test";
        const char* filter = "density > 99";
        const char* workspace = "forestAI";
        const char* datastore = "postgis";
        bool advertised = true;
    
        status = geoserver_api::create_layer(layer_name, layer_title,
            postgis_table_name, filter, workspace, datastore, advertised);
        http_response_body = geoserver_api::get_http_response_body();
        http_response_code = geoserver_api::get_http_response_code();
    
        fprintf(stdout, "Creating layer - status: %d.\nresponse body: %s\n"
            "response code: %ld\n", status, http_response_body, http_response_code);
    }

    // 2. Create another layer
    {
        char* http_response_body;
        long http_response_code;
    
        const char* layer_name = "example_layer2";
        const char* layer_title = "example_layer2";
        const char* postgis_table_name = "density_test";
        const char* filter = NULL;
        const char* workspace = "forestAI";
        const char* datastore = "postgis";
        bool advertised = true;
    
        status = geoserver_api::create_layer(layer_name, layer_title,
            postgis_table_name, filter, workspace, datastore, advertised);
        http_response_body = geoserver_api::get_http_response_body();
        http_response_code = geoserver_api::get_http_response_code();
    
        fprintf(stdout, "Creating layer - status: %d.\nresponse body: %s\n"
            "response code: %ld\n", status, http_response_body, http_response_code);
    }

    // 3. Add style to example_layer1
    {
        char* http_response_body;
        long http_response_code;
    
        const char* layer_name = "example_layer1";
        const char* style_name = "density";
        const char* layer_workspace = "forestAI";
        const char* style_workspace = "forestAI";

        status = geoserver_api::add_style(layer_name, style_name, layer_workspace,
            style_workspace);
        http_response_body = geoserver_api::get_http_response_body();
        http_response_code = geoserver_api::get_http_response_code();
    
        fprintf(stdout, "Adding style - status: %d.\nresponse body: %s\n"
            "response code: %ld\n", status, http_response_body, http_response_code);
    }

    // 4. Add style to example_layer2
    {
        char* http_response_body;
        long http_response_code;
    
        const char* layer_name = "example_layer2";
        const char* style_name = "density";
        const char* layer_workspace = "forestAI";
        const char* style_workspace = "forestAI";

        status = geoserver_api::add_style(layer_name, style_name, layer_workspace,
            style_workspace);
        http_response_body = geoserver_api::get_http_response_body();
        http_response_code = geoserver_api::get_http_response_code();
    
        fprintf(stdout, "Adding style - status: %d.\nresponse body: %s\n"
            "response code: %ld\n", status, http_response_body, http_response_code);
    }


    // 5. Create layer group
    {
        char* http_response_body;
        long http_response_code;

        const char* layer_name = "layer_group3";
        const char* layer_title = "layer_group3";
        bool advertised = true;
        const char* layers[] = {"example_layer1", "example_layer2"};
        const char* layer_workspace = "forestAI";

        char* structure = geoserver_api::prepare_layer_group(layer_workspace,
            sizeof(layers) / sizeof(layers[0]), layers[0], layers[1]);
        if (!structure)
        {
            fprintf(stdout, "prepare_layer_group() failed");
            goto end;
        }

        status = geoserver_api::create_layer_group(layer_name, layer_title, structure,
           layer_workspace, advertised);
        http_response_body = geoserver_api::get_http_response_body();
        http_response_code = geoserver_api::get_http_response_code();
    
        fprintf(stdout, "Creating layer group - status: %d.\nresponse body: %s\n"
            "response code: %ld\n", status, http_response_body, http_response_code);

        if (structure) free(structure);
    }

    // 6. Get all layer names
    {
        char* http_response_body;
        long http_response_code;

        int num_layers = 0;
        char** layers = NULL;
        // status = geoserver_api::get_layers(NULL, &num_layers, &layers); // For all layers
        status = geoserver_api::get_layers("forestAI", &num_layers, &layers);

        printf("6\n");

        http_response_body = geoserver_api::get_http_response_body();
        http_response_code = geoserver_api::get_http_response_code();

        // fprintf(stdout, "Creating layer group - status: %d.\nresponse body: %s\n"
        //     "response code: %ld\n", status, http_response_body, http_response_code);
        
        fprintf(stdout, "Total num layers: %d\n", num_layers);
    
        if (status && layers)
        {
            for(size_t i=0; i < num_layers; i++)
            {
                fprintf(stdout, "\t%s\n", layers[i]);
            }
        }

        // Remember to free "layers" variable
        if (layers)
        {
            for(size_t i=0; i < num_layers; i++)
            {
                if (layers[i]) free(layers[i]);
            }
            free(layers);
        }
    }

    // Remember to free resources regarding "geoserver_curl_wrapper"
    end:
        fprintf(stdout, "Cleaning up!\n");
        geoserver_api::cleanup();


    return 0;
}