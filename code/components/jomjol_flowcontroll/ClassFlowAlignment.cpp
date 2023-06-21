#include "ClassFlowAlignment.h"
#include "ClassFlowTakeImage.h"
#include "ClassFlow.h"
#include "MainFlowControl.h"

#include "CRotateImage.h"
#include "esp_log.h"

#include "nvs_flash.h"
#include "nvs.h"

#include "ClassLogFile.h"
#include "psram.h"
#include "../../include/defines.h"


static const char *TAG = "ALIGN";

// #define DEBUG_DETAIL_ON  


void ClassFlowAlignment::SetInitialParameter(void)
{
    PresetFlowStateHandler(true);
    initalrotate = 0.0;
    anz_ref = 0;
    AlignFAST_SADThreshold = 10;  // FAST ALIGN ALGO: SADNorm -> if smaller than threshold use same alignment values as last round
    initialmirror = false;
    use_antialiasing = false;
    initialflip = false;
    SaveAllFiles = false;
    ListFlowControll = NULL;
    AlignAndCutImage = NULL;
    ImageBasis = NULL;
    ImageTMP = NULL;
    AlgROI = (ImageData*)heap_caps_malloc(sizeof(ImageData), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    previousElement = NULL;
    disabled = false;
}


ClassFlowAlignment::ClassFlowAlignment(std::vector<ClassFlow*>* lfc)
{
    SetInitialParameter();
    ListFlowControll = lfc;

    for (int i = 0; i < ListFlowControll->size(); ++i)
    {
        if (((*ListFlowControll)[i])->name().compare("ClassFlowTakeImage") == 0)
            ImageBasis = ((ClassFlowTakeImage*) (*ListFlowControll)[i])->rawImage;
    }
}


bool ClassFlowAlignment::ReadParameter(FILE* pfile, string& aktparamgraph)
{
    std::vector<string> splitted;
    int search_x = 20;  // target_x +/- search_x
    int search_y = 20;  // target_y +/- search_y
    int alg_algo = 0;   // 0= DEFAULT; 1 =HIGHACCURACY; 2= FAST; 3= OFF //add disable aligment algo |01.2023

    aktparamgraph = trim(aktparamgraph);

    if (aktparamgraph.size() == 0)
        if (!this->GetNextParagraph(pfile, aktparamgraph))
            return false;

    if (aktparamgraph.compare("[Alignment]") != 0)       //Paragraph does not fit Alignment
        return false;

    while (this->getNextLine(pfile, &aktparamgraph) && !this->isNewParagraph(aktparamgraph))
    {
        splitted = ZerlegeZeile(aktparamgraph);
        
        if ((toUpper(splitted[0]) == "ALIGNMENTALGO") && (splitted.size() > 1))
        {
            if (toUpper(splitted[1]) == "HIGHACCURACY")
                alg_algo = 1;
            else if (toUpper(splitted[1]) == "FAST")
                alg_algo = 2;
            else if (toUpper(splitted[1]) == "ROTATION")
                alg_algo = 3;
            else if (toUpper(splitted[1]) == "OFF")
                alg_algo = 4;
            else
                alg_algo = 0;   // Default

            #ifdef DEBUG_DETAIL_ON
                std::string zw2 = "Alignment mode selected: " + std::to_string(alg_algo);
                LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, zw2);
            #endif
        }
        
        if ((toUpper(splitted[0]) == "SEARCHFIELDX") && (splitted.size() > 1))
        {
            search_x = std::stoi(splitted[1]);
        } 

        if ((toUpper(splitted[0]) == "SEARCHFIELDY") && (splitted.size() > 1))
        {
            search_y = std::stoi(splitted[1]);
        }

        if ((toUpper(splitted[0]) == "FLIPIMAGESIZE") && (splitted.size() > 1))
        {
            if (toUpper(splitted[1]) == "TRUE")
                initialflip = true;
            else
                initialflip = false;
        }

        if ((toUpper(splitted[0]) == "INITIALMIRROR") && (splitted.size() > 1))
        {
            if (toUpper(splitted[1]) == "TRUE")
                initialmirror = true;
            else
                initialmirror = false;
        }

        if (((toUpper(splitted[0]) == "INITALROTATE") || (toUpper(splitted[0]) == "INITIALROTATE")) && (splitted.size() > 1))
        {
            this->initalrotate = std::stof(splitted[1]);
        }
 
        if ((toUpper(splitted[0]) == "ANTIALIASING") && (splitted.size() > 1))
        {
            if (toUpper(splitted[1]) == "TRUE")
                use_antialiasing = true;
            else
                use_antialiasing = false;
        }

        if ((toUpper(splitted[0]) == "SAVEALLFILES") && (splitted.size() > 1))
        {
            if (toUpper(splitted[1]) == "TRUE")
                SaveAllFiles = true;
            else
                SaveAllFiles = false;
        }

        if ((splitted.size() == 3) && (anz_ref < 2))
        {
            int x=0,y=0,n=0;
            References[anz_ref].image_file = FormatFileName("/sdcard" + splitted[0]);
            stbi_info(References[anz_ref].image_file.c_str(), &x, &y, &n);

            References[anz_ref].refImage = new CImageBasis("refImage" + std::to_string(anz_ref));
            if (References[anz_ref].refImage) {
                if(!References[anz_ref].refImage->CreateEmptyImage(x, y, n, 1)) {
                    LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to create alignment marker image");
                    return false;
                }
            }
            STBIObjectPSRAM.name = "refImage" + std::to_string(anz_ref);
            STBIObjectPSRAM.usePreallocated = true;
            STBIObjectPSRAM.PreallocatedMemory = References[anz_ref].refImage->RGBImageGet();
            STBIObjectPSRAM.PreallocatedMemorySize = References[anz_ref].refImage->getMemsize();
            References[anz_ref].refImage->LoadFromFilePreallocated("refImage" + std::to_string(anz_ref), References[anz_ref].image_file.c_str());

            References[anz_ref].target_x = std::stoi(splitted[1]);
            References[anz_ref].target_y = std::stoi(splitted[2]);

            References[anz_ref].search_x = search_x;
            References[anz_ref].search_y = search_y;
            References[anz_ref].fastalg_SADThreshold = AlignFAST_SADThreshold;
            References[anz_ref].alignment_algo = alg_algo;
            anz_ref++;
        }
    }

    if (References[0].alignment_algo == 2) // Load references if "fast" algo is used
        LoadReferenceAlignmentValues();

    return true;
}


