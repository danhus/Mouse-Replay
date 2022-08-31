#ifndef MOUSEREPLAY_H
#define MOUSEREPLAY_H

#include <windows.h>
#include <winuser.h>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

class MouseReplay {
 private:
  using Time_t = DWORD;
  using Coord_t = int;

 public:
  MouseReplay();
  ~MouseReplay();
  bool record();
  bool record(Time_t);
  static void stop();
  void replay(
      bool relative = false,
      bool movement_only = false,
      bool blocking = true);
  void reset();
  [[nodiscard]] Time_t duration() const;

 private:
  enum EventType : char;
  struct MouseEvent {
    Coord_t x; // coords end up being off by 1 pixel (!)
    Coord_t y;
    Time_t time;
    EventType et;
  };
  std::vector<MouseEvent> m_recorded;
  static HHOOK m_hook;
  static int m_count;
  static LRESULT CALLBACK
  HookCallbackProc(int, WPARAM, LPARAM); // fix time; record time with chrono
  static bool m_blocking_active;
  static MouseReplay* m_active; // LOCKS!
  static int m_width;
  static int m_height;
};

#endif