#ifndef CIMAGELOCKGUARD_H
#define CIMAGELOCKGUARD_H

#include "CImage.h"
#include "CImageJpg.h"

class CImage;    // Forward declaration
class CImageJpg; // Forward declaration

/**
 * @brief RAII-style lock guard for image objects
 *
 * Ensures that an image remains locked while the guard is in scope
 */
class CImageLockGuard
{
  private:
    void *imgPtr;
    bool isJpg;
    bool locked;

  public:
    /**
     * @brief Constructs a lock guard for a CImage object
     * @param image Reference to the image to be locked
     */
    explicit CImageLockGuard(const CImage &image);

    /**
     * @brief Constructs a lock guard for a CImageJpg object
     * @param image Reference to the image to be locked
     */
    explicit CImageLockGuard(const CImageJpg &image);

    /**
     * @brief Destructor unlocks the image if locked
     */
    ~CImageLockGuard();

    /**
     * @brief Checks if the image is currently locked
     * @return True if locked, false otherwise
     */
    bool isLocked() const;
};

#endif // CIMAGELOCKGUARD_H