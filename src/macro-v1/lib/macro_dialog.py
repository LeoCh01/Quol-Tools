from PySide6.QtWidgets import QPushButton, QLineEdit
from lib.quol_window import QuolDialogWindow
from recorder import record_macro
from player import play_macro


class MacroDialog(QuolDialogWindow):
    def __init__(self, parent, macro_name: str, macro_path: str):
        super().__init__(parent, "Edit Macro")
        self.setGeometry(400, 400, 200, 150)
        self.macro_path = macro_path

        self.name_input = QLineEdit(macro_name)
        self.name_input.setPlaceholderText("Macro name")
        self.layout.addWidget(self.name_input)

        self.record_btn = QPushButton("Record")
        self.play_btn = QPushButton("Play")

        self.layout.addWidget(self.record_btn)
        self.layout.addWidget(self.play_btn)

        self.record_btn.clicked.connect(self.on_record)
        self.play_btn.clicked.connect(self.on_play)

    def on_record(self):
        record_macro(self.macro_path)

    def on_play(self):
        play_macro(self.macro_path)

    def get_name(self) -> str:
        return self.name_input.text().strip()
