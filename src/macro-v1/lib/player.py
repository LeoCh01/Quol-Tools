from pynput.mouse import Controller as MouseController, Button
from pynput.keyboard import Controller as KeyboardController, Key
import json
import time


def play_macro(path, scale=1):
    mouse = MouseController()
    keyboard = KeyboardController()

    with open(path, "r") as f:
        events = json.load(f)

    for i, event in enumerate(events):
        if i > 0:
            delay = event['time'] - events[i - 1]['time']
            time.sleep(delay)

        if event['type'] == 'move':
            mouse.position = (event['pos'][0] * scale, event['pos'][1] * scale)

        elif event['type'] == 'click':
            mouse.position = (event['pos'][0] * scale, event['pos'][1] * scale)
            btn = Button.left if 'left' in event['btn'] else Button.right
            if event['pressed']:
                mouse.press(btn)
            else:
                mouse.release(btn)

        elif event['type'] == 'scroll':
            mouse.scroll(event['dx'], event['dy'])

        elif event['type'] == 'key_press':
            key = event['key']
            try:
                keyboard.press(eval(key) if "Key." in key else key)
            except Exception as e:
                print(e)
                pass

        elif event['type'] == 'key_release':
            key = event['key']
            try:
                keyboard.release(eval(key) if "Key." in key else key)
            except Exception as e:
                print(e)
                pass
