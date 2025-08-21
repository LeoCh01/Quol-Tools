import base64
import datetime

import httpx
from PySide6.QtCore import QTimer, QThread, Signal
from qasync import asyncSlot
import ollama


class AI:
    def __init__(self, main_window: 'MainWindow', chat_window: 'ChatWindow'):
        self.main_window = main_window
        self.chat_window = chat_window
        self.ollama_client = None
        self.current_type = None

        self.is_img = True
        self.is_hist = True
        self.text_content = ''
        self.loading_counter = 0

        self.max_hist = self.main_window.config['config']['max_history']
        self.history = []

    def prompt(self, model, d):
        self.current_type = model
        self.chat_window.show()
        self.chat_window.set_output('<p>Loading...</p><br><br><br><br>')
        self.chat_window.scroll_to_bottom()

        if self.main_window.test_response:
            self.chat_window.chat_response.setHtml(self.main_window.test_response)
            return

        if model == 'ollama':
            self.ollama(d['model'], d['prompt'])
            return
        elif model == 'gemini':
            self.handle_response(*self.gemini(d['model'], d['prompt'], d['apikey']))
        elif model == 'groq':
            self.handle_response(*self.groq(d['model'], d['prompt'], d['apikey']))

    @asyncSlot()
    async def handle_response(self, url, headers, data):
        self.loading_counter = 0

        def on_loading():
            self.loading_counter += 0.1
            self.chat_window.set_output(f'<p>Loading... ({self.loading_counter:.1f}s)</p>')

        try:
            timer = QTimer()
            timer.timeout.connect(on_loading)
            timer.start(100)

            async with httpx.AsyncClient(timeout=httpx.Timeout(60.0, connect=30.0)) as client:
                response = await client.post(url, headers=headers, json=data)

            timer.stop()
            res = response.json()

            if 'error' in res:
                raise Exception(f'Error: {res["error"]["message"]}')

            if self.current_type == 'gemini':
                self.text_content = res['candidates'][0]['content']['parts'][0]['text']
            elif self.current_type == 'groq':
                self.text_content = res['choices'][0]['message']['content']

            self.add_history(self.current_type, self.text_content, None, False)
            self.chat_window.set_output()
        except Exception as e:
            self.text_content = str(e)
            self.chat_window.set_output(f'Error: {self.text_content}')
            self.history.clear()
        finally:
            self.main_window.set_button_loading_state(False)

    def add_history(self, model, text, img_data, is_user):
        if self.is_hist:
            if is_user:
                self.history.append({'role': 'user', 'text': text})
                if self.is_img:
                    self.history[-1]['image'] = img_data
            else:
                self.history.append({'role': 'model', 'text': text})

        with open(self.main_window.window_info.path + f'/res/{model}.log', 'a', encoding='utf-8') as f:
            if is_user:
                f.write(f'{datetime.datetime.now()}\nQ: {text}\n')
            else:
                f.write(f'A: {text.replace("\n\n", "\n")}\n\n')

        if len(self.history) > (self.max_hist - 1) * 2:
            self.history.pop(0)

    def ollama(self, model, prompt):
        image_path = None
        if self.is_img:
            image_path = self.main_window.window_info.path + '/res/img/screenshot.png'

        self.thread = OllamaThread(model=model, prompt=prompt, image_path=image_path)

        self.thread.output_signal.connect(self.chat_window.set_output)
        self.thread.finished_signal.connect(lambda text: self.on_ollama_finished(prompt, text))
        self.thread.error_signal.connect(self.on_ollama_error)

        self.thread.start()

    def on_ollama_finished(self, prompt, text):
        with open(self.main_window.window_info.path + '/res/ollama.log', 'a', encoding='utf-8') as f:
            f.write(f'{datetime.datetime.now()}\nQ: {prompt}\nA: {text.replace("\n\n", "\n")}\n\n')

        self.text_content = text
        self.main_window.set_button_loading_state(False)

    def on_ollama_error(self, error):
        self.chat_window.set_output(f'Error: {error}')
        self.main_window.set_button_loading_state(False)

    def gemini(self, model, prompt, key):
        key = key or 'APIKEY'
        url = f'https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent?key={key}'
        headers = {'Content-Type': 'application/json'}
        data = {'contents': []}
        img_data = None

        if self.is_hist:
            for h in self.history:
                data['contents'].append({'role': h['role'], 'parts': [{'text': h['text']}]})

                if 'image' in h:
                    data['contents'][-1]['parts'].append(
                        {'inline_data': {'mime_type': 'image/png', 'data': h['image']}}
                    )

        cur = {'role': 'user', 'parts': [{'text': prompt}]}

        if self.is_img:
            with open(self.main_window.window_info.path + '/res/img/screenshot.png', 'rb') as img_file:
                img_data = base64.b64encode(img_file.read()).decode('utf-8')
                cur['parts'].append({'inline_data': {'mime_type': 'image/png', 'data': img_data}})

        data['contents'].append(cur)
        self.add_history('gemini', prompt, img_data, True)
        return url, headers, data

    def groq(self, model, prompt, key):
        key = key or 'APIKEY'
        url = 'https://api.groq.com/openai/v1/chat/completions'
        headers = {'Content-Type': 'application/json', 'Authorization': f'Bearer {key}'}
        data = {'messages': [], 'model': model, 'stream': False}
        img_data = None

        if self.is_hist:
            for h in self.history:
                if h['role'] == 'user':
                    data['messages'].append({'role': 'user', 'content': [{'type': 'text', 'text': h['text']}]})

                    if 'image' in h:
                        data['messages'][-1]['content'].append(
                            {'type': 'image_url', 'image_url': {'url': f'data:image/png;base64,{h['image']}'}}
                        )
                else:
                    data['messages'].append({'role': 'assistant', 'content': h['text']})

        cur = {'role': 'user', 'content': [{'type': 'text', 'text': prompt}]}

        if self.is_img:
            with open(self.main_window.window_info.path + '/res/img/screenshot.png', 'rb') as img_file:
                img_data = base64.b64encode(img_file.read()).decode('utf-8')
                cur['content'].append({'type': 'image_url', 'image_url': {'url': f'data:image/png;base64,{img_data}'}})

        data['messages'].append(cur)
        self.add_history('groq', prompt, img_data, True)
        return url, headers, data


class OllamaThread(QThread):
    output_signal = Signal(str)
    finished_signal = Signal(str)
    error_signal = Signal(str)

    def __init__(self, model, prompt, image_path=None, parent=None):
        super().__init__(parent)
        self.model = model
        self.prompt = prompt
        self.image_path = image_path
        self.client = ollama.Client(host='http://localhost:11434')

    def run(self):
        try:
            messages = {
                'role': 'user',
                'content': self.prompt,
            }

            if self.image_path:
                messages['images'] = [self.image_path]

            response = self.client.chat(model=self.model, stream=True, messages=[messages])

            text = ''
            for chunk in response:
                content = chunk['message']['content']
                text += content
                self.output_signal.emit(text)

            self.finished_signal.emit(text)

        except Exception as e:
            self.error_signal.emit(str(e))
