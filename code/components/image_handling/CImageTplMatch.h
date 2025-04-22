#ifndef CIMAGETPLMATCH_H
#define CIMAGETPLMATCH_H

#include "ClassFlowDefineTypes.h"
#include "CImage.h"


/**
 * @brief Supported matching algorithms
 */
enum class TplMatchAlgorithm {
    SAD = 0 // Sum of Absolute Difference
};


/**
 * @brief Status codes
 */
enum TplMatchStatus {
    TPL_MATCH_OK_SIMILAR = 1,                 // Match successful with similarity check
    TPL_MATCH_OK = 0,                         // Match successful with algorithm
    TPL_MATCH_FAILED = -1,                    // Match failed
    TPL_MATCH_UNKNOWN_AGLORITHM = -2,         // Error unkown algorithm
    TPL_MATCH_ERROR_IMAGE = -3,               // Error with image
    TPL_MATCH_ERROR_TIMEOUT = ESP_ERR_TIMEOUT // Error with image lock
};


/**
 * @brief Image template matching (Helper class)
 *
 * Provides image template matching functionality
 */
class CImageTplMatch
{
  private:
    /**
     * @brief Performs template matching using the Sum of Absolute Differences (SAD) algorithm
     * @param img Source image
     * @param marker Alignment marker to match
     * @param noSimilarityCheck Disable similarity check (default: false)
     * @return Matching status
     */
    static TplMatchStatus tplMatchBySad(CImage &img, AlignmentMarker &marker, bool noSimilarityCheck = false);

    /**
     * @brief Calculates similarity scores of a given template image
     * @param img Source image
     * @param marker Alignment marker reference
     * @return true if similarity check is succeeded, false otherwise
     */
    static bool calcSimilarities(CImage &img, AlignmentMarker &marker);

  public:
    /**
     * @brief Invokes template matching using a specified matching algorithm
     * @param img Source image
     * @param imgTarget Target image (template)
     * @param marker1 First alignment marker
     * @param marker2 Second alignment marker
     * @param tplMatchAlgorithm Algorithm to use for matching (default: SAD)
     * @return Status
     */
    static TplMatchStatus invokeTplMatch(CImage &img, CImage &imgTarget, AlignmentMarker &marker1, AlignmentMarker &marker2,
                                         TplMatchAlgorithm tplMatchAlgorithm = TplMatchAlgorithm::SAD);
};

#endif // CIMAGETPLMATCH_H
