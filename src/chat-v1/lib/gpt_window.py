from PySide6.QtCore import QThread, Signal, QTimer
from PySide6.QtGui import QGuiApplication
from PySide6.QtWidgets import QHBoxLayout, QTextBrowser, QApplication

from lib.quol_window import QuolSubWindow
from lib.window_loader import WindowInfo, WindowContext
from simulator import Simulator

test_response = ['']


class GPTWindow(QuolSubWindow):
    def __init__(self, main_window):
        super().__init__(main_window, 'GPT')
        self.setGeometry(1200, 410, 500, 600)
        self.window_info: WindowInfo = main_window.window_info
        self.window_context: WindowContext = main_window.window_context

        self.output_box = QTextBrowser(self)
        self.output_box.setOpenExternalLinks(True)
        self.output_box.setReadOnly(True)
        self.layout.addWidget(self.output_box)

        with open(self.window_info.path + '/res/gptstyles.css') as f:
            self.style_tag = '<style>' + f.read() + '</style>'

        self.l2 = QHBoxLayout()
        self.layout.addLayout(self.l2)

        self.simulator = Simulator()
        self.simulate_thread = None
        self.history = []
        self.set_output()
        self.loading_counter = 0

        with open(self.window_info.path + '/test_response.txt', 'r') as f:
            test_response[0] = f.read()

        if test_response[0]:
            self.set_output(test_response[0])

    def on_send(self, input_field, toggle_image_button):
        prompt = input_field.text()
        if not prompt.strip() or not self.simulator.is_loaded:
            return

        input_field.clear()
        self.show()
        self.loading_counter = 0

        img_path = None
        if toggle_image_button.isChecked():
            screen = QGuiApplication.primaryScreen()
            self.window_context.toggle_windows_instant(True)
            screenshot = screen.grabWindow(0).toImage()
            self.window_context.toggle_windows_instant(False)
            img_path = self.window_info.path + '/res/img/screenshot.png'
            screenshot.save(img_path)

        def on_loading():
            self.loading_counter += 0.1
            self.set_output(f'<p>Loading... ({self.loading_counter:.1f}s)</p>')

        timer = QTimer()
        timer.timeout.connect(on_loading)
        timer.start(100)

        self.history.append(('user', '<p>' + prompt + '</p>'))
        self.simulate_thread = SimulateThread(self.simulator, prompt, img_path=img_path)
        self.simulate_thread.response_signal.connect(lambda response: self.set_output(response))
        self.simulate_thread.loaded_signal.connect(lambda: timer.stop())
        self.simulate_thread.finished_signal.connect(lambda response: self.history.append(('ai', response)))
        self.simulate_thread.start()

    def on_clear(self):
        self.history = []
        self.set_output()
        if self.simulate_thread and self.simulate_thread.isRunning():
            self.simulate_thread.requestInterruption()
            self.simulate_thread.wait()
        self.simulator.refresh()

    def on_image(self, toggle_image_button):
        if toggle_image_button.isChecked():
            toggle_image_button.setStyleSheet("padding: 5px; background-color: #4CAF50;")
        else:
            toggle_image_button.setStyleSheet("padding: 5px; background-color: #F44336;")

    def reload(self, reload_button, clear_btn, send_btn):
        self.show()
        reload_button.setEnabled(False)
        reload_button.setText("Loading...")
        send_btn.setEnabled(False)
        clear_btn.setEnabled(False)

        def on_finished():
            reload_button.setEnabled(True)
            reload_button.setText("Reload")
            send_btn.setEnabled(True)
            clear_btn.setEnabled(True)
            self.set_output('<p>GPT loaded successfully.</p>')

        self.simulate_thread = ReloadThread(self.simulator, self.main_window.config['gpt']['headless'])
        self.simulate_thread.finished_signal.connect(on_finished)
        self.simulate_thread.start()

    def set_output(self, text=''):
        data = ''.join(
            f'''
                <table width="100%">
                  <tr>
                    <td align="{'left' if s == 'ai' else 'right'}" class="{'ai-block' if s == 'ai' else 'user-block'}">{content}</td>
                  </tr>
                </table>
            ''' for s, content in self.history + [['ai', text]]
        )

        scrollbar = self.output_box.verticalScrollBar()
        scroll_pos = scrollbar.value()

        formatted_text = f'{self.style_tag}<body>{data}</body>'
        self.output_box.setHtml(formatted_text)

        scrollbar.setValue(scroll_pos)
        self.output_box.repaint()
        QApplication.processEvents()

    def closeEvent(self, event):
        self.hide()
        self.output_box.clear()
        if self.simulate_thread and self.simulate_thread.isRunning():
            self.simulate_thread.requestInterruption()
            self.simulate_thread.wait()
        self.simulator.close()
        super().close()


class ReloadThread(QThread):
    finished_signal = Signal()

    def __init__(self, ai, headless):
        super().__init__()
        self.simulator = ai
        self.headless = headless

    def run(self):
        self.simulator.reload('chatgpt', headless=self.headless)
        self.finished_signal.emit()


class SimulateThread(QThread):
    response_signal = Signal(str)
    loaded_signal = Signal()
    finished_signal = Signal(str)

    def __init__(self, ai, input_text, img_path=None):
        super().__init__()
        self.simulator = ai
        self.input_text = input_text
        self.img_path = img_path

    def run(self):
        res = ''
        is_loaded = False
        for response in self.simulator.submit(self.input_text, img_path=self.img_path):
            if not is_loaded:
                self.loaded_signal.emit()
                is_loaded = True
            res = response
            self.response_signal.emit(res)
        self.finished_signal.emit(res)
