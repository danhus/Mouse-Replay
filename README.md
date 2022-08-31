# Mouse Recorder for Windows
This is a tool for recording and replaying mouse events. It will install a low-level hook via _SetWindowsHookExA_, and will use this hook to record mouse movement, scrolling and clicking, including the middle mouse button and two side buttons. Only one instance can be recording or replaying, but multiple recording can be stored. At will, a recording can be played back, with a few options available to alter its behavior.

### Usage:
```
#include "mousereplay.h"

//...

MouseReplay mr1;
```
In the constructor of ```MouseReplay``` the hook will be installed.

```
mr1.record();
//...
mr1.stop();
```
mr1 will begin and stop recording, respectively. Alternatively, ```record(int)``` can be used to record for a preset duration:
```
mr1.record(3500); // 3500 milliseconds
```

Afterwards, the recording can be played back:
```
mr1.replay();
```
```replay()``` takes three boolean arguments, all of which are defaulted as following:
```
void MouseReplay::replay(
  bool relative = false,
  bool movement_only = false,
  bool blocking = true
);
```
```relative``` when true, replays start from the mouse position at the time of playback, and will otherwise use absolute coordinates

```movement_only``` when true, will exclude clicks and scrolls during playback

```blocking``` when false, will allow you to move your mouse at the same time that it's being played

```
mr1.reset();
```
```reset()``` discards the stored recording. ```duration()``` will return the duration (in milliseconds) of the stored recording.



### example.cpp
example.cpp is a very simple program that makes use of this tool. While running, you can press __Numpad*__ to begin recording, __Numpad-__ to stop recording, and __Numpad+__ to begin replay.
