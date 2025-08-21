import subprocess
from PySide6.QtWidgets import QPushButton, QVBoxLayout, QLineEdit, QHBoxLayout, QGroupBox, QLabel, QPlainTextEdit, \
    QCheckBox

from lib.io_helpers import read_json, write_json
from lib.quol_window import QuolMainWindow, QuolSubWindow
from lib.window_loader import WindowInfo, WindowContext


class MainWindow(QuolMainWindow):
    def __init__(self, window_info: WindowInfo, window_context: WindowContext):
        super().__init__('Command', window_info, window_context, default_geometry=(390, 10, 170, 1), show_config=False)

        self.commands_groupbox = QGroupBox('Commands')
        self.commands_layout = QVBoxLayout()
        self.commands_layout.setContentsMargins(5, 5, 5, 5)
        self.commands_groupbox.setLayout(self.commands_layout)
        self.layout.addWidget(self.commands_groupbox)

        self.add_btn = QPushButton('Add Command')
        self.add_btn.clicked.connect(self.open_add_command_dialog)
        self.layout.addWidget(self.add_btn)

        self.commands = []
        self.commands_path = self.window_info.path + '/res/commands.json'
        self.load_commands()
        self.dialog = CommandConfig(self)

    def open_add_command_dialog(self):
        self.dialog.show()

    def add_command_to_layout(self, cmd_name, cmd, show_output, init=False):
        cmd_layout = QHBoxLayout()
        cmd_btn = QPushButton(cmd_name)
        cmd_btn.clicked.connect(lambda _, c=cmd, s=show_output: self.run_cmd(c, s))
        cmd_layout.addWidget(cmd_btn)

        delete_btn = QPushButton('✖')
        delete_btn.setFixedWidth(20)
        delete_btn.clicked.connect(lambda _, c=cmd_name, l=cmd_layout: self.delete_command(c, l))
        cmd_layout.addWidget(delete_btn)

        if not init:
            self.setFixedHeight(self.height() + 29)
        self.commands_layout.addLayout(cmd_layout)

    def delete_command(self, cmd_name, layout):
        self.commands = [c for c in self.commands if c[0] != cmd_name]
        for i in reversed(range(layout.count())):
            widget = layout.itemAt(i).widget()
            if widget is not None:
                widget.deleteLater()

        self.commands_layout.removeItem(layout)
        self.save_commands()
        self.setFixedHeight(self.height() - 29)

    def run_cmd(self, cmd, show_output):
        try:
            result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
            if show_output:
                self.show_command_output(result.stdout)
        except Exception as e:
            print(f'An error occurred: {e}')

    def show_command_output(self, res):
        output_window = QuolSubWindow(self, 'Command Output')
        output_window.setGeometry(100, 100, 600, 400)

        output_text = QPlainTextEdit(output_window)
        output_text.setPlainText(res)
        output_text.setReadOnly(True)

        output_window.layout.addWidget(output_text)
        output_window.show()

    def save_commands(self):
        write_json(self.commands_path, self.commands)

    def load_commands(self):
        try:
            self.commands = read_json(self.commands_path)
        except Exception as e:
            print('error :: ', e)
            self.commands = []

        for cmd_name, cmd, show_output in self.commands:
            self.add_command_to_layout(cmd_name, cmd, show_output, init=True)


class CommandConfig(QuolSubWindow):
    def __init__(self, main_window: MainWindow):
        super().__init__(main_window, 'Add Command')

        screen_geometry = self.screen().geometry()
        self.setGeometry(
            screen_geometry.width() // 2 - 300,
            screen_geometry.height() // 2 - 200,
            600,
            400
        )

        self.command_name_input = QLineEdit(self)
        self.command_name_input.setPlaceholderText('Enter command name...')

        self.command_input = QPlainTextEdit(self)
        self.command_input.setPlaceholderText(
            'Add terminal command...\n\n'
            'Example 1: open webpage (start https://www.google.com)\n\n'
            'Example 2: show IP address (ipconfig)\n\n'
            'Example 3: concatenate commands (start https://www.google.com && ipconfig)\n\n'
        )

        self.show_output_checkbox = QCheckBox('Show Output in Terminal', self)

        self.save_btn = QPushButton('Save Command', self)
        self.save_btn.clicked.connect(self.on_save_clicked)

        self.layout.addWidget(QLabel('Command Name:'))
        self.layout.addWidget(self.command_name_input)
        self.layout.addWidget(QLabel('Command:'))
        self.layout.addWidget(self.command_input)
        self.layout.addWidget(self.show_output_checkbox)
        self.layout.addWidget(self.save_btn)

    def on_save_clicked(self):
        cmd_name = self.command_name_input.text()
        cmd = self.command_input.toPlainText()
        show_output = self.show_output_checkbox.isChecked()

        if cmd_name and cmd:
            self.main_window.add_command_to_layout(cmd_name, cmd, show_output)
            self.main_window.commands.append((cmd_name, cmd, show_output))
            self.main_window.save_commands()

            self.command_name_input.clear()
            self.command_input.clear()
            self.show_output_checkbox.setChecked(False)

        self.close()
