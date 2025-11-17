import os
from PySide6.QtCore import QTimer
from PySide6.QtWidgets import (
    QVBoxLayout,
    QHBoxLayout,
    QPushButton,
    QWidget,
    QGroupBox,
)

from lib.io_helpers import read_json, write_json
from lib.quol_window import QuolMainWindow
from lib.window_loader import WindowInfo, WindowContext
from lib.keymap_group import KeymapGroupDialog


class MainWindow(QuolMainWindow):
    def __init__(self, window_info: WindowInfo, window_context: WindowContext):
        super().__init__(
            'Keymap',
            window_info,
            window_context,
            default_geometry=(200, 180, 180, 1),
            show_config=False
        )

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

        # Track which destination keys are currently pressed
        self.active_outputs = set()

        # Add a single key release listener (to release mapped outputs)
        im = self.window_context.input_manager
        self.release_listener_id = im.add_key_release_listener(self.on_key_release)

        QTimer.singleShot(0, self.load_mappings)

    # ===============================================================
    # 🔹 CALLBACK HELPERS
    # ===============================================================
    def make_press_callback(self, dst):
        """Create a callback that presses (but does not release) the mapped key."""
        def callback():
            im = self.window_context.input_manager
            im.send_press(dst)
            self.active_outputs.add(dst)
            return False  # prevent propagation
        return callback

    def on_key_release(self, key):
        """When *any* key is released, release mapped outputs that were pressed."""
        # Only release outputs that were previously pressed via mappings
        im = self.window_context.input_manager
        for dst in list(self.active_outputs):
            im.send_release(dst)
            self.active_outputs.discard(dst)

    # ===============================================================
    # 🔹 UI: GROUP MANAGEMENT
    # ===============================================================
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
        delete_btn.setStyleSheet("background-color: #f44336;")
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

        # ----------------------------
        # BUTTON CALLBACKS
        # ----------------------------
        def toggle_enable():
            group = self.mapping_groups[group_id]
            im = self.window_context.input_manager

            if not group['enabled']:
                # Register hotkeys for this group
                for src, dst in group['mappings']:
                    try:
                        handle = im.add_hotkey(
                            src,
                            self.make_press_callback(dst),
                            suppressed=True
                        )
                        group['handles'][src] = handle
                    except Exception as e:
                        print(f"Failed to bind {src} -> {dst}: {e}")
                group['enabled'] = True
                enable_btn.setStyleSheet("background-color: #4CAF50;")
            else:
                # Unregister hotkeys
                for handle in group['handles'].values():
                    im.remove_hotkey(handle)
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

                group['mappings'] = mappings
                group['name'] = name
                group_button.setText(name)

                # Refresh bindings if enabled
                if group['enabled']:
                    im = self.window_context.input_manager
                    for handle in group['handles'].values():
                        im.remove_hotkey(handle)
                    group['handles'].clear()
                    for src, dst in mappings:
                        try:
                            handle = im.add_hotkey(
                                src,
                                self.make_press_callback(dst),
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
        """Remove a group and unregister its hotkeys."""
        if group_id not in self.mapping_groups:
            return

        group = self.mapping_groups[group_id]
        im = self.window_context.input_manager

        if group['enabled']:
            for handle in group['handles'].values():
                im.remove_hotkey(handle)

        group['widget'].setParent(None)
        group['widget'].deleteLater()
        del self.mapping_groups[group_id]
        self.setFixedHeight(self.height() - 25)

    # ===============================================================
    # 🔹 SAVE / LOAD
    # ===============================================================
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

    # ===============================================================
    # 🔹 CLEANUP
    # ===============================================================
    def closeEvent(self, event):
        im = self.window_context.input_manager
        im.remove_key_release_listener(self.release_listener_id)

        # Unregister all hotkeys
        for group in self.mapping_groups.values():
            for handle in group['handles'].values():
                im.remove_hotkey(handle)

        super().closeEvent(event)
