import os
import uuid
from PySide6.QtCore import QTimer, Qt, Signal
from PySide6.QtWidgets import (
    QVBoxLayout, QHBoxLayout, QPushButton, QWidget, QGroupBox, QLabel, QLineEdit
)
from lib.quol_window import QuolMainWindow
from lib.window_loader import WindowInfo, WindowContext
from lib.io_helpers import read_json, write_json
from pynput.keyboard import Listener

from lib.recorder import record_macro
from lib.player import play_macro
from lib.popup import Popup

WM_KEYDOWN = 256
WM_KEYUP = 257


class MainWindow(QuolMainWindow):
    record_signal = Signal()
    stop_signal = Signal()
    start_recording_signal = Signal()
    stop_recording_signal = Signal()

    def __init__(self, window_info: WindowInfo, window_context: WindowContext):
        super().__init__('Macros', window_info, window_context, default_geometry=(300, 300, 180, 1))

        self.macros_path = window_info.path + '/res/macros.json'
        self.macros_dir = window_info.path + '/res/macros'
        os.makedirs(self.macros_dir, exist_ok=True)

        self.macro_groupbox = QGroupBox("Macros")
        self.macro_layout = QVBoxLayout()
        self.macro_groupbox.setLayout(self.macro_layout)
        self.macro_layout.setContentsMargins(0, 5, 0, 5)
        self.layout.addWidget(self.macro_groupbox)

        self.toggle_layout = QHBoxLayout()
        self.toggle_label = QLabel("Hotkeys:")
        self.toggle_label.setFixedWidth(50)
        self.toggle_layout.addWidget(self.toggle_label)
        self.toggle_btn = QPushButton()
        self.toggle_btn.clicked.connect(lambda: self.on_toggle())
        self.toggle_layout.addWidget(self.toggle_btn)
        self.layout.addLayout(self.toggle_layout)

        self.record_popup = Popup("Recording...")
        self.record_signal.connect(self.record_popup.play)

        self.stop_popup = Popup("Stopped")
        self.stop_signal.connect(self.stop_popup.play)

        self.start_recording_signal.connect(self.start_recording)
        self.stop_recording_signal.connect(self.stop_recording)

        self.recording = False
        self.current_macro_id = None
        self.macro_rows = {}
        self.listener = None
        self.on_toggle()

        QTimer.singleShot(0, self.load_macros)

    def on_toggle(self):
        if self.toggle_btn.text() == "On":
            self.toggle_btn.setText("Off")
            self.toggle_btn.setStyleSheet("background-color: #f44336;")
            if self.listener:
                self.listener.stop()
                self.listener = None
        else:
            self.toggle_btn.setText("On")
            self.toggle_btn.setStyleSheet("background-color: #4CAF50;")
            self.listener = Listener(win32_event_filter=self.win32_event_filter)
            self.listener.start()

    def win32_event_filter(self, msg, data):
        if msg == WM_KEYDOWN and data.vkCode == 114 and not self.recording:
            self.start_recording_signal.emit()

            self.listener.suppress_event()

    def start_recording(self):
        # self.record_signal.emit()  # not working
        self.recording = True
        self.current_macro_id = f"__macro_{uuid.uuid4().hex}"
        print("Recording macro:", self.current_macro_id)

        record_macro(f'{self.macros_dir}/{self.current_macro_id}.json', self.stop_recording_signal.emit, self.config['stop_key'])

    def stop_recording(self):
        self.stop_signal.emit()
        self.recording = False

        print("Stopped recording macro:", self.current_macro_id)
        self.add_macro_row(f'{self.current_macro_id[-4:]}', self.current_macro_id)
        self.save_macros()

    def add_macro_row(self, name, macro_id):
        row_widget = QWidget()
        row_layout = QHBoxLayout(row_widget)
        row_layout.setContentsMargins(0, 0, 0, 0)
        row_layout.setSpacing(0)

        name_label = QLabel(name)
        name_label.setFixedWidth(40)

        repeats_input = QLineEdit("1")
        repeats_input.setPlaceholderText("repeat")
        repeats_input.setFixedWidth(50)
        repeats_input.setAlignment(Qt.AlignmentFlag.AlignCenter)

        play_btn = QPushButton("▶")
        play_btn.setFixedWidth(20)

        delete_btn = QPushButton("✖")
        delete_btn.setFixedWidth(20)

        row_layout.addWidget(name_label)
        row_layout.addWidget(repeats_input)
        row_layout.addWidget(play_btn)
        row_layout.addWidget(delete_btn)

        self.macro_layout.addWidget(row_widget)

        self.macro_rows[macro_id] = {
            "name": name,
            "play_button": play_btn,
            "widget": row_widget,
            "repeats_input": repeats_input
        }

        play_btn.clicked.connect(lambda: self.on_play_clicked(macro_id))
        delete_btn.clicked.connect(lambda: self.remove_macro(macro_id))

    def on_play_clicked(self, macro_id):
        play_macro(f'{self.macros_dir}/{macro_id}.json', rep=self.get_rep(macro_id))

    def get_rep(self, macro_id):
        row = self.macro_rows[macro_id]
        try:
            rep = int(row["repeats_input"].text())
            return max(1, rep)
        except ValueError:
            return 1

    def remove_macro(self, macro_id):
        if macro_id not in self.macro_rows:
            return

        row = self.macro_rows[macro_id]
        row["widget"].setParent(None)
        row["widget"].deleteLater()
        del self.macro_rows[macro_id]

        self.save_macros()
        self.macro_layout.removeWidget(row["widget"])

    def save_macros(self):
        data = {row["name"]: macro_id for macro_id, row in self.macro_rows.items()}
        write_json(self.macros_path, data)

    def load_macros(self):
        if not os.path.exists(self.macros_path):
            return
        data = read_json(self.macros_path)
        for name, macro_id in data.items():
            self.add_macro_row(name, macro_id)

    def closeEvent(self, event):
        if self.listener:
            self.listener.stop()
            self.listener = None
        super().closeEvent(event)
