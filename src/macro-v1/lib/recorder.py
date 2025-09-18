from pynput import mouse, keyboard
import time
import json

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


def on_press(key, mouse_listener, keyboard_listener, stop_key):
    try:
        k = key.char
    except AttributeError:
        k = str(key)

    if str(key) == stop_key:
        mouse_listener.stop()
        keyboard_listener.stop()
        return

    events.append({
        'type': 'key_press',
        'time': time.time() - start_time,
        'key': k
    })


def on_release(key):
    k = getattr(key, 'char', str(key))
    events.append({
        'type': 'key_release',
        'time': time.time() - start_time,
        'key': k
    })


def record_macro(path, stop_callback=None, stop_key='Key.esc', start_callback=None):

    global start_time, events
    events = []
    start_time = time.time()

    mouse_listener = mouse.Listener(on_move=on_move, on_click=on_click, on_scroll=on_scroll)
    keyboard_listener = keyboard.Listener(on_release=on_release, on_press=lambda key: on_press(key, mouse_listener, keyboard_listener, stop_key))

    mouse_listener.start()
    keyboard_listener.start()

    mouse_listener.join()
    keyboard_listener.join()

    with open(path, 'w') as f:
        json.dump(events, f, indent=4)

    print('Macro saved to', path)
    if stop_callback:
        stop_callback()
