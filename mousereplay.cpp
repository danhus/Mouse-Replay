#include "mousereplay.h"
#include <chrono>
#include <iostream>
#include <thread>

HHOOK MouseReplay::m_hook{nullptr};
int MouseReplay::m_count{0};
MouseReplay* MouseReplay::m_active{nullptr};
int MouseReplay::m_width{0};
int MouseReplay::m_height{0};
bool MouseReplay::m_blocking_active{false};
bool MouseReplay::m_replay_active{false};

enum MouseReplay::EventType : char {
  Move,
  LMB_down,
  LMB_up,
  RMB_down,
  RMB_up,
  MMB_down,
  MMB_up,
  ScrollUp,
  ScrollUp_2,
  ScrollUp_3,
  ScrollDown,
  ScrollDown_2,
  ScrollDown_3,
  Side1_down,
  Side1_up,
  Side2_down,
  Side2_up,
  Invalid
};

MouseReplay::MouseReplay() {
  ++m_count;
  if (!m_hook) {
    m_hook = SetWindowsHookExA(WH_MOUSE_LL, HookCallbackProc, nullptr, 0);
    if (!m_hook)
      std::cerr << "Failed to install mouse hook.\n";
  }
  if (!m_width || !m_height) {
    m_width = GetSystemMetrics(SM_CXSCREEN);
    m_height = GetSystemMetrics(SM_CYSCREEN);
    if (!m_width || !m_height)
      std::cerr << "Could not determine screen resolution.\n";
  }
}
MouseReplay::MouseReplay(const MouseReplay& rhs) : m_recorded(rhs.m_recorded) {
  ++m_count;
}
MouseReplay::MouseReplay(MouseReplay&& rhs) noexcept
    : m_recorded(std::move(rhs.m_recorded)) {
  ++m_count;
}
MouseReplay& MouseReplay::operator=(MouseReplay&& rhs) noexcept {
  m_recorded = std::move(rhs.m_recorded);
  return *this;
}
MouseReplay::~MouseReplay() {
  if (--m_count == 0) {
    UnhookWindowsHookEx(m_hook);
  }
}

bool MouseReplay::record() {
  if (m_active)
    return false;
  m_active = this;
  return true;
}

bool MouseReplay::record(Time_t duration) {
  bool ret = duration > 0 && record();
  if (ret) {
    std::thread stopper_worker{[=]() {
      // std::async
      std::this_thread::sleep_for(std::chrono::milliseconds(duration));
      stop();
    }};
    stopper_worker.detach();
  }

  return ret;
}
void MouseReplay::stop() {
  m_active = nullptr;
}
void MouseReplay::replay(bool relative, bool movement_only, bool blocking) {
  /*"The time field in the MOUSE­INPUT structure is not for introducing delays
   * in playback."*/

  if (m_recorded.size() > 0 && !m_replay_active) {
    std::thread replay_worker{
        [&](bool relative, bool movement_only, bool blocking) {
          if (blocking)
            m_blocking_active = true;
          m_replay_active = true;

          INPUT input;
          input.type = INPUT_MOUSE;
          input.mi.time = 0;
          input.mi.dwExtraInfo = 0;

          int offset_x = 0;
          int offset_y = 0;
          if (relative) {
            POINT p;
            GetCursorPos(&p);
            offset_x = p.x - m_recorded[0].x;
            offset_y = p.y - m_recorded[0].y;
          }

          auto generate_input = [&](auto i) {
            input.mi.dx = ((m_recorded[i].x + offset_x) * 65535) / m_width;
            input.mi.dy = ((m_recorded[i].y + offset_y) * 65535) / m_height;

            input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
            input.mi.mouseData = 0;
            switch (m_recorded[i].et) {
              case Move:
                input.mi.dwFlags |= MOUSEEVENTF_MOVE;
                break;
              case LMB_down:
                input.mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
                break;
              case LMB_up:
                input.mi.dwFlags |= MOUSEEVENTF_LEFTUP;
                break;
              case RMB_down:
                input.mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
                break;
              case RMB_up:
                input.mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
                break;
              case MMB_down:
                input.mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
                break;
              case MMB_up:
                input.mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
                break;
              case ScrollUp:
                input.mi.dwFlags |= MOUSEEVENTF_WHEEL;
                input.mi.mouseData = 1 * WHEEL_DELTA;
                break;
              case ScrollUp_2:
                input.mi.dwFlags |= MOUSEEVENTF_WHEEL;
                input.mi.mouseData = 2 * WHEEL_DELTA;
                break;
              case ScrollUp_3:
                input.mi.dwFlags |= MOUSEEVENTF_WHEEL;
                input.mi.mouseData = 3 * WHEEL_DELTA;
                break;
              case ScrollDown:
                input.mi.dwFlags |= MOUSEEVENTF_WHEEL;
                input.mi.mouseData = -1 * WHEEL_DELTA;
                break;
              case ScrollDown_2:
                input.mi.dwFlags |= MOUSEEVENTF_WHEEL;
                input.mi.mouseData = -2 * WHEEL_DELTA;
                break;
              case ScrollDown_3:
                input.mi.dwFlags |= MOUSEEVENTF_WHEEL;
                input.mi.mouseData = -3 * WHEEL_DELTA;
                break;
              case Side1_down:
                input.mi.dwFlags |= MOUSEEVENTF_XDOWN;
                input.mi.mouseData = XBUTTON1;
                break;
              case Side1_up:
                input.mi.dwFlags |= MOUSEEVENTF_XUP;
                input.mi.mouseData = XBUTTON1;
                break;
              case Side2_down:
                input.mi.dwFlags |= MOUSEEVENTF_XDOWN;
                input.mi.mouseData = XBUTTON2;
                break;
              case Side2_up:
                input.mi.dwFlags |= MOUSEEVENTF_XUP;
                input.mi.mouseData = XBUTTON2;
                break;
              default:
                std::cerr << "error, et is unknown\n";
                break;
            }
          };
          for (int i = 0; i < m_recorded.size() - 1; ++i) {
            auto delay = m_recorded[i + 1].time - m_recorded[i].time;
            if (0 < delay && delay < 17)
              delay = 1;
            generate_input(i);

            if (!movement_only || m_recorded[i].et == Move)
              SendInput(1, &input, sizeof(INPUT));
            if (delay)
              std::this_thread::sleep_for(std::chrono::milliseconds(delay));
          }
          generate_input(m_recorded.size() - 1);
          if (!movement_only || m_recorded[m_recorded.size() - 1].et == Move)
            SendInput(1, &input, sizeof(INPUT));
          if (blocking)
            m_blocking_active = false;
          m_replay_active = false;
        },
        relative,
        movement_only,
        blocking};
    replay_worker.detach();
  };
}

