
#include "../../include/defines.h"
#ifdef DEBUG_HIMEM_MEMORY_CHECK

#ifndef HIMEM_MEMORY_CHECK_H
#define HIMEM_MEMORY_CHECK_H

#include <string>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp32/himem.h"


std::string himem_memory_check();

#endif //HIMEM_MEMORY_CHECK_H
#endif // DEBUG_HIMEM_MEMORY_CHECK
