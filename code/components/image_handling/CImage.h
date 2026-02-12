#ifndef CIMAGE_H
#define CIMAGE_H

#include "../../include/defines.h"

#include <string>
#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <esp_http_server.h>

#include "stb/stb_image.h"
#include "stb/stb_image_resize.h"
#include "stb/stb_image_write.h"
#include "CImageLockGuard.h"
#include "CImageJpg.h"

class CImageJpg; // Forward declaration

/**
 * @brief Class representing a BMP image (RGB / grayscale)
 *
 * Provides functionalities for managing and processing BMP images
 */
class CImage
{
    friend class CImageLockGuard;

  private:
    mutable SemaphoreHandle_t imageMutex;
    mutable TaskHandle_t lockingTask = NULL;
    mutable int lockCount = 0;

    bool lock() const;
    void unlock() const;

    std::string name;
    int width, height, channels;
    int imgDataSize;
    uint8_t *imgData;
    int allocatedSize;
    bool externalMemory;

    void freeImageData();
    bool getIsInbound(int x, int y);

  public:
    /**
     * @brief Constructor: Default
     */
    CImage();

    /**
     * @brief Constructor: Parametrized
     * @param objName Image name
     * @param width Image width
     * @param height Image height
     * @param channels Number of color channels
     * @param stbLibMemoryMod Use STB memory modifications
     * Note: This allocates one more byte to be able to reuse this memory by STB library using a custom memory allocation)
     * @param data Image data (Optional, default: nullptr)
     */
    CImage(std::string objName, int width, int height, int channels, bool stbLibMemoryMod = false, const uint8_t *data = nullptr);

    /**
     * @brief Constructor: Loads an image from a JPG file
     * @param objName Object name
     * @param filename File path
     * @param customStbLibMemAllocation Use custom STB memory allocation (Optional, default: false).
     * Note: Set this to 'true', STBIObjectPSRAM (psram.h) has be configured properly before calling this function
     * @param grayscale Load as grayscale (Optional, default: false)
     */
    explicit CImage(std::string objName, const std::string &filename, bool customStbLibMemAllocation = false, bool grayscale = false);

    /**
     * @brief Copy constructor
     * @param other Another CImage object to copy from
     */
    CImage(const CImage &other);

    /**
     * @brief Copy assignment operator
     * @param other Another CImage object to copy from
     * @return Reference to the current object
     */
    CImage &operator=(const CImage &other);

    /**
     * @brief Move constructor
     * @param other Another CImage object to move from
     */
    CImage(CImage &&other) noexcept;

    /**
     * @brief Move assignment operator
     * @param other Another CImage object to move from
     * @return Reference to the current object
     */
    CImage &operator=(CImage &&other) noexcept;

    /**
     * @brief Destructor
     */
    ~CImage();


    // Functions
    /**
     * @brief Loads a JPG image from a file
     * @param filename File path
     * @param overwriteSource Overwrite current image data memory (Optional, default: false)
     * @param grayscale Load as grayscale (Optional, default: false)
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t loadJpgFromFile(const std::string &filename, bool overwriteSource = false, bool grayscale = false);

    /**
     * @brief Loads a JPG image from memory
     * @param buffer Pointer to image data
     * @param size Buffer size
     * @param overwriteSource Overwrite current image data memory (Optional, default: false)
     * @param grayscale Load as grayscale (Optional, default: false)
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t loadJpgFromMemory(void *buffer, int size, bool overwriteSource = false, bool grayscale = false);

    /**
     * @brief Saves the image as a JPG file
     * @param filename File path
     * @param quality JPEG quality (0-100, optional, default: IMAGE_JPG_DEFAULT_QUALITY)
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t saveJpgToFile(const std::string &filename, const int quality = IMAGE_JPG_DEFAULT_QUALITY);

    /**
     * @brief Saves the image to a buffer in JPG format
     * @param jpgBuffer Pointer to buffer
     * @param size Buffer size
     * @param quality JPEG quality (0-100 | Optional, default: IMAGE_JPG_DEFAULT_QUALITY)
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t saveJpgToBuffer(uint8_t *jpgBuffer, const int size, const int quality = IMAGE_JPG_DEFAULT_QUALITY);

    /**
     * @brief Saves the image to a CImageJpg container
     * @param jpgContainer Pointer to container
     * @param quality JPEG quality (0-100 | Optional, default: IMAGE_JPG_DEFAULT_QUALITY)
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t saveJpgToContainer(CImageJpg *jpgContainer, const int quality = IMAGE_JPG_DEFAULT_QUALITY);

    /**
     * @brief Sends the image over HTTP as a JPEG
     * @param req HTTP request
     * @param quality JPEG quality (0-100 | Optional, default: IMAGE_JPG_DEFAULT_QUALITY)
     * @return ESP_OK on success, error code otherwise
     */
    esp_err_t sendJpgToHttp(httpd_req_t *req, const int quality = IMAGE_JPG_DEFAULT_QUALITY);


    // Getter + Setter
    /**
     * @brief Gets the name of the image
     * @return Image name
     */
    std::string getName() const { return name; };

    /**
     * @brief Sets the name of the image
     * @param objName New name of the image
     */
    void setName(std::string objName) { name = objName; };

    /**
     * @brief Checks if the image is valid
     * @return True if valid, false otherwise
     */
    bool isValid() const;

    /**
     * @brief Gets the width of the image (in pixels)
     * @return Image width
     */
    int getWidth() const { return width; };

    /**
     * @brief Gets the height of the image (in pixels)
     * @return Image height
     */
    int getHeight() const { return height; };

    /**
     * @brief Gets the channels of the image
     * @return Image channels (1: grayscale, 3: RGB image)
     */
    int getChannels() const { return channels; };

    /**
     * @brief Gets the image data size
     * @return Image data size in bytes
     */
    int getImgDataSize() const { return imgDataSize; };

    /**
     * @brief Gets the allocated size
     * @return Allocated size in bytes
     */
    int getAllocSize() const { return allocatedSize; };

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
     * @brief Gets the color value of a specific pixel channel
     *
     * Note: No thread-safety, caller has to ensure thread-safefy
     *
     * @param x X coordinate
     * @param y Y coordinate
     * @param ch Channel index
     * @return Pixel color value
     */
    uint8_t getPixelColor(int x, int y, int ch);

    /**
     * @brief Sets the color of a specific pixel
     *
     * Note: No thread-safety, caller has to ensure thread-safefy
     *
     * @param x X coordinate
     * @param y Y coordinate
     * @param r Red component
     * @param g Green component
     * @param b Blue component
     */
    void setPixelColor(int x, int y, uint8_t r, uint8_t g, uint8_t b);
};

#endif // CIMAGE_H
