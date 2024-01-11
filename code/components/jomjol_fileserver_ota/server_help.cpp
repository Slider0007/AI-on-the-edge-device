#include "server_help.h"
#include "../../include/defines.h"

#include <stdio.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <dirent.h>
#ifdef __cplusplus
}
#endif

#include "esp_http_server.h"
#include "esp_err.h"
#include "esp_log.h"
#include "miniz.h"

#include "Helper.h"
#include "ClassLogFile.h"


static const char *TAG = "SERVER_HELP";

std::string SUFFIX_ZW = "_0xge";
char scratch[SERVER_HELPER_SCRATCH_BUFSIZE];


bool endsWith(std::string const &str, std::string const &suffix)
{
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}


esp_err_t send_file(httpd_req_t *req, std::string filename)
{
    FILE *fd = fopen(filename.c_str(), "r");
    if (!fd) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "send_file: Failed to read file: " + filename);
        /* Respond with 404 Not Found */
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, get404());
        return ESP_FAIL;
    }

    /* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
    // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
    setvbuf(fd, NULL, _IOFBF, 512);

    ESP_LOGD(TAG, "Sending file: %s", filename.c_str());
//    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    /* For all files with the following file extention tell
       the webbrowser to cache them for 24h */
    if (endsWith(filename, ".html") ||
        endsWith(filename, ".htm") ||
        endsWith(filename, ".css") ||
        endsWith(filename, ".js") ||
        endsWith(filename, ".map") ||
        endsWith(filename, ".jpg") ||
        endsWith(filename, ".jpeg") ||
        endsWith(filename, ".ico") ||
        endsWith(filename, ".gif") ||
        endsWith(filename, ".svg") ||
        endsWith(filename, ".png"))
    {
    	if (filename == "/sdcard/html/setup.html") {    
            httpd_resp_set_hdr(req, "Clear-Site-Data", "\"*\"");
        }
        else {
            httpd_resp_set_hdr(req, "Cache-Control", "max-age=86400");
        }
    }

    set_content_type_from_file(req, filename.c_str());

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *chunk = scratch;
    size_t chunksize;
    do {
        /* Read file in chunks into the scratch buffer */
        chunksize = fread(chunk, 1, SERVER_HELPER_SCRATCH_BUFSIZE, fd);

        /* Send the buffer contents as HTTP response chunk */
        if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
            fclose(fd);
            std::string msg_txt = "send_file: Failed to send file: " + filename;
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, msg_txt);
            /* Abort sending file */
            httpd_resp_sendstr_chunk(req, NULL);
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg_txt.c_str());
            return ESP_FAIL;
        }

        /* Keep looping till the whole file is sent */
    } while (chunksize != 0);

    /* Close file after sending complete */
    fclose(fd);
    ESP_LOGD(TAG, "File sending complete");    
    return ESP_OK;    
}


/* Copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path) */
const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest) {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash) {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize) {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }

    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    return dest + base_pathlen;
}


/* Set HTTP response content type according to file extension */
esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".pdf")) {
        return httpd_resp_set_type(req, "application/pdf");
    } else if (IS_FILE_EXT(filename, ".html")) {
        return httpd_resp_set_type(req, "text/html");
    } else if (IS_FILE_EXT(filename, ".jpeg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    } else if (IS_FILE_EXT(filename, ".jpg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    } else if (IS_FILE_EXT(filename, ".ico")) {
        return httpd_resp_set_type(req, "image/x-icon");
    } else if (IS_FILE_EXT(filename, ".js")) {
        return httpd_resp_set_type(req, "text/javascript");
    } else if (IS_FILE_EXT(filename, ".css")) {
        return httpd_resp_set_type(req, "text/css");
    }
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}


