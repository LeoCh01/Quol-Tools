from PySide6.QtCore import Signal
from PySide6.QtWidgets import QPushButton

from lib.quol_window import QuolMainWindow
from lib.window_loader import WindowInfo, WindowContext
from lib.t import StopwatchWindow


class MainWindow(QuolMainWindow):
    signal = Signal()

    def __init__(self, window_info: WindowInfo, window_context: WindowContext):
        super().__init__('Timer', window_info, window_context, default_geometry=(570, 180, 150, 1))

        self.timerWindow = StopwatchWindow((self.x(), self.y(), 1, 1), self.window_info.path + '/res')
        self.signal.connect(self.timerWindow.toggle_stopwatch)

        self.toggle_btn = QPushButton()
        self.toggle_btn.clicked.connect(lambda: self.on_toggle())
        self.layout.addWidget(self.toggle_btn)

        self.input_id = None
        self.on_toggle()

    def on_key_press(self, key):
        if key == self.config['toggle_key']:
            self.signal.emit()

    def on_update_config(self):
        if self.input_id:
            self.window_context.input_manager.remove_key_press_listener(self.input_id)
            self.input_id = None
        if self.toggle_btn.text() == 'On':
            self.input_id = self.window_context.input_manager.add_key_press_listener(self.on_key_press, suppressed=(self.config['toggle_key'], ))

    def on_toggle(self):
        if self.toggle_btn.text() == 'Off':
            self.timerWindow.show()
            self.toggle_btn.setText('On')
            self.toggle_btn.setStyleSheet('background-color: #4CAF50;')
            self.input_id = self.window_context.input_manager.add_key_press_listener(self.on_key_press, suppressed=(self.config['toggle_key'],))
        else:
            self.timerWindow.reset_stopwatch()
            self.timerWindow.hide()
            self.toggle_btn.setText('Off')
            self.toggle_btn.setStyleSheet('background-color: #f44336;')
            if self.input_id:
                self.window_context.input_manager.remove_key_press_listener(self.input_id)
                self.input_id = None

