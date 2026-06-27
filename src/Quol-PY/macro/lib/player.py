import json
import time
import threading

from pynput.mouse import Controller as MouseController, Button
from pynput.keyboard import Controller as KeyboardController, Key
from pynput import keyboard

stop_event = threading.Event()
pressed_keys = set()
listener = None

WM_KEYDOWN = 256
WM_KEYUP = 257

STR_TO_PY = {
    'backspace': Key.backspace,
    'tab': Key.tab,
    'enter': Key.enter,
    'pause': Key.pause,
    'caps_lock': Key.caps_lock,
    'esc': Key.esc,
    ' ': Key.space,
    'space': Key.space,
    'page_up': Key.page_up,
    'page_down': Key.page_down,
    'end': Key.end,
    'home': Key.home,
    'left': Key.left,
    'up': Key.up,
    'right': Key.right,
    'down': Key.down,
    'print_screen': Key.print_screen,
    'insert': Key.insert,
    'delete': Key.delete,

    'f1': Key.f1,
    'f2': Key.f2,
    'f3': Key.f3,
    'f4': Key.f4,
    'f5': Key.f5,
    'f6': Key.f6,
    'f7': Key.f7,
    'f8': Key.f8,
    'f9': Key.f9,
    'f10': Key.f10,
    'f11': Key.f11,
    'f12': Key.f12,

    'shift': Key.shift,
    'shift_r': Key.shift_r,
    'ctrl': Key.ctrl,
    'ctrl_r': Key.ctrl_r,
    'alt': Key.alt,
    'alt_r': Key.alt_r,
}


def win32_event_filter(msg, data):
    if msg == WM_KEYDOWN:
        pressed_keys.add(data.vkCode)
        # 162 is VK_CONTROL, 27 is VK_ESCAPE
        if 162 in pressed_keys and 27 in pressed_keys:
            print("Ctrl + Esc detected. Stopping playback.")
            stop_event.set()
            listener.suppress_event()

    elif msg == WM_KEYUP:
        pressed_keys.discard(data.vkCode)


def start_user_activity_monitor():
    global listener
    stop_event.clear()
    listener = keyboard.Listener(win32_event_filter=win32_event_filter)
    listener.start()
    return listener


def _play_macro_thread(path, rep=1, scale=1, speed=1):
    m = MouseController()
    kb = KeyboardController()

    try:
        with open(path, "r") as f:
            events = json.load(f)
    except Exception as e:
        print(f"Failed to load macro: {e}")
        return

    keyboard_listener = start_user_activity_monitor()

    try:
        for _ in range(rep):
            if stop_event.is_set():
                print("Playback interrupted by user. 1")
                break

            for i, event in enumerate(events):
                if stop_event.is_set():
                    print("Playback interrupted by user. 2")
                    return

                if i > 0:
                    delay = (event['time'] - events[i - 1]['time']) / speed
                    time.sleep(delay)

                if event['type'] == 'move':
                    m.position = (event['pos'][0] * scale, event['pos'][1] * scale)

                elif event['type'] == 'click':
                    m.position = (event['pos'][0] * scale, event['pos'][1] * scale)
                    btn = Button.left if 'left' in event['btn'] else Button.right
                    if event['pressed']:
                        m.press(btn)
                    else:
                        m.release(btn)

                elif event['type'] == 'scroll':
                    m.scroll(event['dx'], event['dy'])

                elif event['type'] == 'key_press':
                    key = event['key']
                    try:
                        kb.press(STR_TO_PY.get(key, key))
                    except Exception as e:
                        print(f"Key press error: {e}")

                elif event['type'] == 'key_release':
                    key = event['key']
                    try:
                        kb.release(STR_TO_PY.get(key, key))
                    except Exception as e:
                        print(f"Key release error: {e}")
    finally:
        keyboard_listener.stop()
        pressed_keys.clear()
        print("Done playback")


def play_macro(path, rep=1, scale=1, speed=1):
    stop_event.clear()
    t = threading.Thread(
        target=_play_macro_thread,
        args=(path, rep, scale, speed),
        daemon=True
    )
    t.start()
