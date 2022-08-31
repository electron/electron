# InputEvent Object

* `type` string - One of "undefined", "mousedown", "mouseup", "mousemove", "mouseenter", "mouseleave", "contextmenu", "mousewheel", "rawkeydown", "keydown", "keyup", "char", "gesturescrollbegin", "gesturescrollend", "gesturescrollupdate", "gestureflingstart", "gestureflingcancel", "gesturepinchbegin", "gesturepinchend", "gesturepinchupdate", "gesturetapdown", "gestureshowpress", "gesturetap", "gesturetapcancel", "gestureshortpress", "gesturelongpress", "gesturelongtap", "gesturetwofingertap", "gesturetapunconfirmed", "gesturedoubletap", "touchstart", "touchmove", "touchend", "touchcancel", "touchscrollstarted", "pointerdown", "pointerup", "pointermove", "pointerrawupdate", "pointercancel", or "pointercauseduaaction".
* `modifiers` string[] (optional) - An array of modifiers of the event, can
  be `shift`, `control`, `ctrl`, `alt`, `meta`, `command`, `cmd`, `isKeypad`,
  `isAutoRepeat`, `leftButtonDown`, `middleButtonDown`, `rightButtonDown`,
  `capsLock`, `numLock`, `left`, `right`.
