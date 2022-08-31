#include <iostream>
#include "mousereplay.h"

int main() {
  MouseReplay mr;
  std::thread t(
      [](MouseReplay& x) {
        x.record(4000);
        std::this_thread::sleep_for(std::chrono::milliseconds(4100));
        // x.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        x.replay(true);
        return;
      },
      std::ref(mr));
  t.detach();
  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0))
    ;
}