#pragma once

#include <memory>
#include <list>
#include <vector>
#include "esp_task_wdt.h"
#include <config.hpp>
#include <mb3/defaults.hpp>

// #define EXECUTION_TIMES_SIZE 1000

// #define print_heap() \
//     log("Heap  0x%06X %2.0f%% Free", ESP.getFreeHeap(), (float)ESP.getFreeHeap() / ESP.getHeapSize() * 100.f); \
//     log("PSRAM 0x%06X %2.0f%% Free", ESP.getFreePsram(), (float)ESP.getFreePsram() / ESP.getPsramSize() * 100.f)

// #define print_heap_d() \
//     log_d("Heap  0x%06X %2.0f%% Free", ESP.getFreeHeap(), (float)ESP.getFreeHeap() / ESP.getHeapSize() * 100.f); \
//     log_d("PSRAM 0x%06X %2.0f%% Free", ESP.getFreePsram(), (float)ESP.getFreePsram() / ESP.getPsramSize() * 100.f)

inline uint32_t get_8bit_heap_size(void)
{
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_8BIT);
    return info.total_free_bytes + info.total_allocated_bytes;
}

#define print_heap() \
{ \
    static char buffer[512] = {0}; \
    sprintf(buffer, "[%6u][I] Heap  0x%06X %2.0f%%%% Free\r\n", (unsigned long) (esp_timer_get_time() / 1000ULL), \
        ESP.getFreeHeap(), (float)ESP.getFreeHeap() / ESP.getHeapSize() * 100.f); \
    Log::_log(buffer); \
    printf(buffer); \
    sprintf(buffer, "[%6u][I] PSRAM 0x%06X %2.0f%%%% Free\r\n", (unsigned long) (esp_timer_get_time() / 1000ULL), \
        ESP.getFreePsram(), (float)ESP.getFreePsram() / ESP.getPsramSize() * 100.f); \
    Log::_log(buffer); \
    printf(buffer); \
}

#define print_heap_d() \
{ \
    static char buffer[512] = {0}; \
    sprintf(buffer, "[%6u][I] Heap  0x%06X %2.0f%%%% Free\r\n", (unsigned long) (esp_timer_get_time() / 1000ULL), \
        ESP.getFreeHeap(), (float)ESP.getFreeHeap() / ESP.getHeapSize() * 100.f); \
    printf(buffer); \
    sprintf(buffer, "[%6u][I] PSRAM 0x%06X %2.0f%%%% Free\r\n", (unsigned long) (esp_timer_get_time() / 1000ULL), \
        ESP.getFreePsram(), (float)ESP.getFreePsram() / ESP.getPsramSize() * 100.f); \
    printf(buffer); \
}

class ISystem;

class Systems {
public:
    static std::vector<std::shared_ptr<ISystem>> systems;
};

class ISystem {
public:
    ISystem(std::string name) {
        this->name = name;
        // execution_times.resize(EXECUTION_TIMES_SIZE);
    }

    void create_execution_times() {
        if (frequency > 0) {
            execution_time_size = 5000 / frequency;
        }
        execution_times = (uint32_t *)heap_caps_calloc(execution_time_size, sizeof(uint32_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT);
    }

    virtual bool setup_impl() = 0;

    virtual void task_impl() {

    }

    // virtual void start() {

    // }

    virtual void stop() {
        vTaskSuspend(taskHandle);
    }

    void add_time(uint32_t time) {
        if (execution_times) {
            execution_times[execution_time_index] = time;
            execution_time_index = (execution_time_index + 1) % execution_time_size;
        }
    }

    // us
    float average_time() {
        if (execution_times) {
            static float sum;
            sum = 0.f;
            max_execution_time = 0.f;
            for (int i = 0; i < execution_time_size; i++) {
                sum += execution_times[i];
                max_execution_time = std::max(max_execution_time, (float)execution_times[i]);
            }
            all_max_execution_time = std::max(all_max_execution_time, max_execution_time);
            return sum / execution_time_size;
        } else {
            return 0.f;
        }
    }

    std::string name;
    int stacksize;
    int priority;
    int core;

    // ms
    TickType_t frequency;
    
    bool initialized = false;
    bool started = false;
    TaskHandle_t taskHandle = nullptr;
    float all_max_execution_time = 0;
    float max_execution_time = 0;
    int execution_time_index = 0;
    int execution_time_size = -1;
    // int64_t execution_times[EXECUTION_TIMES_SIZE] = {0};
    uint32_t * execution_times = nullptr;
    TickType_t xLastWakeTime;
    bool should_start = false;
};

inline bool wdt_init = false;

template <class T>
class System : public ISystem {
public:
    System(std::string name) : ISystem(name) {
        if (!wdt_init) {
            wdt_init = true;
            // watchdog timer setup, in seconds
            esp_task_wdt_init(2, true);
        }
    }

