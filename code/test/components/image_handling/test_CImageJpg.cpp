#include "unity.h"
#include "CImageJpg.h"
#include "esp_log.h"


// Test constructor: Default Constructor
void test_CImageJpg_default_constructor(void)
{
    CImageJpg *testImage = new CImageJpg();

    TEST_ASSERT_NOT_NULL(testImage);

    // Check default values
    TEST_ASSERT_EQUAL_STRING("default", testImage->getName().c_str());
    TEST_ASSERT_EQUAL(0, testImage->getImgDataSize());
    TEST_ASSERT_NULL(testImage->getImgData());

    delete testImage;
}


// Test constructor: Parameterized Constructor
void test_CImageJpg_parameterized_constructor(void)
{
    const std::string name = "Test";
    CImageJpg img(name, 10);

    TEST_ASSERT_EQUAL_STRING(name.c_str(), img.getName().c_str());
    TEST_ASSERT_NOT_NULL(img.getImgData());
}


// Test constructor: Parameterized Constructor
void test_CImageJpg_parameterized_constructor_file(void)
{
    const std::string filename = "/sdcard/config/reference.jpg";
    CImageJpg *testImage = new CImageJpg("Test Image", filename);

    TEST_ASSERT_NOT_EQUAL(nullptr, testImage->getImgData());

    delete testImage;
}


// Test updateImageDataFromJpgBuffer function
void test_CImageJpg_updateImageDataFromJpgBuffer(void)
{
    const uint8_t data[10] = {0};
    CImageJpg *testImage = new CImageJpg("test", sizeof(data));
    esp_err_t result = testImage->updateImageDataFromJpgBuffer(data, sizeof(data));

    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(testImage->getImgDataSize(), sizeof(data));
    TEST_ASSERT_NOT_NULL(testImage->getImgData());

    delete testImage;
}


// Test updateImageDataFromJpgFile function
void test_CImageJpg_updateImageDataFromJpgFile(void)
{
    CImageJpg *testImage = new CImageJpg("test", IMAGE_JPG_MAX_SIZE);
    const std::string filename = "/sdcard/config/reference.jpg";
    esp_err_t result = testImage->updateImageDataFromJpgFile(filename);

    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_LESS_OR_EQUAL_INT32(IMAGE_JPG_MAX_SIZE, testImage->getImgDataSize());
    TEST_ASSERT_NOT_EQUAL(nullptr, testImage->getImgData());

    delete testImage;
}


// Test updateImageDataFromJpgFile function: Resize container
void test_CImageJpg_updateImageDataFromJpgFile_resize(void)
{
    CImageJpg *testImage = new CImageJpg("test", 10);
    const std::string filename = "/sdcard/config/reference.jpg";
    esp_err_t result = testImage->updateImageDataFromJpgFile(filename, true);

    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_NOT_EQUAL(testImage->getImgDataSize(), 10);
    TEST_ASSERT_NOT_EQUAL(nullptr, testImage->getImgData());

    delete testImage;
}


// Test loadJpgFromMemory function
void test_CImageJpg_loadJpgFromMemory(void)
{
    CImageJpg *testImage = new CImageJpg();
    const uint8_t data[10] = {0};
    esp_err_t result = testImage->loadJpgFromMemory((const void *)data, 10);

    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_NOT_EQUAL(nullptr, testImage->getImgData());

    delete testImage;
}


// Test saveJpgToFile function: Success case
void test_CImageJpg_saveJpgToFile_success(void)
{
    CImageJpg *testImage = new CImageJpg("Test", "/sdcard/config/reference.jpg");

    const std::string filename = "/sdcard/img_tmp/unity_CImageJpg_saveJpgToFile.jpg";
    esp_err_t result = testImage->saveJpgToFile(filename);

    TEST_ASSERT_EQUAL(ESP_OK, result);

    delete testImage;
}


// Test saveJpgToFile function: Failure case (no image data)
void test_CImageJpg_saveJpgToFile_failure_no_data(void)
{
    CImageJpg *testImage = new CImageJpg();
    const std::string filename = "/sdcard/img_tmp/unity_saveJpgToFile.jpg";
    esp_err_t result = testImage->saveJpgToFile(filename);

    TEST_ASSERT_EQUAL(ESP_FAIL, result);

    delete testImage;
}


// Test copy constructor
void test_CImageJpg_copy_constructor(void)
{
    CImageJpg *testImage = new CImageJpg("test-copy", 10);
    CImageJpg copiedImage = *testImage;

    std::string newName = testImage->getName() + "-copy";

    TEST_ASSERT_EQUAL_STRING(newName.c_str(), copiedImage.getName().c_str());
    TEST_ASSERT_EQUAL(testImage->getImgDataSize(), copiedImage.getImgDataSize());
    TEST_ASSERT_NOT_NULL(copiedImage.getImgData());

    delete testImage;
}


// Test copy assignment operator
void test_CImageJpg_copy_assignment_operator(void)
{
    CImageJpg *testImage = new CImageJpg("test-copy-assign", 11);
    CImageJpg anotherImage;
    anotherImage = *testImage;

    std::string newName = testImage->getName();

    TEST_ASSERT_EQUAL_STRING(newName.c_str(), anotherImage.getName().c_str());
    TEST_ASSERT_EQUAL(testImage->getImgDataSize(), anotherImage.getImgDataSize());
    TEST_ASSERT_NOT_NULL(anotherImage.getImgData());

    delete testImage;
}


// Test move constructor
void test_CImageJpg_move_constructor(void)
{
    CImageJpg *testImage = new CImageJpg("test-move", 10);
    CImageJpg movedImage = std::move(*testImage);

    TEST_ASSERT_EQUAL_STRING("test-move", movedImage.getName().c_str());
    TEST_ASSERT_EQUAL(10, movedImage.getImgDataSize());
    TEST_ASSERT_NOT_NULL(movedImage.getImgData());

    delete testImage;
}


// Test move assignment operator
void test_CImageJpg_move_assignment_operator(void)
{
    CImageJpg *testImage = new CImageJpg("test-move-assign", 11);
    CImageJpg anotherImage;
    anotherImage = std::move(*testImage);

    TEST_ASSERT_EQUAL_STRING("test-move-assign", anotherImage.getName().c_str());
    TEST_ASSERT_EQUAL(11, anotherImage.getImgDataSize());
    TEST_ASSERT_NOT_NULL(anotherImage.getImgData());

    delete testImage;
}


/**
 * @brief test CImageJpg handling
 */
void test_CImageJpgHandling()
{
    test_CImageJpg_default_constructor();
    test_CImageJpg_parameterized_constructor();
    test_CImageJpg_parameterized_constructor_file();
    test_CImageJpg_updateImageDataFromJpgBuffer();
    test_CImageJpg_updateImageDataFromJpgFile();
    test_CImageJpg_updateImageDataFromJpgFile_resize();
    test_CImageJpg_loadJpgFromMemory();
    test_CImageJpg_saveJpgToFile_success();
    test_CImageJpg_saveJpgToFile_failure_no_data();
    test_CImageJpg_copy_constructor();
    test_CImageJpg_copy_assignment_operator();
    test_CImageJpg_move_constructor();
    test_CImageJpg_move_assignment_operator();
}