string ClassFlowAlignment::getHTMLSingleStep(string host)
{
    string result;

    result =          "<p>Rotated Image: </p> <p><img src=\"" + host + "/img_tmp/rot.jpg\"></p>\n";
    result = result + "<p>Found Alignment: </p> <p><img src=\"" + host + "/img_tmp/rot_roi.jpg\"></p>\n";
    result = result + "<p>Aligned Image: </p> <p><img src=\"" + host + "/img_tmp/alg.jpg\"></p>\n";
    return result;
}


bool ClassFlowAlignment::doFlow(string time) 
{
    PresetFlowStateHandler();
    if (AlgROI == NULL)  // AlgROI needs to be allocated before ImageTMP to avoid heap fragmentation
    {
        AlgROI = (ImageData*)heap_caps_realloc(AlgROI, sizeof(ImageData), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);     
        if (AlgROI == NULL) 
        {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to allocate AlgROI");
            LogFile.WriteHeapInfo("ClassFlowAlignment-doFlow");
        }
    }

    if (AlgROI)
    {
        ImageBasis->writeToMemoryAsJPG((ImageData*)AlgROI, 90);
    }

    if (ImageTMP == NULL) 
    {
        ImageTMP = new CImageBasis("ImageTMP", ImageBasis, 1);
        if (ImageTMP == NULL) 
        {
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to allocate ImageTMP");
            LogFile.WriteHeapInfo("ClassFlowAlignment-doFlow");
            return false;
        }
    }

    delete AlignAndCutImage;
    AlignAndCutImage = new CAlignAndCutImage("AlignAndCutImage", ImageBasis, ImageTMP);
    if (AlignAndCutImage == NULL) 
    {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Failed to allocate AlignAndCutImage");
        LogFile.WriteHeapInfo("ClassFlowAlignment-doFlow");
        return false;
    }

    CRotateImage rt("rawImageRT", AlignAndCutImage, ImageTMP, initialflip);
    if (initialflip)
    {
        int _zw = ImageBasis->height;
        ImageBasis->height = ImageBasis->width;
        ImageBasis->width = _zw;

        _zw = ImageTMP->width;
        ImageTMP->width = ImageTMP->height;
        ImageTMP->height = _zw;
    }

    if (initialmirror)
    {
        ESP_LOGD(TAG, "do mirror");
        rt.Mirror();
        
        if (SaveAllFiles)
            AlignAndCutImage->SaveToFile(FormatFileName("/sdcard/img_tmp/mirror.jpg"));
    }
 
    if ((initalrotate != 0) || initialflip)
    {
        if (References[0].alignment_algo == 4)  // alignment off: no initial rotation and no additional alignment algo
            initalrotate = 0;
        
        if (use_antialiasing)
            rt.RotateAntiAliasing(initalrotate);
        else
            rt.Rotate(initalrotate);
        
        if (SaveAllFiles)
            AlignAndCutImage->SaveToFile(FormatFileName("/sdcard/img_tmp/rot.jpg"));
    }

    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Initial rotation: " + std::to_string(initalrotate));

    if(References[0].alignment_algo <= 2) { // Only if any additional alignment algo is used: "default", "highaccuracy" or "fast"
        int AlignRetval = AlignAndCutImage->Align(&References[0], &References[1]);

        if (AlignRetval >= 0) {
            SaveReferenceAlignmentValues();
        }
        else if (AlignRetval == -1) {   // alignment failed         
            LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "Misalignment too large. Check initial rotation, alignment marker or "
                                                    "increase alignment expert parameter: Search Field X, Y");
            FlowStateHandlerSetError(-1);  
        }
    }

    if (AlgROI) {
        if(References[0].alignment_algo <= 2) { // Only if any additional alignment algo is used: "default", "highaccuracy" or "fast"
            DrawRef(ImageTMP);
        }
        flowctrl.DigitalDrawROI(ImageTMP);
        flowctrl.AnalogDrawROI(ImageTMP);
        ImageTMP->writeToMemoryAsJPG((ImageData*)AlgROI, 90);
    }
    
    if (SaveAllFiles)
    {
        AlignAndCutImage->SaveToFile(FormatFileName("/sdcard/img_tmp/alg.jpg"));
        ImageTMP->SaveToFile(FormatFileName("/sdcard/img_tmp/alg_roi.jpg"));
    }

    // must be deleted to have memory space for loading tflite
    delete ImageTMP;
    ImageTMP = NULL;

    if (!getFlowState()->isSuccessful)
        return false;

    return true;
}


