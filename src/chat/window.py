import re
from PySide6.QtGui import QGuiApplication, QIcon
from PySide6.QtWidgets import QPushButton, QComboBox, QLineEdit, QHBoxLayout

from qlib.windows.quol_window import QuolMainWindow
from qlib.windows.tool_loader import ToolSpec
from lib.chat_window import ChatWindow
from lib.ai import AI
from lib.snip_overlay import SnipOverlay


class MainWindow(QuolMainWindow):
    def __init__(self, tool_spec: ToolSpec):
        super().__init__('Chat', tool_spec, default_geometry=(10, 930, 500, 1))
        self.setGeometry(10, QGuiApplication.primaryScreen().geometry().height() - 140, 500, 1)

        with open(tool_spec.path + '/test_response.txt', 'r') as f:
            self.test_response = f.read().strip()

        self.chat_window = ChatWindow(self)
        self.tool_spec.toggle_signal.connect(self.chat_window.toggle_tool)
        self.tool_spec.toggle_instant_signal.connect(self.chat_window.toggle_tool_instant)

        self.ai = AI(self, self.chat_window)
        self.ai_list = QComboBox()
        self.ai_list.addItems(['groq', 'gemini', 'ollama'])

        self.ai_list_cycle_icon = QIcon(self.tool_spec.path + "/res/img/cycle.svg")
        self.ai_list_cycle_btn = QPushButton(self)
        self.ai_list_cycle_btn.setIcon(self.ai_list_cycle_icon)

        self.prompt = QLineEdit()
        self.prompt.setPlaceholderText('Prompt for groq...')

        self.img_icon = QIcon(self.tool_spec.path + "/res/img/img.svg")
        self.img_btn = QPushButton(self)
        self.img_btn.setIcon(self.img_icon)
        self.img_btn.setCheckable(True)
        self.img_btn.setChecked(True)
        self.img_btn.setStyleSheet("background-color: #4CAF50;")
        self.img_btn.clicked.connect(self.on_image)

        self.snip_icon = QIcon(self.tool_spec.path + "/res/img/snip.svg")
        self.snip_btn = QPushButton(self)
        self.snip_btn.setIcon(self.snip_icon)
        self.snip_btn.setStyleSheet("padding-left: 5px; padding-right: 5px;")
        self.snip_overlay = None

        self.prompt_layout = QHBoxLayout()

        self.prompt_layout.addWidget(self.ai_list_cycle_btn)
        self.prompt_layout.addWidget(self.prompt)
        self.prompt_layout.addWidget(self.img_btn)
        self.prompt_layout.addWidget(self.snip_btn)
        self.layout.addLayout(self.prompt_layout)

        self.ai_list_cycle_btn.clicked.connect(self.on_cycle)
        self.snip_btn.clicked.connect(self.on_snip)
        self.prompt.returnPressed.connect(self.send_prompt)

    def on_update_config(self):
        self.chat_window.close()

    def on_cycle(self):
        current_index = self.ai_list.currentIndex()
        next_index = (current_index + 1) % self.ai_list.count()
        self.ai_list.setCurrentIndex(next_index)
        self.prompt.setPlaceholderText(f'Prompt for {self.ai_list.currentText()}...')

    def on_image(self):
        if self.img_btn.isChecked():
            self.img_btn.setStyleSheet("background-color: #4CAF50;")
        else:
            self.img_btn.setStyleSheet("background-color: #F44336;")

    def on_snip(self):
        screen = QGuiApplication.primaryScreen()
        if not screen:
            return

        self.tool_spec.toggle_instant_signal.emit(False)
        screenshot = screen.grabWindow(0)
        self.tool_spec.toggle_instant_signal.emit(True)

        self.snip_overlay = SnipOverlay(screenshot, self.on_snip_selected)
        self.snip_overlay.showFullScreen()
        self.snip_overlay.raise_()
        self.snip_overlay.activateWindow()

    def on_snip_selected(self, cropped):
        if cropped.isNull():
            return

        cropped.toImage().save(self.tool_spec.path + '/res/img/screenshot.png')

        self.img_btn.setChecked(True)
        self.on_image()

        if not self.prompt.text().strip():
            self.prompt.setText(self.config['config']['snip'])

        self.send_prompt(use_existing_image=True)

    def send_prompt(self, use_existing_image=False):
        self.ai.is_img = self.img_btn.isChecked()
        self.ai.is_hist = self.config['config']['history']

        if self.img_btn.isChecked() and not use_existing_image:
            screen = QGuiApplication.primaryScreen()
            self.tool_spec.toggle_instant_signal.emit(False)
            screenshot = screen.grabWindow(0).toImage()
            self.tool_spec.toggle_instant_signal.emit(True)
            screenshot.save(self.tool_spec.path + '/res/img/screenshot.png')

        self.set_button_loading_state(True)
        self.start_chat()

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
            self.snip_btn.setEnabled(False)
        else:
            self.snip_btn.setEnabled(True)

    def closeEvent(self, event):
        self.chat_window.close()
        # self.gpt.close()
        super().closeEvent(event)
