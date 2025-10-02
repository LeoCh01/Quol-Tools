import os

from PySide6.QtCore import QTimer
from PySide6.QtWidgets import QVBoxLayout, QHBoxLayout, QPushButton, QWidget, QGroupBox

from lib.io_helpers import read_json, write_json
from lib.quol_window import QuolMainWindow
from lib.window_loader import WindowInfo, WindowContext
from lib.keymap_group import KeymapGroupDialog


class MainWindow(QuolMainWindow):
    def __init__(self, window_info: WindowInfo, window_context: WindowContext):
        super().__init__('Keymap', window_info, window_context, default_geometry=(200, 180, 180, 1), show_config=False)

        self.keymap_groupbox = QGroupBox('Key Mappings')
        self.keymap_layout = QVBoxLayout()
        self.keymap_groupbox.setLayout(self.keymap_layout)
        self.keymap_layout.setContentsMargins(0, 5, 0, 5)
        self.layout.addWidget(self.keymap_groupbox)

        self.add_button = QPushButton('+ Add Mapping Group')
        self.add_button.clicked.connect(lambda: self.add_group_row())
        self.layout.addWidget(self.add_button)

        self.mapping_groups: dict[str, dict] = {}
        self.group_counter = 0

        self.mappings_path = window_info.path + '/res/keymaps.json'

        QTimer.singleShot(0, self.load_mappings)

    def make_send_callback(self, dst):
        def callback():
            self.window_context.input_manager.send_keys(dst)
            return False

        return callback

    def add_group_row(self, name="Unnamed", mappings=None):
        self.setFixedHeight(self.height() + 25)
        group_id = f"__group_{self.group_counter}"
        self.group_counter += 1

        row_widget = QWidget()
        row_layout = QHBoxLayout(row_widget)
        row_layout.setContentsMargins(0, 0, 0, 0)
        row_layout.setSpacing(0)

        group_button = QPushButton(name)
        group_button.setFixedWidth(100)

        enable_btn = QPushButton("✔")
        enable_btn.setFixedWidth(20)

        delete_btn = QPushButton("✖")
        delete_btn.setFixedWidth(20)

        row_layout.addWidget(group_button)
        row_layout.addWidget(enable_btn)
        row_layout.addWidget(delete_btn)

        self.keymap_layout.addWidget(row_widget)

        self.mapping_groups[group_id] = {
            'widget': row_widget,
            'button': group_button,
            'mappings': mappings if mappings else [],
            'enabled': False,
            'handles': {},
            'name': name
        }

        def toggle_enable():
            group = self.mapping_groups[group_id]
            if not group['enabled']:
                for src, dst in group['mappings']:
                    try:
                        handle = self.window_context.input_manager.add_hotkey(
                            src,
                            self.make_send_callback(dst),
                            suppressed=True
                        )
                        group['handles'][src] = handle
                    except Exception as e:
                        print(f"Failed to bind {src} -> {dst}: {e}")
                group['enabled'] = True
                enable_btn.setStyleSheet("background-color: #4CAF50;")
            else:
                for handle in group['handles'].values():
                    self.window_context.input_manager.remove_hotkey(handle)
                group['handles'].clear()
                group['enabled'] = False
                enable_btn.setStyleSheet("")

        def delete_group():
            self.remove_group(group_id)
            self.save_mappings()

        def open_dialog():
            group = self.mapping_groups[group_id]
            dialog = KeymapGroupDialog(self, group['name'], group['mappings'])

            def on_accept():
                name, mappings = dialog.get_group_data()
                if not name or not mappings:
                    return

                # Update group data
                group['mappings'] = mappings
                group['name'] = name
                group_button.setText(name)

                # Refresh hotkeys if enabled
                if group['enabled']:
                    for handle in group['handles'].values():
                        self.window_context.input_manager.remove_hotkey(handle)
                    group['handles'].clear()
                    for src, dst in mappings:
                        try:
                            handle = self.window_context.input_manager.add_hotkey(
                                src,
                                self.make_send_callback(dst),
                                suppressed=True
                            )
                            group['handles'][src] = handle
                        except Exception as e:
                            print(f"Failed to bind {src} -> {dst}: {e}")

                self.save_mappings()
                dialog.close()

            dialog.on_accept(on_accept)
            dialog.show()

        enable_btn.clicked.connect(toggle_enable)
        delete_btn.clicked.connect(delete_group)
        group_button.clicked.connect(open_dialog)

    def remove_group(self, group_id):
        if group_id not in self.mapping_groups:
            return

        group = self.mapping_groups[group_id]
        if group['enabled']:
            for handle in group['handles'].values():
                self.window_context.input_manager.remove_hotkey(handle)

        group['widget'].setParent(None)
        group['widget'].deleteLater()
        del self.mapping_groups[group_id]
        self.setFixedHeight(self.height() - 25)

    def save_mappings(self):
        data = {
            group['name']: {src: dst for src, dst in group['mappings']}
            for group in self.mapping_groups.values()
        }
        write_json(self.mappings_path, data)

    def load_mappings(self):
        if not os.path.exists(self.mappings_path):
            return
        data = read_json(self.mappings_path)
        for name, mappings_dict in data.items():
            self.add_group_row(name, list(mappings_dict.items()))

    def closeEvent(self, event):
        for group in self.mapping_groups.values():
            for handle in group['handles'].values():
                self.window_context.input_manager.remove_hotkey(handle)
        super().closeEvent(event)
