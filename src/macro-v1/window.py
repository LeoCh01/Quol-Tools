import os
import uuid

from PySide6.QtCore import QTimer
from PySide6.QtWidgets import (
    QVBoxLayout, QHBoxLayout, QPushButton, QWidget, QGroupBox
)
from lib.quol_window import QuolMainWindow
from lib.window_loader import WindowInfo, WindowContext
from lib.io_helpers import read_json, write_json
from lib.macro_dialog import MacroDialog


class MainWindow(QuolMainWindow):
    def __init__(self, window_info: WindowInfo, window_context: WindowContext):
        super().__init__('Macros', window_info, window_context, default_geometry=(300, 300, 160, 1), show_config=False)

        self.macros_path = window_info.path + '/res/macros.json'
        self.macros_dir = window_info.path + '/res/macros'
        os.makedirs(self.macros_dir, exist_ok=True)

        self.macro_groupbox = QGroupBox("Macros")
        self.macro_layout = QVBoxLayout()
        self.macro_groupbox.setLayout(self.macro_layout)
        self.macro_layout.setContentsMargins(0, 5, 0, 5)
        self.layout.addWidget(self.macro_groupbox)

        self.add_button = QPushButton("+ Add Macro")
        self.add_button.clicked.connect(lambda: self.add_macro_row())
        self.layout.addWidget(self.add_button)

        self.macro_rows: dict[str, dict] = {}
        self.macro_counter = 0

        QTimer.singleShot(0, self.load_macros)

    def add_macro_row(self, name="Unnamed", filename=None):
        self.setFixedHeight(self.height() + 25)
        macro_id = f"__macro_{self.macro_counter}"
        self.macro_counter += 1

        if not filename:
            filename = f"{uuid.uuid4().hex}.json"

        row_widget = QWidget()
        row_layout = QHBoxLayout(row_widget)
        row_layout.setContentsMargins(0, 0, 0, 0)
        row_layout.setSpacing(0)

        name_btn = QPushButton(name)
        name_btn.setFixedWidth(100)
        delete_btn = QPushButton("✖")
        delete_btn.setFixedWidth(20)

        row_layout.addWidget(name_btn)
        row_layout.addWidget(delete_btn)
        self.macro_layout.addWidget(row_widget)

        self.macro_rows[macro_id] = {
            "name": name,
            "filename": filename,
            "button": name_btn,
            "widget": row_widget
        }

        def delete_macro():
            self.remove_macro(macro_id)
            self.save_macros()

        def open_macro_dialog():
            dialog = MacroDialog(self, self.macro_rows[macro_id]['name'], self.macros_dir + '/' + filename)

            def on_accept():
                new_name = dialog.get_name()
                if new_name:
                    self.macro_rows[macro_id]['name'] = new_name
                    name_btn.setText(new_name)
                    self.save_macros()
                dialog.close()

            dialog.on_accept(on_accept)
            dialog.show()

        delete_btn.clicked.connect(delete_macro)
        name_btn.clicked.connect(open_macro_dialog)

    def remove_macro(self, macro_id):
        if macro_id not in self.macro_rows:
            return

        row = self.macro_rows[macro_id]
        row["widget"].setParent(None)
        row["widget"].deleteLater()
        del self.macro_rows[macro_id]
        self.setFixedHeight(self.height() - 25)

    def save_macros(self):
        data = {
            row["name"]: row["filename"]
            for row in self.macro_rows.values()
        }
        write_json(self.macros_path, data)

    def load_macros(self):
        if not os.path.exists(self.macros_path):
            return
        data = read_json(self.macros_path)
        for name, filename in data.items():
            self.add_macro_row(name, filename)
