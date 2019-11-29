/* SPIFFS Image Generation on Build Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "mbedtls/md5.h"

static const char *TAG = "SPIFFSGEN";
static const char *PATH_HELLO_TEXT = "/spiffs/hello.txt";
static const char *PATH_CONFIG_JSON = "/spiffs/config.json";

/* Common utilities */
static char *read_file(const char *path)
{
    int len;
    struct stat st;
    char *buf, *p;

    FILE* fd = fopen(path, "r");
    if (fd == NULL) {
        ESP_LOGE(TAG, "Failed to open file");
        return NULL;
    }
	
    if (fstat(fileno(fd), &st)){
		ESP_LOGE(TAG, "Failed to stat file");
		return NULL;
	}
	
    if (!(buf = p = malloc(st.st_size + 1))){
        ESP_LOGE(TAG, "Failed to malloc");
        return NULL;		
	}

    while ((len = fread(p, 1, 1024,fd)) > 0)
        p += len;
    fclose(fd);

    if (len < 0)
    {
        free(buf);
        return NULL;
    }

    *p = '\0';
    return buf;
}

static void read_hello_txt(const char *path)
{
    // Display the read contents from the file
    ESP_LOGI(TAG, "Read from hello.txt: %s", read_file(path));
}

static void read_config_json(const char *path)
{
    // Display the read contents from the file
    ESP_LOGI(TAG, "Read from config.json: %s", read_file(path));
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

	char partition_name[] = { 'f', 's', '_', '0', '\0' };

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = partition_name,
      .max_files = 8,
      .format_if_mount_failed = true
    };

    ESP_LOGI(TAG, "Loading config from partition %s", partition_name);
	
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(partition_name, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    /* The following calls demonstrate reading files from the generated SPIFFS
     * image. The images should contain the same files and contents as the spiffs_image directory.
     */

    // Read and display the contents of a small file (hello.txt or config.json)
    read_hello_txt(PATH_HELLO_TEXT);
	read_config_json(PATH_CONFIG_JSON);

    // All done, unmount partition and disable SPIFFS
    esp_vfs_spiffs_unregister(NULL);
    ESP_LOGI(TAG, "SPIFFS unmounted");
}