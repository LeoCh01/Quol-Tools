from pynput import mouse, keyboard
import time
import json
import threading

VK_TO_STR = {
    0x08: 'backspace',
    0x09: 'tab',
    0x0D: 'enter',
    0x13: 'pause',
    0x14: 'caps_lock',
    0x1B: 'esc',
    0x20: ' ',
    0x21: 'page_up',
    0x22: 'page_down',
    0x23: 'end',
    0x24: 'home',
    0x25: 'left',
    0x26: 'up',
    0x27: 'right',
    0x28: 'down',
    0x2C: 'print_screen',
    0x2D: 'insert',
    0x2E: 'delete',

    # Numbers 0-9
    0x30: '0',
    0x31: '1',
    0x32: '2',
    0x33: '3',
    0x34: '4',
    0x35: '5',
    0x36: '6',
    0x37: '7',
    0x38: '8',
    0x39: '9',

    # Letters a-z
    0x41: 'a',
    0x42: 'b',
    0x43: 'c',
    0x44: 'd',
    0x45: 'e',
    0x46: 'f',
    0x47: 'g',
    0x48: 'h',
    0x49: 'i',
    0x4A: 'j',
    0x4B: 'k',
    0x4C: 'l',
    0x4D: 'm',
    0x4E: 'n',
    0x4F: 'o',
    0x50: 'p',
    0x51: 'q',
    0x52: 'r',
    0x53: 's',
    0x54: 't',
    0x55: 'u',
    0x56: 'v',
    0x57: 'w',
    0x58: 'x',
    0x59: 'y',
    0x5A: 'z',

    # Function keys F1-F12
    0x70: 'f1',
    0x71: 'f2',
    0x72: 'f3',
    0x73: 'f4',
    0x74: 'f5',
    0x75: 'f6',
    0x76: 'f7',
    0x77: 'f8',
    0x78: 'f9',
    0x79: 'f10',
    0x7A: 'f11',
    0x7B: 'f12',

    # Numpad keys
    0x60: '0',
    0x61: '1',
    0x62: '2',
    0x63: '3',
    0x64: '4',
    0x65: '5',
    0x66: '6',
    0x67: '7',
    0x68: '8',
    0x69: '9',
    0x6A: '*',
    0x6B: '+',
    0x6D: '-',
    0x6E: '.',
    0x6F: '/',

    # Other punctuation keys
    0xBA: ';',
    0xBB: '=',
    0xBC: ',',
    0xBD: '-',
    0xBE: '.',
    0xBF: '/',
    0xC0: '`',
    0xDB: '[',
    0xDC: '\\',
    0xDD: ']',
    0xDE: '\'',

    # Modifier keys ?
    0xA0: 'shift',
    0xA1: 'shift_r',
    0xA2: 'ctrl',
    0xA3: 'ctrl_r',
    0xA4: 'alt',
    0xA5: 'alt_r',

    # 0x10: 'shift',
    # 0x11: 'ctrl',
    # 0x12: 'alt',
}

events = []
start_time = time.time()


def on_move(x, y):
    events.append({
        'type': 'move',
        'time': time.time() - start_time,
        'pos': (x, y)
    })


def on_click(x, y, btn, pressed):
    events.append({
        'type': 'click',
        'time': time.time() - start_time,
        'pos': (x, y),
        'btn': str(btn),
        'pressed': pressed
    })


def on_scroll(x, y, dx, dy):
    events.append({
        'type': 'scroll',
        'time': time.time() - start_time,
        'pos': (x, y),
        'dx': dx,
        'dy': dy
    })


def record_macro(path, stop_callback=None, stop_key='Key.esc'):
    global start_time, events
    events = []
    start_time = time.time()

    def win32_event_filter(msg, data):
        if msg == 256:
            k = VK_TO_STR.get(data.vkCode, f'vk_{data.vkCode}')
            if k in (stop_key, f'Key.{stop_key}'):
                mouse_listener.stop()
                keyboard_listener.stop()
                return
            events.append({
                'type': 'key_press',
                'time': time.time() - start_time,
                'key': k
            })
        elif msg == 257:
            k = VK_TO_STR.get(data.vkCode, f'vk_{data.vkCode}')
            events.append({
                'type': 'key_release',
                'time': time.time() - start_time,
                'key': k
            })

    def on_stop():
        with open(path, 'w') as f:
            json.dump(events, f, indent=4)
        print('Macro saved to', path)
        if stop_callback:
            stop_callback()

    mouse_listener = mouse.Listener(
        on_move=on_move,
        on_click=on_click,
        on_scroll=on_scroll
    )

    keyboard_listener = keyboard.Listener(
        win32_event_filter=win32_event_filter
    )

    def wait_for_listeners():
        mouse_listener.join()
        keyboard_listener.join()
        on_stop()

    mouse_listener.start()
    keyboard_listener.start()

    threading.Thread(target=wait_for_listeners, daemon=True).start()
