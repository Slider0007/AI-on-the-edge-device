#ifndef CIMAGEJPG_H
#define CIMAGEJPG_H

#include "../../include/defines.h"

#include <string>
#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <esp_http_server.h>

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "CImageLockGuard.h"
#include "CImage.h"

class CImage; // Forward declaration

/**
 * @brief Class representing a JPG image
 *
 * Provides functionalities for managing and processing JPG images
 */
class CImageJpg
{
    friend class CImageLockGuard;

  private:
    mutable SemaphoreHandle_t imageMutex;
    mutable TaskHandle_t lockingTask = NULL;
    mutable int lockCount = 0;

    bool lock() const;
    void unlock() const;

    std::string name;
    int imgDataSize;
    uint8_t *imgData;

    void freeImageData();

  public:
    /**
     * @brief Constructor: Default
     */
    CImageJpg();

    /**
     * @brief Constructor to create an image object with data
     * @param objName Name of the image
     * @param size Size of the image data
     * @param data Pointer to the image data (optional, default: nullptr)
     */
    CImageJpg(std::string objName, int size, const uint8_t *data = nullptr);

    /**
     * @brief Constructor: Create an image object from a file
     * @param objName Name of the image
     * @param filename Path to the JPEG file
     */
    CImageJpg(std::string objName, const std::string &filename);

    /**
     * @brief Copy constructor
     * @param other Another CImageJpg object to copy from
     */
    CImageJpg(const CImageJpg &other);

    /**
     * @brief Copy assignment operator
     * @param other Another CImageJpg object to copy from
     * @return Reference to the current object
     */
    CImageJpg &operator=(const CImageJpg &other);

    /**
     * @brief Move constructor
     * @param other Another CImageJpg object to move from
     */
    CImageJpg(CImageJpg &&other) noexcept;

    /**
     * @brief Move assignment operator
     * @param other Another CImageJpg object to move from
     * @return Reference to the current object
     */
    CImageJpg &operator=(CImageJpg &&other) noexcept;

    /**
     * @brief Destructor
     */
    ~CImageJpg();

    /**
     * @brief Updates existing image data from a JPG buffer
     * @param newData Pointer to the new image data
     * @param newSize Size of the new image data
     * @param updateContainerSize If true, updates existing image data size (default: false)
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t updateImageDataFromJpgBuffer(const uint8_t *newData, int newSize, bool updateContainerSize = false);

    /**
     * @brief Updates image data from a JPEG file
     * @param filename Path to the JPEG file
     * @param updateContainerSize If true, updates existing image data size (default: false)
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t updateImageDataFromJpgFile(const std::string &filename, bool updateContainerSize = false);

    /**
     * @brief Loads JPG image from memory
     * @param data Pointer to the image data
     * @param size Size of the image data
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t loadJpgFromMemory(const void *data, int size);

    /**
     * @brief Saves the image to a file
     * @param filename Path where the image should be saved
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t saveJpgToFile(const std::string &filename);

    /**
     * @brief Sends the JPG image to an HTTP client
     * @param req Pointer to the HTTP request
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t sendJpgToHttp(httpd_req_t *req);

    /**
     * @brief Gets the name of the image
     * @return Image name
     */
    std::string getName() const { return name; }

    /**
     * @brief Sets the name of the image
     * @param objName New name of the image
     */
    void setName(std::string objName) { name = objName; }

    /**
     * @brief Checks if the image is valid
     * @return True if valid, false otherwise
     */
    bool isValid() const;

    /**
     * @brief Gets the image data size
     * @return Image data size in bytes
     */
    int getImgDataSize() const { return imgDataSize; }

    /**
     * @brief Gets a pointer to the image data
     * @return Pointer to the image data
     */
    uint8_t *getImgData();

    /**
     * @brief Gets a constant pointer to the image data
     * @return Constant pointer to the image data
     */
    const uint8_t *getImgData() const;

    /**
     * @brief Sets the image data size
     * @param size New image data size
     */
    void setImgDataSize(int size) { imgDataSize = size; }
};

#endif // CIMAGEJPG_H