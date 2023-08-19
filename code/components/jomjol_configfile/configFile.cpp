#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Helper.h"
#include "configFile.h"
#include <esp_log.h>

#include "../../include/defines.h"

static const char *TAG = "CONFIG";

ConfigFile::ConfigFile(std::string filePath)
{
    std::string config = FormatFileName(filePath);
    pFile = fopen(config.c_str(), "r");

	/* Related to article: https://blog.drorgluska.com/2022/06/esp32-sd-card-optimization.html */
    // Set buffer to SD card allocation size of 512 byte (newlib default: 128 byte) -> reduce system read/write calls
    setvbuf(pFile, NULL, _IOFBF, 512);
}

ConfigFile::~ConfigFile()
{
    fclose(pFile);
}

bool ConfigFile::isNewParagraph(std::string input)
{
	if ((input[0] == '[') || ((input[0] == ';') && (input[1] == '[')))
	{
		return true;
	}
	return false;
}

bool ConfigFile::GetNextParagraph(std::string& aktparamgraph, bool &disabled, bool &eof)
{
	while (getNextLine(&aktparamgraph, disabled, eof) && !isNewParagraph(aktparamgraph));

	if (isNewParagraph(aktparamgraph))
		return true;
	return false;
}

bool ConfigFile::getNextLine(std::string *rt, bool &disabled, bool &eof)
{
    eof = false;
	char zw[256] = "";
	if (pFile == NULL)
	{
		*rt = "";
		return false;
	}

	if (fgets(zw, sizeof(zw), pFile))
	{
		ESP_LOGD(TAG, "%s", zw);
		if ((strlen(zw) == 0) && feof(pFile))
		{
			*rt = "";
			eof = true;
			return false;
		}
	}
	else
	{
		*rt = "";
		eof = true;
		return false;
	}
	*rt = zw;
	*rt = trim(*rt);
	while ((zw[0] == ';' || zw[0] == '#' || (rt->size() == 0)) && !(zw[1] == '['))
	{
		fgets(zw, sizeof(zw), pFile);
		ESP_LOGD(TAG, "%s", zw);
		if (feof(pFile))
		{
			*rt = "";
            eof = true;
			return false;
		}
		*rt = zw;
		*rt = trim(*rt);
	}

    disabled = ((*rt)[0] == ';');
	return true;
}
