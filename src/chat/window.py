import re
from PySide6.QtCore import QTimer
from PySide6.QtGui import QGuiApplication, QIcon
from PySide6.QtWidgets import QPushButton, QComboBox, QLineEdit, QHBoxLayout

from lib.quol_window import QuolMainWindow
from lib.window_loader import WindowInfo, WindowContext
from lib.chat_window import ChatWindow
from lib.ai import AI
from lib.gpt_window import GPTWindow


class MainWindow(QuolMainWindow):
    def __init__(self, window_info: WindowInfo, window_context: WindowContext):
        super().__init__('Chat', window_info, window_context, default_geometry=(10, 930, 500, 1))
        self.setGeometry(10, QGuiApplication.primaryScreen().geometry().height() - 140, 500, 1)

        with open(window_info.path + '/test_response.txt', 'r') as f:
            self.test_response = f.read().strip()

        self.chat_window = ChatWindow(self)
        self.window_context.toggle.connect(self.chat_window.toggle_windows)

        # self.gpt = GPTWindow(self)
        # self.window_context.toggle.connect(self.gpt.toggle_windows)
        self.window_context.toggle.connect(self.focus)

        self.ai = AI(self, self.chat_window)
        self.ai_list = QComboBox()
        self.ai_list.addItems(['gemini', 'groq', 'ollama'])

        self.ai_list_cycle_icon = QIcon(self.window_info.path + "/res/img/cycle.svg")
        self.ai_list_cycle_btn = QPushButton(self)
        self.ai_list_cycle_btn.setIcon(self.ai_list_cycle_icon)

        # self.reload_btn_gpt = QPushButton('init')

        self.clear_icon = QIcon(self.window_info.path + "/res/img/clear.svg")
        self.clear_btn = QPushButton(self)
        self.clear_btn.setIcon(self.clear_icon)

        self.prompt = QLineEdit()
        self.prompt.setPlaceholderText('Prompt for gemini...')

        self.img_icon = QIcon(self.window_info.path + "/res/img/img.svg")
        self.img_btn = QPushButton(self)
        self.img_btn.setIcon(self.img_icon)
        self.img_btn.setCheckable(True)
        self.img_btn.setChecked(True)
        self.img_btn.setStyleSheet("background-color: #4CAF50;")
        self.img_btn.clicked.connect(self.on_image)

        self.send_icon = QIcon(self.window_info.path + "/res/img/send.svg")
        self.send_btn = QPushButton(self)
        self.send_btn.setIcon(self.send_icon)
        self.send_btn.setStyleSheet("padding-left: 5px; padding-right: 5px;")

        self.prompt_layout = QHBoxLayout()

        self.prompt_layout.addWidget(self.ai_list_cycle_btn)
        self.prompt_layout.addWidget(self.clear_btn)
        self.prompt_layout.addWidget(self.prompt)
        self.prompt_layout.addWidget(self.img_btn)
        self.prompt_layout.addWidget(self.send_btn)
        self.layout.addLayout(self.prompt_layout)

        # self.reload_btn_gpt.clicked.connect(lambda: self.gpt.reload(self.reload_btn_gpt, self.clear_btn, self.send_btn))
        self.ai_list_cycle_btn.clicked.connect(self.on_cycle)
        self.clear_btn.clicked.connect(self.on_clear)
        self.prompt.returnPressed.connect(self.send_prompt)
        self.send_btn.clicked.connect(self.send_prompt)
        # self.connect_signals()

    def on_update_config(self):
        self.clear_btn.clicked.disconnect()
        self.prompt.returnPressed.disconnect()
        self.send_btn.clicked.disconnect()
        # self.gpt.close()
        self.chat_window.close()

        # self.connect_signals()

    def connect_signals(self):
        self.swap_widgets()
        if self.config['gpt']['mode']:
            self.clear_btn.clicked.connect(self.gpt.on_clear)
            self.prompt.returnPressed.connect(lambda: self.gpt.on_send(self.prompt, self.img_btn))
            self.send_btn.clicked.connect(lambda: self.gpt.on_send(self.prompt, self.img_btn))

            self.ai_list.setDisabled(True)
        else:
            self.clear_btn.clicked.connect(self.on_clear)
            self.prompt.returnPressed.connect(self.send_prompt)
            self.send_btn.clicked.connect(self.send_prompt)
            self.ai_list.setDisabled(False)

    def swap_widgets(self):
        if self.config['gpt']['mode']:
            self.top_layout.replaceWidget(self.ai_list, self.reload_btn_gpt)
            self.reload_btn_gpt.show()
            self.ai_list.hide()
        else:
            self.top_layout.replaceWidget(self.reload_btn_gpt, self.ai_list)
            self.ai_list.show()
            self.reload_btn_gpt.hide()

    def on_cycle(self):
        current_index = self.ai_list.currentIndex()
        next_index = (current_index + 1) % self.ai_list.count()
        self.ai_list.setCurrentIndex(next_index)
        self.prompt.setPlaceholderText(f'Prompt for {self.ai_list.currentText()}...')

    def on_clear(self):
        self.chat_window.chat_response.clear()
        self.ai.history.clear()

    def on_image(self):
        if self.img_btn.isChecked():
            self.img_btn.setStyleSheet("background-color: #4CAF50;")
        else:
            self.img_btn.setStyleSheet("background-color: #F44336;")

    def focus(self):
        if self.config['config']['auto_focus'] and not self.window_context.get_is_hidden():
            self.raise_()
            self.activateWindow()
            self.prompt.setFocus()
        elif self.config['config']['auto_focus']:
            self.prompt.clearFocus()

    def send_prompt(self):
        self.ai.is_img = self.img_btn.isChecked()
        self.ai.is_hist = self.config['config']['history']

        if self.img_btn.isChecked():
            screen = QGuiApplication.primaryScreen()
            self.window_context.toggle_windows_instant(True)
            screenshot = screen.grabWindow(0).toImage()
            self.window_context.toggle_windows_instant(False)
            screenshot.save(self.window_info.path + '/res/img/screenshot.png')

        self.set_button_loading_state(True)
        QTimer.singleShot(0, self.start_chat)

    def start_chat(self):
        s = self.prompt.text().split()
        t = self.prompt.text()

        if s and s[0] in self.config['commands']:
            t = self.config['commands'][s[0]]
            for i in range(1, len(s)):
                t = re.sub(r'\{' + str(i - 1) + r'(?::[^}]*)?}', s[i], t)
            t = re.sub(r'\{(\d+):([^}]*)}', r'\2', t)
            t = re.sub(r'\{\d+}', '', t)

        print('Question:', t)

        if self.ai_list.currentText() == 'ollama':
            self.ai.prompt('ollama', {'prompt': t, 'model': self.config['ollama']['model']})
        else:
            data = {
                'prompt': t,
                'model': self.config[self.ai_list.currentText()]['model'],
                'apikey': self.config[self.ai_list.currentText()]['apikey']
            }
            self.ai.prompt(self.ai_list.currentText(), data)

        self.prompt.setText('')

    def set_button_loading_state(self, is_loading):
        if is_loading:
            self.send_btn.setEnabled(False)
        else:
            self.send_btn.setEnabled(True)

    def closeEvent(self, event):
        self.chat_window.close()
        # self.gpt.close()
        super().close()
