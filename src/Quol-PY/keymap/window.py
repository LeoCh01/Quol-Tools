import os
from PySide6.QtCore import QTimer
from PySide6.QtWidgets import (
    QVBoxLayout,
    QHBoxLayout,
    QPushButton,
    QWidget,
    QGroupBox,
)

from qlib.io_helpers import read_json, write_json
from qlib.windows.quol_window import QuolMainWindow
from qlib.windows.tool_loader import ToolSpec
from lib.keymap_group import KeymapGroupDialog


class MainWindow(QuolMainWindow):
    def __init__(self, tool_spec: ToolSpec):
        super().__init__('Keymap', tool_spec, default_geometry=(560, 10, 180, 1), show_config=False)

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
        self.mappings_path = tool_spec.path + '/res/keymaps.json'

        # Track active key states
        self.active_mappings = {}  # {src_key: dst_key}
        self.enabled_mapping_cache = {}  # {src: dst}
        self.held_keys = set()  # tracks physical held keys

        im = self.tool_spec.input_manager
        self.press_listener_id = im.add_key_press_listener(self.on_key_press)
        self.refresh_suppressed_keys()
        self.release_listener_id = im.add_key_release_listener(self.on_key_release)

        QTimer.singleShot(0, self.load_mappings)

    # ===============================================================
    # 🔹 INPUT CALLBACKS
    # ===============================================================
    def on_key_press(self, key):
        if not key:
            return

        key = key.lower()

        if key not in self.enabled_mapping_cache:
            return

        dst = self.enabled_mapping_cache[key]

        if key not in self.held_keys:
            # First physical press
            self.held_keys.add(key)
            self.tool_spec.input_manager.send_press(dst)
        else:
            # Repeat event
            self.tool_spec.input_manager.send_keys(dst)

    def on_key_release(self, key):
        if not key:
            return

        key = key.lower()

        if key in self.held_keys:
            self.held_keys.remove(key)

            if key in self.enabled_mapping_cache:
                dst = self.enabled_mapping_cache[key]
                self.tool_spec.input_manager.send_release(dst)

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
            'name': name
        }

        # ----------------------------
        # BUTTON CALLBACKS
        # ----------------------------
        def toggle_enable():
            group = self.mapping_groups[group_id]
            group['enabled'] = not group['enabled']
            self.rebuild_mapping_cache()

            if group['enabled']:
                enable_btn.setStyleSheet("background-color: #4CAF50;")
            else:
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

                self.save_mappings()
                self.rebuild_mapping_cache()
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

        group['widget'].setParent(None)
        group['widget'].deleteLater()
        del self.mapping_groups[group_id]

        self.rebuild_mapping_cache()

        self.setFixedHeight(self.height() - 25)

    def refresh_suppressed_keys(self):
        suppressed = tuple(self.enabled_mapping_cache.keys())

        im = self.tool_spec.input_manager
        im.remove_key_press_listener(self.press_listener_id)

        self.press_listener_id = im.add_key_press_listener(
            self.on_key_press,
            suppressed=suppressed
        )

    def rebuild_mapping_cache(self):
        cache = {}

        for group in self.mapping_groups.values():
            if not group['enabled']:
                continue

            for src, dst in group['mappings']:
                cache[src.lower()] = dst

        self.enabled_mapping_cache = cache
        self.refresh_suppressed_keys()

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
        im = self.tool_spec.input_manager
        im.remove_key_press_listener(self.press_listener_id)
        im.remove_key_release_listener(self.release_listener_id)

        super().closeEvent(event)
