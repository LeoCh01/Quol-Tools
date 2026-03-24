from PySide6.QtCore import QTimer, Signal
from PySide6.QtWidgets import (
    QHBoxLayout,
    QVBoxLayout,
    QPushButton,
    QLabel,
    QWidget,
    QFrame, QGridLayout,
)
from PySide6.QtGui import QGuiApplication, Qt

from qlib.windows.quol_window import QuolMainWindow
from qlib.windows.tool_loader import ToolSpec
from lib.triangulation import StrongholdTriangulator


class MainWindow(QuolMainWindow):
    f3c_signal = Signal()

    def __init__(self, tool_spec: ToolSpec):
        super().__init__('Stronghold', tool_spec, default_geometry=(1510, 580, 180, 1))

        self.triangulator = StrongholdTriangulator()
        self.hotkey_id = None

        self.f3c_signal.connect(self.on_f3c_pressed)

        row_layout = QHBoxLayout(self)
        row_layout.setContentsMargins(0, 0, 0, 0)
        row_layout.setSpacing(5)

        self.toggle_btn = QPushButton("Off")
        self.toggle_btn.setStyleSheet("background-color: #f44336;")

        self.clear_btn = QPushButton("Clear")

        row_layout.addWidget(self.toggle_btn)
        row_layout.addWidget(self.clear_btn)

        self.layout.addLayout(row_layout)

        self.results_box = QFrame()
        self.results_box.setStyleSheet("""
            QFrame {
                background-color: #2f2f2f;
                border-radius: 6px;
            }
        """)

        results_layout = QGridLayout(self.results_box)

        # Throws
        self.throws_label_title = QLabel("Throws:")
        self.throws_label_value = QLabel("0")

        # Overworld
        self.overworld_title = QLabel("OW (x, z):")
        self.overworld_value = QLabel("-")

        # Nether
        self.nether_title = QLabel("N (x, z):")
        self.nether_value = QLabel("-")

        # Error
        self.error_title = QLabel("Error:")
        self.error_value = QLabel("-")

        self.throws_label_value.setAlignment(Qt.AlignmentFlag.AlignRight)
        self.overworld_value.setAlignment(Qt.AlignmentFlag.AlignRight)
        self.nether_value.setAlignment(Qt.AlignmentFlag.AlignRight)
        self.error_value.setAlignment(Qt.AlignmentFlag.AlignRight)

        results_layout.addWidget(self.throws_label_title, 0, 0)
        results_layout.addWidget(self.throws_label_value, 0, 1)
        results_layout.addWidget(self.overworld_title, 1, 0)
        results_layout.addWidget(self.overworld_value, 1, 1)
        results_layout.addWidget(self.nether_title, 2, 0)
        results_layout.addWidget(self.nether_value, 2, 1)
        results_layout.addWidget(self.error_title, 3, 0)
        results_layout.addWidget(self.error_value, 3, 1)

        self.layout.addWidget(self.results_box)

        self.tp_cmd = QLabel("Use F3+C to add throws")
        self.layout.addWidget(self.tp_cmd)

        self.toggle_btn.clicked.connect(self.on_toggle)
        self.clear_btn.clicked.connect(self.clear_data)

    def on_toggle(self):
        if self.toggle_btn.text() == "Off":
            self.toggle_btn.setText("On")
            self.toggle_btn.setStyleSheet("background-color: #4CAF50;")

            self.hotkey_id = self.tool_spec.input_manager.add_hotkey(
                "f3+c",
                self.f3c_signal.emit
            )
        else:
            self.toggle_btn.setText("Off")
            self.toggle_btn.setStyleSheet("background-color: #f44336;")

            if self.hotkey_id:
                self.tool_spec.input_manager.remove_hotkey(self.hotkey_id)
                self.hotkey_id = None

    def on_f3c_pressed(self):
        QTimer.singleShot(100, self.process_clipboard)

    def process_clipboard(self):
        clipboard = QGuiApplication.clipboard()
        text = clipboard.text()

        if not text:
            return

        self.triangulator.insert_f3c_string(text)
        coords, accuracy = self.triangulator.calculate()

        throws_count = len(self.triangulator.throws)
        self.throws_label_value.setText(str(throws_count))

        if coords:
            x, z = coords
            x = round(x)
            z = round(z)
            nx = x // 8
            nz = z // 8

            self.overworld_value.setText(f"({x}, {z})")
            self.nether_value.setText(f"({nx}, {nz})")
            self.error_value.setText(f"{round(accuracy, 5)} blocks")
            self.tp_cmd.setText(f"/tp @p {x} ~ {z}")
        else:
            self.overworld_value.setText("-")
            self.nether_value.setText("-")
            self.error_value.setText("-")
            self.tp_cmd.setText("Use F3+C to add throws")

    def clear_data(self):
        self.triangulator.clear()

        self.throws_label_value.setText("0")
        self.overworld_value.setText("-")
        self.nether_value.setText("-")
        self.error_value.setText("-")
        self.tp_cmd.setText("Use F3+C to add throws")

    def closeEvent(self, event):
        if self.hotkey_id:
            self.tool_spec.input_manager.remove_hotkey(self.hotkey_id)
            self.hotkey_id = None

        super().closeEvent(event)