#ifndef GEOSERVER_CURL_WRAPPER_H
#define GEOSERVER_CURL_WRAPPER_H

#include <curl/curl.h>

/** This curl wrapper allows to interact easily with geoserver.
 * These functions expects, that all layers are made in workspaces
 * in Geoserver. More info about Geoserver RETS API:
 * https://docs.geoserver.org/2.25.x/en/user/rest/api/index.html * 
 */

namespace geoserver_api
{
    /**
     * @brief Function to initialize "geoserver_curl_wrapper" lib.
     * You must call "cleanup()" function aferwards to free resource.
     * 
     * @param hostname Geoserver hostname
     * @param port Geoserver port number
     * @param username Geoserver username with r/w access
     * @param password Geoserver password with r/w access
     * @param timeout_s CURL timeout in seconds
     * 
     * @returns boolean to indicate wether success or not
     */
    bool init(const char* hostname, const int port,
        const char* username, const char* password, const int timeout_s);

    /**
     * @brief Clean allocated resources for "geoserver_curl_wrapper" lib
     */
    void cleanup();

    /**
     * @brief Function to get curl handle for detailed operations.
     * It allows to create custom requests if needed.
     * 
     * @returns CURL handle
     */
    CURL* get_curl_handle();

    /**
     * @brief Get http response code from last request
     * 
     * @returns http code or 0 if no code
     */
    long get_http_response_code();

    /**
     * @brief Get http response body from last request.If no body
     * is available, returns NULL.
     * 
     * @returns response body or NULL if no body
     */
    char* get_http_response_body();
     
    /**
     * @brief Function to create layer in Geoserver. It expects that
     * workspace and datastore already exists in Geoserver. Expected 
     * datastore type is Postgis database.
     * 
     * @param layer_name name of the layer
     * @param layer_title created layer title
     * @param postgis_table_name table name from Postgis database
     * @param filter CQL filter, if needed
     * @param workspace workspace for datastore and layer
     * @param datastore datastore name
     * @param advertised wether to advertize layer
     * 
     * @returns boolean to indicate wether successful function call or not.
     * Use "get_http_response_code()" function to check HTTP status code. 201 on success
     */
    bool create_layer(const char* layer_name, const char* layer_title,
        const char* postgis_table_name, const char* filter=NULL,
        const char* workspace="forestAI", const char* datastore="postgis",
        const bool advertised=true);
    
    /**
     * @brief Function to add style to layer. Layer and style must exists
     * in Geoserver inside some workspace.
     * 
     * @param layer_name name of the layer
     * @param style_name name of the style
     * @param layer_workspace name of the layer workspace
     * @param style_workspace name of the style workspace
     * 
     * @returns boolean to indicate wether successful function call or not.
     * Use "get_http_response_code()" function to check HTTP status code. 200 on success
     */
    bool add_style(const char* layer_name, const char* style_name,
        const char* layer_workspace="forestAI", const char* style_workspace="forestAI");
    
    /**
     * @brief This function is used to prepare layer structure for layer
     * goup creation. 
     * 
     * @param workspace workspace name for layers to be used
     * @param number_layers number of layers passed in this function
     * @param ... layer names without workspace
     * 
     * @returns pointer to C string which contains layer group structure or
     * NULL if failed. Remeber to free pointer.
     */
    char* prepare_layer_group(const char* workspace, int number_layers, ...);

    /**
     * @brief Function to create layer group - combine exactly 2 layers in one.
     * 
     * @param layer_group_name name of the layer group to be made
     * @param layer_title title of the layer group to be made
     * @param layer_structure layer structure for layers to be joined. Use function
     * "prepare_layer_group()" to create this
     * @param workspace name of the workspace where layer group is made
     * @param advertised wether to advertize layer
     * 
     * @returns boolean to indicate wether successful function call or not.
     * Use "get_http_response_code()" function to check HTTP status code. 201 on success
     */
    bool create_layer_group(const char* layer_group_name, const char* layer_title,
        const char* const layer_structure, const char* workspace="forestAI",
        const bool advertised=true);

    
    /**
     * @brief Callback function for curl request to store response body. 
     * For internal use only.
     * 
     * @param content callback function content
     * @param size size of each data elements in bytes
     * @param nmemb number of data elements
     * @param user_data pointer to user data variable 
     * 
     * @returns size of bytes to be received
     */
    size_t curl_body_callback(const char* const content, size_t size,
        size_t nmemb, void* user_data);
    

    /**
     * @brief Gett all layers from Geoserve rin form of {worksapce}:{layername}
     * 
     * @param workspace determine which workspace layers to return.
     * If NULL, returns all layers.
     * @param num_layers storage for number of layers returned
     * @param layer_names storage for actual layer names which are returned.
     * Remember to free this pointer in 1st and 2nd dimension.
     * 
     * @returns boolean to indicate wether successful function call or not.
     * Use "get_http_response_code()" function to check HTTP status code. 200 on success
     */
    bool get_layers(const char* workspace, int* num_layers, char*** layer_names);
   








} // end: namespace geoserver_api

#endif