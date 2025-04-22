#include "unity.h"
#include "CImage.h"
#include "esp_log.h"


// Test constructor: Default Constructor
void test_CImage_default_constructor(void)
{
    CImage *testImage = new CImage();

    TEST_ASSERT_NOT_NULL(testImage);

    // Check default values
    TEST_ASSERT_EQUAL_STRING("default", testImage->getName().c_str());
    TEST_ASSERT_EQUAL(0, testImage->getWidth());
    TEST_ASSERT_EQUAL(0, testImage->getHeight());
    TEST_ASSERT_EQUAL(0, testImage->getChannels());
    TEST_ASSERT_EQUAL(0, testImage->getImgDataSize());
    TEST_ASSERT_NULL(testImage->getImgData());

    delete testImage;
}


// Test constructor: Parameterized Constructor
void test_CImage_parameterized_constructor(void)
{
    std::string name = "Test Image";
    int width = 100;
    int height = 200;
    int channels = 3;
    bool stbLibMemoryMod = true;
    uint8_t *data = nullptr;

    CImage img(name, width, height, channels, stbLibMemoryMod, data);

    TEST_ASSERT_EQUAL_STRING(name.c_str(), img.getName().c_str());
    TEST_ASSERT_EQUAL(width, img.getWidth());
    TEST_ASSERT_EQUAL(height, img.getHeight());
    TEST_ASSERT_EQUAL(channels, img.getChannels());
    TEST_ASSERT_NOT_NULL(img.getImgData());
}


// Test loadJpgFromFile function: Success case
void test_CImage_loadJpgFromFile_success(void)
{
    CImage *testImage = new CImage();
    const std::string filename = "/sdcard/config/reference.jpg";
    esp_err_t result = testImage->loadJpgFromFile(filename, false, false);

    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_NOT_EQUAL(nullptr, testImage->getImgData());

    delete testImage;
}

// Test loadJpgFromFile function: Failure case (file not found or invalid)
void test_CImage_loadJpgFromFile_failure(void)
{
    CImage *testImage = new CImage();
    const std::string filename = "/sdcard/config/not_available.jpg";
    esp_err_t result = testImage->loadJpgFromFile(filename, false, false);

    TEST_ASSERT_EQUAL(ESP_FAIL, result);
    TEST_ASSERT_EQUAL(nullptr, testImage->getImgData());

    delete testImage;
}

// Test saveJpgToFile function: Success case
void test_CImage_saveJpgToFile_success(void)
{
    CImage *testImage = new CImage();
    testImage->loadJpgFromFile("/sdcard/config/reference.jpg", false, false);

    const std::string filename = "/sdcard/img_tmp/unity_CImage_saveJpgToFile.jpg";
    const int quality = 90;
    esp_err_t result = testImage->saveJpgToFile(filename, quality);

    TEST_ASSERT_EQUAL(ESP_OK, result);

    delete testImage;
}


// Test saveJpgToFile function: Failure case (no image data)
void test_CImage_saveJpgToFile_failure_no_data(void)
{
    CImage *testImage = new CImage();
    const std::string filename = "/sdcard/img_tmp/unity_CImage_saveJpgToFile.jpg";
    const int quality = 90;
    esp_err_t result = testImage->saveJpgToFile(filename, quality);

    TEST_ASSERT_EQUAL(ESP_FAIL, result);

    delete testImage;
}


// Test copy constructor
void test_CImage_copy_constructor(void)
{
    CImage *testImage = new CImage("test-copy", 100, 100, 1);
    CImage copiedImage = *testImage;

    std::string newName = testImage->getName() + "-copy";

    TEST_ASSERT_EQUAL_STRING(newName.c_str(), copiedImage.getName().c_str());
    TEST_ASSERT_EQUAL(testImage->getWidth(), copiedImage.getWidth());
    TEST_ASSERT_EQUAL(testImage->getHeight(), copiedImage.getHeight());
    TEST_ASSERT_EQUAL(testImage->getChannels(), copiedImage.getChannels());
    TEST_ASSERT_EQUAL(testImage->getImgDataSize(), copiedImage.getImgDataSize());
    TEST_ASSERT_NOT_NULL(copiedImage.getImgData());

    delete testImage;
}


// Test copy assignment operator
void test_CImage_copy_assignment_operator(void)
{
    CImage *testImage = new CImage("test-copy-assign", 101, 101, 1);
    CImage anotherImage;
    anotherImage = *testImage;

    std::string newName = testImage->getName() + "-copy-assign";

    TEST_ASSERT_EQUAL_STRING(newName.c_str(), anotherImage.getName().c_str());
    TEST_ASSERT_EQUAL(testImage->getWidth(), anotherImage.getWidth());
    TEST_ASSERT_EQUAL(testImage->getHeight(), anotherImage.getHeight());
    TEST_ASSERT_EQUAL(testImage->getChannels(), anotherImage.getChannels());
    TEST_ASSERT_EQUAL(testImage->getImgDataSize(), anotherImage.getImgDataSize());
    TEST_ASSERT_NOT_NULL(anotherImage.getImgData());

    delete testImage;
}


// Test move constructor
void test_CImage_move_constructor(void)
{
    CImage *testImage = new CImage("test-move", 99, 99, 1);
    CImage movedImage = std::move(*testImage);

    TEST_ASSERT_EQUAL_STRING("test-move", movedImage.getName().c_str());
    TEST_ASSERT_EQUAL(99, movedImage.getWidth());
    TEST_ASSERT_EQUAL(99, movedImage.getHeight());
    TEST_ASSERT_EQUAL(1, movedImage.getChannels());
    TEST_ASSERT_EQUAL(99 * 99 * 1, movedImage.getImgDataSize());
    TEST_ASSERT_NOT_NULL(movedImage.getImgData());

    delete testImage;
}


// Test move assignment operator
void test_CImage_move_assignment_operator(void)
{
    CImage *testImage = new CImage("test-move-assign", 101, 101, 1);
    CImage anotherImage;
    anotherImage = std::move(*testImage);

    TEST_ASSERT_EQUAL_STRING("test-move-assign", anotherImage.getName().c_str());
    TEST_ASSERT_EQUAL(101, anotherImage.getWidth());
    TEST_ASSERT_EQUAL(101, anotherImage.getHeight());
    TEST_ASSERT_EQUAL(1, anotherImage.getChannels());
    TEST_ASSERT_EQUAL(101 * 101 * 1, anotherImage.getImgDataSize());
    TEST_ASSERT_NOT_NULL(anotherImage.getImgData());

    delete testImage;
}


/**
 * @brief test CImage handling
 */
void test_CImageHandling()
{
    test_CImage_default_constructor();
    test_CImage_parameterized_constructor();
    test_CImage_loadJpgFromFile_success();
    test_CImage_loadJpgFromFile_failure();
    test_CImage_saveJpgToFile_success();
    test_CImage_saveJpgToFile_failure_no_data();
    test_CImage_copy_constructor();
    test_CImage_copy_assignment_operator();
    test_CImage_move_constructor();
    test_CImage_move_assignment_operator();
}