    static inline std::shared_ptr<T> instance;

    /// @brief Setup and start task (if stacksize && priority)
    /// @param stacksize 0x1000 is safe
    /// @param priority 19 for standard op
    /// @param core 1 for most things
    /// @param frequency time in ms
    static void setup(int stacksize = -1, int priority = -1, BaseType_t core = 1, TickType_t frequency = 100) {
        create(stacksize, priority, core, frequency);

        instance->start_impl();
    }

    /// @brief Setup task (if stacksize && priority)
    /// @param stacksize 0x1000 is safe
    /// @param priority 19 for standard op
    /// @param core 1 for most things
    /// @param frequency time in ms
    static void create(int stacksize = -1, int priority = -1, BaseType_t core = 1, TickType_t frequency = 100) {
        if (instance)
            return;
        
        instance = std::make_shared<T>();
        Systems::systems.push_back(instance);

        instance->stacksize = stacksize;
        instance->priority = priority;
        instance->core = core;
        instance->frequency = frequency;

        instance->create_execution_times();

        auto start_time = esp_timer_get_time();
        MB3_LOG("[%6u][I][%s] Running setup\r\n", (unsigned long) (esp_timer_get_time() / 1000ULL), instance->name.c_str());
        instance->should_start = instance->setup_impl();
        if (instance->should_start) {
            instance->initialized = true;
            auto diff = esp_timer_get_time() - start_time;
            MB3_LOG("[%6u][I][%s] System Initialized in %0.6f\r\n", (unsigned long) (esp_timer_get_time() / 1000ULL), instance->name.c_str(), diff / 1000000.0);
        } else {
            MB3_LOG("[%6u][E][%s] Couldn't initialize System\r\n", (unsigned long) (esp_timer_get_time() / 1000ULL), instance->name.c_str());
            // basic_log("[ERROR] Couldn't initialize system\r\n");
        }
    }

    static void start() {
        instance->start_impl();
    }

    void start_impl() {
        if (!initialized) {
            MB3_LOG("[%6u][E][%s] System not initialized\r\n", (unsigned long) (esp_timer_get_time() / 1000ULL), name.c_str());
            return;
        }
        // print_heap_d();
        if (should_start) {
            // print_heap_d();
            if (priority != -1 && stacksize != -1) {
                auto res = xTaskCreatePinnedToCore(task, name.c_str(), stacksize, this, priority, &taskHandle, core);
                if (res == pdPASS) {
                    esp_task_wdt_add(taskHandle);
                } else if (res == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY){
                    MB3_LOG("[%6u][E][%s] Couldn't alloc memory for task\r\n", (unsigned long) (esp_timer_get_time() / 1000ULL), name.c_str());
                    // basic_log("[ERROR] Couldn't alloc memory\r\n");
                } else {
                    MB3_LOG("[%6u][E][%s] Couldn't start task: %d\r\n", (unsigned long) (esp_timer_get_time() / 1000ULL), name.c_str(), res);
                    // basic_log("[ERROR] Couldn't start task\r\n");
                }
            }
        }
    }

    static void task(void * parameter) {
        auto instance = static_cast<ISystem *>(parameter);
        MB3_LOG("[%6u][I][%s] System Task Started\r\n", (unsigned long) (esp_timer_get_time() / 1000ULL), instance->name.c_str());
        instance->xLastWakeTime = xTaskGetTickCount();
        int64_t start_time;

        for (;;) {
            vTaskDelayUntil(&instance->xLastWakeTime, instance->frequency);
            esp_task_wdt_reset();
            start_time = esp_timer_get_time();
            instance->task_impl();
            esp_task_wdt_reset();
            instance->add_time(esp_timer_get_time() - start_time);
        }
    }
    
    static std::shared_ptr<T> get() 
    {
        return std::dynamic_pointer_cast<T>(instance);
    }

    // virtual std::string name() = 0;

};

// class TimeSystem : public System<TimeSystem, 8> {
// public:
//     inline static const char * NAME = "Time";

//     static bool setup_impl(void) {
//         return true;
//     }

//     static void task_impl(void * parameter) {
        
//     }
// };