std::string unzip_new(std::string _in_zip_file, std::string _target_zip, std::string _target_bin, std::string _main, bool _initial_setup)
{
    int i, sort_iter;
    mz_bool status;
    size_t uncomp_size;
    mz_zip_archive zip_archive;
    void* p;
    char archive_filename[64];
    std::string zw, ret = "";
    std::string directory = "";

    ESP_LOGD(TAG, "miniz.c version: %s", MZ_VERSION);
    ESP_LOGD(TAG, "Zipfile: %s", _in_zip_file.c_str());

    // Now try to open the archive.
    memset(&zip_archive, 0, sizeof(zip_archive));
    status = mz_zip_reader_init_file(&zip_archive, _in_zip_file.c_str(), 0);
    if (!status) {
        ESP_LOGD(TAG, "mz_zip_reader_init_file() failed");
        return ret;
    }

    // Get and print information about each file in the archive.
    int numberoffiles = (int)mz_zip_reader_get_num_files(&zip_archive);
    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "Files to be extracted: " + std::to_string(numberoffiles));

    sort_iter = 0;

    memset(&zip_archive, 0, sizeof(zip_archive));
    status = mz_zip_reader_init_file(&zip_archive, _in_zip_file.c_str(), sort_iter ? MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY : 0);
    if (!status) {
        ESP_LOGD(TAG, "mz_zip_reader_init_file() failed");
        return ret;
    }

    for (i = 0; i < numberoffiles; i++) {
        mz_zip_archive_file_stat file_stat;
        mz_zip_reader_file_stat(&zip_archive, i, &file_stat);
        sprintf(archive_filename, file_stat.m_filename);
        
        if (!file_stat.m_is_directory) {
        // Try to extract all the files to the heap.
        p = mz_zip_reader_extract_file_to_heap(&zip_archive, archive_filename, &uncomp_size, 0);
            if (!p) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "mz_zip_reader_extract_file_to_heap() failed on file " + std::string(archive_filename));
                mz_zip_reader_end(&zip_archive);
                return ret;
            }
        
            // Save to File
            zw = std::string(archive_filename);
            ESP_LOGD(TAG, "archive filename: %s", zw.c_str());

            if (toUpper(zw) == "FIRMWARE.BIN") {
                zw = _target_bin + zw;
                ret = zw;
            }
            else {
                std::string _dir = getDirectory(zw);
                if ((_dir == "config-initial") && !_initial_setup) {
                    continue;
                }
                else {
                    _dir = "config";
                    std::string _s1 = "config-initial";
                    FindReplace(zw, _s1, _dir);
                }

                if (_dir.length() > 0) {
                    zw = _main + zw;
                }
                else {
                    zw = _target_zip + zw;
                }
            }
        
            std::string filename_zw = zw + SUFFIX_ZW;

            ESP_LOGI(TAG, "Unzip file: %s -> temp. filename: %s", zw.c_str(), filename_zw.c_str());

            std::string folder = filename_zw.substr(0, filename_zw.find_last_of('/'));
            MakeDir(folder);

            // extrahieren in temp file
            DeleteFile(filename_zw);

            FILE* fpTargetFile = fopen(filename_zw.c_str(), "wb");
            uint writtenbytes = fwrite(p, 1, (uint)uncomp_size, fpTargetFile);
            fclose(fpTargetFile);
            
            bool isokay = true;

            if (writtenbytes == (uint)uncomp_size) {
                isokay = true;
            }
            else {
                isokay = false;
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "ERROR in writting extracted file (function fwrite) extracted file \"" +
                        std::string(archive_filename) + "\", size " + std::to_string(uncomp_size));
            }

            DeleteFile(zw);
            if (!isokay)
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "ERROR in fwrite \"" + std::string(archive_filename) + "\", size " + std::to_string(uncomp_size));
            
            isokay = isokay && RenameFile(filename_zw, zw);

            if (!isokay)
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "ERROR in Rename \"" + filename_zw + "\" to \"" + zw);

            if (isokay) {
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Successfully extracted file \"" + std::string(archive_filename) + "\", size " + std::to_string(uncomp_size));
            }
            else {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "ERROR in extracting file \"" + std::string(archive_filename) + "\", size " + std::to_string(uncomp_size));
                ret = "ERROR";
            }
            mz_free(p);
        }
    }

    // Close the archive, freeing any resources it was using
    mz_zip_reader_end(&zip_archive);

    ESP_LOGD(TAG, "Success.");
    return ret;
}


void unzip(std::string _in_zip_file, std::string _target_directory){
    int i, sort_iter;
    mz_bool status;
    size_t uncomp_size;
    mz_zip_archive zip_archive;
    void* p;
    char archive_filename[64];
    std::string zw;
    //static const char* s_Test_archive_filename = "testhtml.zip";

    ESP_LOGD(TAG, "miniz.c version: %s", MZ_VERSION);
    ESP_LOGD(TAG, "Zipfile: %s", _in_zip_file.c_str());
    ESP_LOGD(TAG, "Target Dir: %s", _target_directory.c_str());

    // Now try to open the archive.
    memset(&zip_archive, 0, sizeof(zip_archive));
    status = mz_zip_reader_init_file(&zip_archive, _in_zip_file.c_str(), 0);
    if (!status) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "mz_zip_reader_init_file() failed");
        return;
    }

    // Get and print information about each file in the archive.
    int numberoffiles = (int)mz_zip_reader_get_num_files(&zip_archive);
    for (sort_iter = 0; sort_iter < 2; sort_iter++) {
        memset(&zip_archive, 0, sizeof(zip_archive));
        status = mz_zip_reader_init_file(&zip_archive, _in_zip_file.c_str(), sort_iter ? MZ_ZIP_FLAG_DO_NOT_SORT_CENTRAL_DIRECTORY : 0);
        if (!status) {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "mz_zip_reader_init_file() failed");
            return;
        }

        for (i = 0; i < numberoffiles; i++) {
            mz_zip_archive_file_stat file_stat;
            mz_zip_reader_file_stat(&zip_archive, i, &file_stat);
            sprintf(archive_filename, file_stat.m_filename);
 
            // Try to extract all the files to the heap.
            p = mz_zip_reader_extract_file_to_heap(&zip_archive, archive_filename, &uncomp_size, 0);
            if (!p) {
                LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "mz_zip_reader_extract_file_to_heap() failed");
                mz_zip_reader_end(&zip_archive);
                return;
            }

            // Save to File.
            zw = std::string(archive_filename);
            zw = _target_directory + zw;
            ESP_LOGD(TAG, "File to extract: %s", zw.c_str());
            FILE* fpTargetFile = fopen(zw.c_str(), "wb");
            fwrite(p, 1, (uint)uncomp_size, fpTargetFile);
            fclose(fpTargetFile);

            ESP_LOGD(TAG, "Successfully extracted file \"%s\", size %u", archive_filename, (uint)uncomp_size);
            //ESP_LOGD(TAG, "File data: \"%s\"", (const char*)p);

            // We're done.
            mz_free(p);
        }

        // Close the archive, freeing any resources it was using
        mz_zip_reader_end(&zip_archive);
    }

    ESP_LOGD(TAG, "Success");
}
