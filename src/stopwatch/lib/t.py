import time

from PySide6.QtCore import Qt, QTimer, QSize
from PySide6.QtGui import QIcon
from PySide6.QtWidgets import QWidget, QLabel, QVBoxLayout, QHBoxLayout, QPushButton


class StopwatchWindow(QWidget):
    def __init__(self, geometry=(100, 100, 1, 1), res_path=''):
        super().__init__()
        self.setGeometry(*geometry)
        self.res_path = res_path
        self.setWindowFlag(Qt.WindowType.FramelessWindowHint | Qt.WindowType.WindowStaysOnTopHint | Qt.WindowType.Tool)
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)

        self.start_time = None
        self.elapsed_time: float = 0
        self.running = False
        self.offset = None
        self.is_lap_mode = False
        self.laps = []

        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_time)

        self.setup_ui()
        self.set_mouse_tracking()

    def setup_ui(self):
        outer_layout = QVBoxLayout(self)

        self.container = QWidget(self)
        self.container.setStyleSheet("background-color: rgba(0, 0, 0, 60); border-radius: 15px;")
        inner_layout = QHBoxLayout(self.container)

        self.label = QLabel("00:00:00", self)
        self.label.setStyleSheet("color: white; font-size: 24px; background: transparent;")
        inner_layout.addWidget(self.label)

        # Play/Stop Button
        self.play_icon = QIcon(self.res_path + "/play-solid-full.svg")
        self.pause_icon = QIcon(self.res_path + "/pause-solid-full.svg")
        self.play_stop_btn = QPushButton(self)
        self.play_stop_btn.setIcon(self.play_icon)
        self.play_stop_btn.setIconSize(QSize(24, 24))
        self.play_stop_btn.setStyleSheet("background-color: transparent; color: white;")
        self.play_stop_btn.clicked.connect(self.toggle_stopwatch)
        inner_layout.addWidget(self.play_stop_btn)

        # Reset Button
        self.reset_icon = QIcon(self.res_path + "/rotate-right-solid-full.svg")
        self.reset_btn = QPushButton(self)
        self.reset_btn.setIcon(self.reset_icon)
        self.reset_btn.setIconSize(QSize(24, 24))
        self.reset_btn.setStyleSheet("background-color: transparent;")
        self.reset_btn.clicked.connect(self.reset_stopwatch)
        inner_layout.addWidget(self.reset_btn)

        # Toggle Stopwatch/Lap Mode Button
        self.stop_watch_icon = QIcon(self.res_path + "/clock.svg")
        self.lap_mode_icon = QIcon(self.res_path + "/flag.svg")
        self.icon_btn = QPushButton(self)
        self.icon_btn.setIconSize(QSize(24, 24))
        self.icon_btn.setIcon(self.stop_watch_icon)
        self.icon_btn.setStyleSheet("background-color: transparent;")
        self.icon_btn.clicked.connect(self.toggle_type)
        inner_layout.addWidget(self.icon_btn)

        outer_layout.addWidget(self.container)

        self.lap_list = QVBoxLayout()
        outer_layout.addLayout(self.lap_list)

    def mousePressEvent(self, event):
        self.offset = event.position().toPoint()

    def mouseMoveEvent(self, event):
        if event.buttons() == Qt.MouseButton.LeftButton:
            self.move(self.pos() + event.position().toPoint() - self.offset)

    def set_mouse_tracking(self):
        self.setMouseTracking(True)

    def clear_laps(self):
        while self.lap_list.count():
            child = self.lap_list.takeAt(0)
            if child.widget():
                child.widget().deleteLater()
        self.laps.clear()
        QTimer.singleShot(0, lambda: self.adjustSize())

    def toggle_type(self):
        self.reset_stopwatch()
        self.is_lap_mode = not self.is_lap_mode
        if self.is_lap_mode:
            self.icon_btn.setIcon(self.lap_mode_icon)
        else:
            self.icon_btn.setIcon(self.stop_watch_icon)
            self.clear_laps()

    def toggle_stopwatch(self):
        if not self.running:
            self.running = True
            self.start_time = time.time() - self.elapsed_time
            self.timer.start(10)
            self.play_stop_btn.setIcon(self.pause_icon)
            print("Stopwatch started")
        else:
            if self.is_lap_mode:
                lap_time = self.label.text()
                self.laps.append(lap_time)
                lap_label = QLabel(lap_time)
                lap_label.setStyleSheet("color: white; background-color: rgba(0, 0, 0, 30); border-radius: 10px; padding: 5px;")
                self.lap_list.addWidget(lap_label)
                return

            self.running = False
            self.elapsed_time = time.time() - self.start_time
            self.timer.stop()
            self.play_stop_btn.setIcon(self.play_icon)

    def reset_stopwatch(self):
        self.running = False
        self.elapsed_time = 0
        self.start_time = None
        self.timer.stop()
        self.label.setText("00:00:00")
        self.play_stop_btn.setIcon(self.play_icon)

    def update_time(self):
        elapsed = time.time() - self.start_time
        minutes = int(elapsed // 60)
        seconds = int(elapsed % 60)
        milliseconds = int((elapsed * 100) % 100)
        self.label.setText(f"{minutes:02}:{seconds:02}:{milliseconds:02}")

    def mouseDoubleClickEvent(self, event):
        self.toggle_stopwatch()
