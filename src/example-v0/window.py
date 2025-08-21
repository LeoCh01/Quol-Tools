from PySide6.QtWidgets import QLabel

from lib.quol_window import QuolMainWindow
from lib.window_loader import WindowInfo, WindowContext
from lib.adder import add_to_str


class MainWindow(QuolMainWindow):
    """
    MainWindow class that inherits from QuolMainWindow.

    This class represents a window with two QLabel widgets.
    """

    def __init__(self, window_info: WindowInfo, window_context: WindowContext):
        super().__init__('Temp', window_info, window_context, default_geometry=(300, 300, 100, 1))

        # self.layout is the main layout of the window
        self.layout.addWidget(QLabel('Hello'))

        # self.config is a dictionary containing data in config.json
        self.number = QLabel(add_to_str(self.config['a'], self.config['b']))
        self.layout.addWidget(self.number)

    def on_update_config(self):
        # This method is called when the config is updated.
        self.number.setText(add_to_str(self.config['a'], self.config['b']))
