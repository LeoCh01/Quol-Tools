from PySide6.QtWidgets import QLineEdit

from lib.quol_window import QuolDialogWindow


class NoteNameDialog(QuolDialogWindow):
    def __init__(self, parent=None):
        super().__init__(parent, "Note")
        self.setGeometry(45, 130, 300, 1)

        self.name_input = QLineEdit(self)
        self.name_input.setPlaceholderText("Enter note name...")
        self.layout.addWidget(self.name_input)

    def get_name(self):
        return self.name_input.text().strip()