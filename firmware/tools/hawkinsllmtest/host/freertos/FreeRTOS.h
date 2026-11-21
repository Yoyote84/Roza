// Port de test hôte repris de REZOR/cardputer-ai (MIT).
// Les sémaphores et tâches FreeRTOS sont représentés par std::thread.
#pragma once
#include <condition_variable>
#include <mutex>
#include <thread>
#define portMAX_DELAY 0
typedef int BaseType_t;
typedef void* TaskHandle_t;
inline BaseType_t xPortGetCoreID() { return 0; }
struct HostBinarySem {
  std::mutex m;
  std::condition_variable cv;
  bool flag = false;
  void give() { std::lock_guard<std::mutex> lock(m); flag = true; cv.notify_one(); }
  void take() {
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [&]{ return flag; });
    flag = false;
  }
};
typedef HostBinarySem* SemaphoreHandle_t;
inline SemaphoreHandle_t xSemaphoreCreateBinary() { return new HostBinarySem(); }
inline void xSemaphoreGive(SemaphoreHandle_t s) { s->give(); }
inline void xSemaphoreTake(SemaphoreHandle_t s, int) { s->take(); }
typedef void (*TaskFunction_t)(void*);
inline BaseType_t xTaskCreatePinnedToCore(TaskFunction_t fn, const char*, int,
                                          void* arg, int, TaskHandle_t* handle,
                                          BaseType_t) {
  std::thread worker(fn, arg);
  *handle = reinterpret_cast<TaskHandle_t>(1);
  worker.detach();
  return 1;
}