bool ClassFlowAlignment::SaveReferenceAlignmentValues()
{ 
    esp_err_t err = ESP_OK;
    
    nvs_handle_t align_nvshandle;
    err = nvs_open("align", NVS_READWRITE, &align_nvshandle);
    if (err != ESP_OK) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "SaveReferenceAlignmentValues: No valid NVS handle - error code : " + std::to_string(err));
        return false;
    }

    err = nvs_set_i32(align_nvshandle, "Ref0fastalg_x", References[0].fastalg_x);
    if (err != ESP_OK) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "SaveReferenceAlignmentValues: Ref0fastalg_x - error code: " + std::to_string(err));
        return false;
    }
    err = nvs_set_i32(align_nvshandle, "Ref0fastalg_y", References[0].fastalg_y);
    if (err != ESP_OK) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "SaveReferenceAlignmentValues: Ref0fastalg_y - error code: " + std::to_string(err));
        return false;
    }

    err = nvs_set_i32(align_nvshandle, "Ref1fastalg_x", References[1].fastalg_x);
    if (err != ESP_OK) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "SaveReferenceAlignmentValues: Ref1fastalg_x - error code: " + std::to_string(err));
        return false;
    }
    err = nvs_set_i32(align_nvshandle, "Ref1fastalg_y", References[1].fastalg_y);
    if (err != ESP_OK) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "SaveReferenceAlignmentValues: Ref1fastalg_y - error code: " + std::to_string(err));
        return false;
    }

    err = nvs_commit(align_nvshandle);
    nvs_close(align_nvshandle);

    if (err != ESP_OK) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "SaveReferenceAlignmentValues: nvs_commit - error code: " + std::to_string(err));
        return false;
    }

    return true;
}


bool ClassFlowAlignment::LoadReferenceAlignmentValues(void)
{
    esp_err_t err = ESP_OK;

    nvs_handle_t align_nvshandle;
    err = nvs_open("align", NVS_READONLY, &align_nvshandle);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND ) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "LoadReferenceAlignmentValues: No valid NVS handle - error code : " + std::to_string(err));
        return false;
    }

    err = nvs_get_i32(align_nvshandle, "Ref0fastalg_x", (int32_t*)&References[0].fastalg_x);
    if (err != ESP_OK) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "LoadReferenceAlignmentValues: Ref0fastalg_x - error code: " + std::to_string(err));
        return false;
    }
    err = nvs_get_i32(align_nvshandle, "Ref0fastalg_y", (int32_t*)&References[0].fastalg_y);
    if (err != ESP_OK) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "LoadReferenceAlignmentValues: Ref0fastalg_y - error code: " + std::to_string(err));
        return false;
    }

    err = nvs_get_i32(align_nvshandle, "Ref1fastalg_x", (int32_t*)&References[1].fastalg_x);
    if (err != ESP_OK) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "LoadReferenceAlignmentValues: Ref1fastalg_x - error code: " + std::to_string(err));
        return false;
    }
    err = nvs_get_i32(align_nvshandle, "Ref1fastalg_y", (int32_t*)&References[1].fastalg_y);
        if (err != ESP_OK) {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "LoadReferenceAlignmentValues: Ref1fastalg_y - error code: " + std::to_string(err));
        return false;
    }
    
    nvs_close(align_nvshandle);
    
    return true;
}


void ClassFlowAlignment::DrawRef(CImageBasis *_zw)
{
    if (_zw->ImageOkay()) 
    {
        _zw->drawRect(References[0].target_x, References[0].target_y, References[0].width, References[0].height, 255, 0, 0, 2);
        _zw->drawRect(References[1].target_x, References[1].target_y, References[1].width, References[1].height, 255, 0, 0, 2);
    }
}


ClassFlowAlignment::~ClassFlowAlignment()
{
    free_psram_heap("AlgROI", AlgROI);
    delete References[0].refImage;
    delete References[1].refImage;  
    delete ImageTMP;
    delete AlignAndCutImage;
}
