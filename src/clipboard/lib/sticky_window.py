from PySide6.QtWidgets import QSizePolicy, QTextEdit

from lib.quol_window import QuolResizableSubWindow


class StickyWindow(QuolResizableSubWindow):

    def __init__(self, main_window: 'MainWindow', wid: str, text: str, pos: tuple):
        super().__init__(main_window, wid)
        self.main_window = main_window

        self.setGeometry(pos[0], pos[1], 300, 300)
        self.setMinimumSize(150, 150)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)

        self.wid = wid
        self.text_edit = QTextEdit(self)
        self.text_edit.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.text_edit.setPlaceholderText('...')
        self.text_edit.setText(text)
        self.text_edit.textChanged.connect(self.save_note)
        self.layout.addWidget(self.text_edit)

    def save_note(self):
        self.main_window.clipboard['sticky'][self.wid] = self.text_edit.toPlainText()
        self.main_window.save_clipboard()

    def close(self):
        super().close()

        if not self.main_window.clipboard['sticky'].get(self.wid):
            del self.main_window.clipboard['sticky'][self.wid]
            self.main_window.save_clipboard()

        self.main_window.sticky_notes.pop(self.wid, None)

        if not self.main_window.sticky_notes:
            self.main_window.window_context.toggle.disconnect(self.toggle_windows)
