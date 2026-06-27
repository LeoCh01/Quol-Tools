from PySide6.QtGui import QGuiApplication, QIcon
from PySide6.QtWidgets import QPushButton, QComboBox, QLineEdit, QHBoxLayout

from qlib.windows.quol_window import QuolMainWindow
from qlib.windows.tool_loader import ToolSpec


class MainWindow(QuolMainWindow):
    def __init__(self, tool_spec: ToolSpec):
        super().__init__('Music', tool_spec, default_geometry=(10, 930, 500, 1))