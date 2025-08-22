from PySide6.QtWidgets import QLabel
from pynput.keyboard import Listener

from lib.quol_window import QuolMainWindow
from lib.window_loader import WindowInfo, WindowContext
from lib.adder import add_to_str


class MainWindow(QuolMainWindow):
    """
    MainWindow class that inherits from QuolMainWindow.

    This class represents a window displaying addition and keyboard listener.
    """

    def __init__(self, window_info: WindowInfo, window_context: WindowContext):
        super().__init__('Temp', window_info, window_context, default_geometry=(300, 300, 100, 1))

        # self.layout is the main layout of the window
        self.layout.addWidget(QLabel('Hello'))

        # self.config is a dictionary containing data in config.json
        self.number = QLabel(add_to_str(self.config['a'], self.config['b']))
        self.layout.addWidget(self.number)

        self.key_label = QLabel("Last Key: None")
        self.layout.addWidget(self.key_label)
        self.start_key_listener()

    def on_update_config(self):
        # This method is called when the config is updated.
        self.number.setText(add_to_str(self.config['a'], self.config['b']))

    def start_key_listener(self):
        self.listener = Listener(on_press=self.on_key_press)
        self.listener.start()

    def on_key_press(self, key):
        try:
            self.key_label.setText(f"Last Key: {key.char}")
        except AttributeError:
            self.key_label.setText(f"Last Key: {key}")