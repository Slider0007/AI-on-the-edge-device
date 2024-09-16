#ifndef CLASSFLOWDEFINETYPES_H
#define CLASSFLOWDEFINETYPES_H

#include <vector>

#include "cfgDataStruct.h"
#include "CImageBasis.h"


enum CNNType {
    CNNTYPE_NONE,
    CNNTYPE_AUTODETECT,
    CNNTYPE_ANALOG_CONT,
    CNNTYPE_ANALOG_CLASS100,
    CNNTYPE_DIGIT_CLASS11,
    CNNTYPE_DIGIT_DOUBLE_HYBRID10,
    CNNTYPE_DIGIT_CLASS100
 };


struct AlignmentMarker { //@TODO. Rename members
    CImageBasis *refImage = NULL;
    std::string image_file;
    std::string error_details = "";
    int alignment_algo = ALIGNALGO_DEFAULT; // 0 = "Default" (nur R-Kanal), 1 = "HighAccuracy" (RGB-Kanal), 2 = "Fast" (1.x RGB, dann isSimilar), 3= "only initial rotation", 4 = "off"
    int target_x = 0;
    int target_y = 0;
    int width = 0;
    int height = 0;
    int found_x = 0;
    int found_y = 0;
    int search_x = 0;
    int search_y = 0;
    int fastalg_x = 0;
    int fastalg_y = 0;
    int fastalg_SADThreshold = 0;
};


struct RoiData { // ROI
    const struct RoiElement *param = NULL;
    CImageBasis *imageRoi = NULL;
    CImageBasis *imageRoiResized = NULL;
    bool isRejected = false; // Only used for dig-cont models
    int CNNResult = -10;    // Normalized to 0-99 (exception for class11: 0-10: 0-9+NaN), default: negative number equal to "-1.0"
    std::string sCNNResult = "-1"; // Result clamped and converted to string for visualization purpose
};


struct SequenceData {
    int sequenceId;                     // Sequence ID
    std::string sequenceName;           // Sequence Name

    std::vector<RoiData *> digitRoi;    // Digit ROIs
    std::vector<RoiData *> analogRoi;   // Analog ROIs

    bool isFallbackValueValid = false;  // Fallback value is valid in terms of not being outdated
    bool isActualValueANumber = false;  // Actual value is valid number (further processing possible)
    bool isActualValueConfirmed = false;// Actual value is without any deviation (fully processed by post-processing without deviation)
    int correctedDecimalShift = 0;      // Decimal shift parameter adapted by actual configuration
    int decimalPlaceCount = 0;          // No of decimal places

    time_t timeProcessed;               // Time of actual source image was taken (== actual result time)
    time_t timeFallbackValue;           // Time of FallbackValue in seconds
    double ratePerMin = 0.0;            // Rate per minute (e.g. m3/min)
    double ratePerInterval = 0.0;       // Rate per interval, value delta between actual and fallback value,
                                        // e.g. dV/dT (actual value-fallbackvalue)/(actual processing timestamp-fallbackvalue processing timestamp)
    double actualValue = 0.0;           // Actual result value
    double fallbackValue = 0.0;         // Fallback value, equal the last successful processed value (legacy name: prevalue)

    std::string sTimeProcessed;         // Processed timestamp -> Time of last processing
    std::string sTimeFallbackValue;     // FallbackValue timestamp -> Time of last valid value
    std::string sRatePerMin = "0.0";    // Rate per minute, based on time between last valid and actual reading
    std::string sRatePerInterval = "0.0"; // Rate per interval, value delta between actual value and last valid value (fallback value)
    std::string sRawValue = "";         // Raw value (possibly incl. N & leading 0)
    std::string sActualValue = "";      // Value of actual valid reading, incl. post-processing corrections
    std::string sFallbackValue = "";    // Fallback value, equal to last valid reading (legacy name: prevalue)
    std::string sValueStatus = "";      // Value status

    const struct PostProcessingPerSequence *paramPostProc = NULL;
    const struct InfluxDBPerSequence *paramInfluxDBv1 = NULL;
    const struct InfluxDBPerSequence *paramInfluxDBv2 = NULL;
};


struct HTMLInfo //@TODO still needed?
{
	float val;
	CImageBasis *image = NULL;
	CImageBasis *image_org = NULL;
	std::string filename;
	std::string filename_org;
	std::string name;
	int position;
};


#endif