void MouseReplay::reset() {
  m_recorded = {};
}

MouseReplay::Time_t MouseReplay::duration() const {
  if (m_recorded.size() > 0)
    return m_recorded.back().time - m_recorded.front().time;
  else
    return 0;
};

LRESULT CALLBACK
MouseReplay::HookCallbackProc(int nCode, WPARAM wParam, LPARAM lParam) {
  auto lPar = *reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
  if (m_blocking_active && lPar.flags != LLMHF_INJECTED) {
    return -1;
  }

  if (nCode >= 0 && m_active) {
    EventType et = Invalid;
    switch (wParam) {
      case WM_MOUSEMOVE:
        et = Move;
        break;
      case WM_LBUTTONDOWN:
        et = LMB_down;
        break;
      case WM_LBUTTONUP:
        et = LMB_up;
        break;
      case WM_RBUTTONDOWN:
        et = RMB_down;
        break;
      case WM_RBUTTONUP:
        et = RMB_up;
        break;
      case 519:
        et = MMB_down;
        break;
      case 520:
        et = MMB_up;
        break;
      case 522:
        if (lPar.mouseData == 1 * 7864320)
          et = ScrollUp;
        else if (lPar.mouseData == 2 * 7864320)
          et = ScrollUp_2;
        else if (lPar.mouseData == 3 * 7864320)
          et = ScrollUp_3;
        else if (lPar.mouseData == 4287102976 - (long long)0 * 7864320)
          et = ScrollDown;
        else if (lPar.mouseData == 4287102976 - (long long)1 * 7864320)
          et = ScrollDown_2;
        else if (lPar.mouseData == 4287102976 - (long long)2 * 7864320)
          et = ScrollDown_3;
        else
          std::cerr << "scroll unknown:" << lPar.mouseData << "\n";
        break;
      case 523:
        if (lPar.mouseData == 131072)
          et = Side2_down;
        else if (lPar.mouseData == 65536)
          et = Side1_down;
        break;
      case 524:
        if (lPar.mouseData == 131072)
          et = Side2_up;
        else if (lPar.mouseData == 65536)
          et = Side1_up;
        break;
      default:
        std::cerr << wParam << '\n';
        break;
    }

    m_active->m_recorded.push_back({lPar.pt.x, lPar.pt.y, lPar.time, et});
  }
  return CallNextHookEx(m_hook, nCode, wParam, lParam);
}
