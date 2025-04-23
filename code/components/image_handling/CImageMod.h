#ifndef CIMAGEMOD_H
#define CIMAGEMOD_H

#include "CImage.h"

/**
 * @brief Image modification (Helper class)
 *
 * Provides image modification functions such as rotation, translation, and filtering
 */
class CImageMod
{
  public:
    /**
     * @brief Rotates an image by a given angle
     * @param img Source image
     * @param angle Rotation angle in degrees
     * @param imgHelper Temporary helper image buffer
     * @param overwriteSource Overwrite the source image (default: false)
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t rotate(CImage &img, float angle, CImage &imgHelper, bool overwriteSource = false);

    /**
     * @brief Rotates an image around a specified center point
     * @param img Source image
     * @param angle Rotation angle in degrees
     * @param centerX X-coordinate of rotation center
     * @param centerY Y-coordinate of rotation center
     * @param imgHelper Temporary helper image buffer
     * @param overwriteSource Overwrite the source image (default: false)
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t rotate(CImage &img, float angle, int centerX, int centerY, CImage &imgHelper, bool overwriteSource = false);

    /**
     * @brief Translates an image by a given x, y offset
     * @param img Source image
     * @param dx Horizontal shift in x
     * @param dy Vertical shift in y
     * @param imgHelper Temporary helper image buffer
     * @param overwriteSource Overwrite the source image (default: false)
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t translate(CImage &img, int dx, int dy, CImage &imgHelper, bool overwriteSource = false);

    /**
     * @brief Crops an image to a specified region
     * @param img Source image
     * @param x X-coordinate of top-left corner
     * @param y Y-coordinate of top-left corner
     * @param newWidth Width of cropped area
     * @param newHeight Height of cropped area
     * @param imgTarget Target image to store cropped result
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t crop(CImage &img, int x, int y, int newWidth, int newHeight, CImage &imgTarget);

    /**
     * @brief Resizes an image to new dimensions
     * @param img Source image
     * @param newWidth Target width
     * @param newHeight Target height
     * @param imgTarget Target image to store resized result
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t resize(CImage &img, int newWidth, int newHeight, CImage &imgTarget);

    /**
     * @brief Converts an image to grayscale
     * @param img Source image
     * @param overwriteSource Overwrite the source image (default: true)
     * @param imgTarget Target image to store grayscale result (optional, default: nullptr)
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t grayscale(CImage &img, bool overwriteSource = true, CImage *imgTarget = nullptr);

    /**
     * @brief Normalizes an image
     * @param img Source image
     * @param overwriteSource Overwrite the source image (default: true)
     * @param imgTarget Target image to store normalized result (optional, default: nullptr)
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t normalize(CImage &img, bool overwriteSource = true, CImage *imgTarget = nullptr);

    /**
     * @brief Applies a negative effect to an image
     * @param img Source image
     * @param overwriteSource Overwrite the source image (default: true)
     * @param imgTarget Target image to store negative result (optional, default: nullptr)
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t negative(CImage &img, bool overwriteSource = true, CImage *imgTarget = nullptr);

    /**
     * @brief Draws a rectangle on an image
     * @param img Image
     * @param x X-coordinate of top-left corner
     * @param y Y-coordinate of top-left corner
     * @param dx Width of the rectangle
     * @param dy Height of the rectangle
     * @param r Red color component
     * @param g Green color component
     * @param b Blue color component
     * @param thickness Thickness of the line (default: 1 pixel)
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t drawRect(CImage &img, int x, int y, int dx, int dy, uint8_t r, uint8_t g, uint8_t b, uint8_t thickness = 1);

    /**
     * @brief Draws a line on an image
     * @param img Image
     * @param x1 X-coordinate of the start point
     * @param y1 Y-coordinate of the start point
     * @param x2 X-coordinate of the end point
     * @param y2 Y-coordinate of the end point
     * @param r Red color component
     * @param g Green color component
     * @param b Blue color component
     * @param thickness Thickness of the line (default: 1 pixel)
     * @return ESP_OK on success, error code otherwise
     * @see [Bresenham Algorithm (without floating points)](https://de.wikipedia.org/wiki/Bresenham-Algorithmus)
     */
    static esp_err_t drawLine(CImage &img, int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t thickness = 1);

    /**
     * @brief Draws a circle on an image
     * @param img Image
     * @param x X-coordinate of the circle center
     * @param y Y-coordinate of the circle center
     * @param rad Radius of the circle
     * @param r Red color component
     * @param g Green color component
     * @param b Blue color component
     * @param thickness Thickness of the line (default: 1 pixel)
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t drawCircle(CImage &img, int x, int y, int rad, uint8_t r, uint8_t g, uint8_t b, uint8_t thickness = 1);

    /**
     * @brief Draws an ellipse on an image
     * @param img Image
     * @param x X-coordinate of the ellipse center
     * @param y Y-coordinate of the ellipse center
     * @param radX Radius along the X-axis
     * @param radY Radius along the Y-axis
     * @param r Red color component
     * @param g Green color component
     * @param b Blue color component
     * @param thickness Thickness of the line (default: 1 pixel)
     * @return ESP_OK on success, error code otherwise
     */
    static esp_err_t drawEllipse(CImage &img, int x, int y, int radX, int radY, uint8_t r, uint8_t g, uint8_t b, uint8_t thickness = 1);
};

#endif // CIMAGEMOD_H
