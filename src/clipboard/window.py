from typing import Optional

from PySide6.QtCore import Signal, QTimer
from PySide6.QtGui import QIcon, Qt
from PySide6.QtWidgets import QGroupBox, QVBoxLayout, QPushButton, QHBoxLayout, QApplication, QSizePolicy

from lib.io_helpers import read_json, write_json
from lib.quol_window import QuolMainWindow
from lib.window_loader import WindowInfo, WindowContext
from lib.note_name_dialog import NoteNameDialog
from lib.button import CustomButton
from lib.sticky_window import StickyWindow


class MainWindow(QuolMainWindow):
    copy_signal = Signal()

    def __init__(self, window_info: WindowInfo, window_context: WindowContext):
        super().__init__('Clipboard', window_info, window_context, default_geometry=(10, 150, 180, 1))

        self.copy_signal.connect(self.on_copy)

        self.copy_params = QHBoxLayout()
        self.clear = QPushButton('Clear')
        self.clear.clicked.connect(self.on_clear)
        self.copy_params.addWidget(self.clear)

        self.note = QPushButton('Note')
        self.note.clicked.connect(lambda: self.on_note())
        self.copy_params.addWidget(self.note)

        self.layout.addLayout(self.copy_params)

        self.clip_layout = QVBoxLayout()
        self.clip_groupbox = QGroupBox('History')
        self.clip_groupbox.setLayout(self.clip_layout)
        self.clip_groupbox.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.layout.addWidget(self.clip_groupbox)

        self.copy_pressed = False
        self.window_context.input_manager.add_hotkey('ctrl+c', self.on_copy_hotkey)

        self.setFixedHeight(self.config['length'] * 30 + 110)
        self.clip_layout.setAlignment(Qt.AlignmentFlag.AlignTop)

        self.sticky_notes: dict[str, StickyWindow] = {}
        
        self.clipboard_path = self.window_info.path + '/res/clipboard.json'
        self.clipboard: Optional[dict] = None
        self.load_clipboard()
        
        if len(self.clipboard['copy']) > self.config['length']:
            self.clipboard['copy'] = self.clipboard['copy'][-self.config['length']:]

        for text in self.clipboard['copy']:
            self.clip_layout.insertWidget(0, self.create_copy_btn(text))

        for i, (k, note) in enumerate(self.clipboard['sticky'].items(), 1):
            if note:
                self.on_note(k, note, (i * 15, self.config['length'] * 30 + 200 + i * 45))

    def on_copy_hotkey(self):
        self.copy_signal.emit()

    def handle_key_release(self, event):
        print(f'Key released: {event.name}')
        if event.name == 'c':
            self.copy_pressed = False

    def create_copy_btn(self, text):
        return CustomButton(QIcon(self.window_info.path + '/res/img/copy.png'), text, self.clipboard['copy'])

    def save_clipboard(self):
        write_json(self.clipboard_path, self.clipboard)

    def load_clipboard(self):
        self.clipboard = read_json(self.clipboard_path)

    def update_clipboard(self):
        if not QApplication.clipboard().text():
            return

        self.clipboard['copy'].append(QApplication.clipboard().text())

        while len(self.clipboard['copy']) > self.config['length']:
            self.clipboard['copy'].pop(0)
            self.clip_layout.itemAt(self.config['length'] - 1).widget().deleteLater()

        self.clip_layout.insertWidget(0, self.create_copy_btn(self.clipboard['copy'][-1]))
        self.save_clipboard()

    def on_clear(self):
        self.clipboard['copy'] = []
        for i in reversed(range(self.clip_layout.count())):
            self.clip_layout.itemAt(i).widget().deleteLater()

        self.save_clipboard()

    def on_note(self, wid='', text='', pos=(100, 100)):
        if wid == '':
            dialog = NoteNameDialog(self)
            dialog.on_accept(lambda: self.open_note(dialog.get_name(), '', pos))
            dialog.show()
        else:
            self.open_note(wid, text, pos)

    def open_note(self, wid, text, pos):
        if wid in self.sticky_notes:
            self.sticky_notes[wid].raise_()
            self.sticky_notes[wid].activateWindow()
            return
        elif wid not in self.clipboard['sticky']:
            self.clipboard['sticky'][wid] = ''
            self.save_clipboard()
        else:
            text = self.clipboard['sticky'][wid]

        sticky_window = StickyWindow(self, wid, text, pos)
        self.sticky_notes[wid] = sticky_window
        self.window_context.toggle.connect(sticky_window.toggle_windows)

        sticky_window.show()
        sticky_window.raise_()
        sticky_window.activateWindow()

    def on_copy(self):
        print('copy')
        QTimer.singleShot(100, self.update_clipboard)

    def on_update_config(self):
        i = 0
        while len(self.clipboard['copy']) > self.config['length']:
            self.clipboard['copy'].pop(0)
            self.clip_layout.itemAt(self.config['length'] - i).widget().deleteLater()
            i += 1

        self.setFixedHeight(self.config['length'] * 30 + 110)

    def close(self):
        super().close()

        for wid, sticky in list(self.sticky_notes.items()):
            sticky.close()

        self.sticky_notes.clear()
