#include <iostream>
#include "mousereplay.h"

HHOOK hook;
MouseReplay mr;

LRESULT CALLBACK KBCallBackProc(int nCode, WPARAM wParam, LPARAM lParam) {
  auto lPar = *reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

  if (nCode >= 0) {
    if (wParam == WM_KEYDOWN && lPar.scanCode == 55) { //*
      mr.reset();
      mr.record();
      return -1;
    } else if (wParam == WM_KEYDOWN && lPar.scanCode == 74) { //-
      mr.stop();
      return -1;
    } else if (wParam == WM_KEYDOWN && lPar.scanCode == 78) { //+
      mr.replay();
      return -1;
    }
  }
  return CallNextHookEx(hook, nCode, wParam, lParam);
};

int main() {
  hook = SetWindowsHookExA(WH_KEYBOARD_LL, KBCallBackProc, nullptr, 0);
  if (!hook)
    std::cerr << "keyboard hook failed to install\n";

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0))
    ;
}