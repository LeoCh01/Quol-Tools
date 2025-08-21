from PySide6.QtWidgets import QVBoxLayout, QHBoxLayout, QPushButton, QLineEdit, QWidget, QGroupBox
from lib.quol_window import QuolMainWindow, QuolDialogWindow


class KeymapGroupDialog(QuolDialogWindow):
    def __init__(self, main_window: QuolMainWindow, group_name='', mappings=None):
        super().__init__(main_window, "Edit Keymap Group")
        self.setGeometry(300, 300, 200, 300)

        self.group_name_input = QLineEdit(group_name)
        self.group_name_input.setPlaceholderText("Group name")
        self.layout.addWidget(self.group_name_input)

        self.groupbox = QGroupBox("Key Mappings")
        self.groupbox_layout = QVBoxLayout()
        self.groupbox.setLayout(self.groupbox_layout)
        self.layout.addWidget(self.groupbox)

        self.add_mapping_btn = QPushButton("+ Add Mapping")
        self.add_mapping_btn.clicked.connect(lambda: self.add_mapping_row())
        self.layout.addWidget(self.add_mapping_btn)

        self.mapping_rows = []

        if mappings:
            for src, dst in mappings:
                self.add_mapping_row(src, dst)

    def add_mapping_row(self, src: str = '', dst: str = ''):
        row_widget = QWidget()
        row_layout = QHBoxLayout(row_widget)
        row_layout.setContentsMargins(0, 0, 0, 0)
        row_layout.setSpacing(0)

        src_input = QLineEdit()
        dst_input = QLineEdit()
        src_input.setText(src)
        dst_input.setText(dst)
        src_input.setPlaceholderText("Old")
        dst_input.setPlaceholderText("New")
        src_input.setFixedWidth(60)
        dst_input.setFixedWidth(60)

        delete_btn = QPushButton("✖")
        delete_btn.setFixedWidth(20)
        delete_btn.clicked.connect(lambda: self.remove_mapping_row(row_widget))

        row_layout.addWidget(src_input)
        row_layout.addWidget(dst_input)
        row_layout.addWidget(delete_btn)

        self.groupbox_layout.addWidget(row_widget)
        self.mapping_rows.append((src_input, dst_input, row_widget))

    def remove_mapping_row(self, row_widget):
        for i, (_, _, widget) in enumerate(self.mapping_rows):
            if widget == row_widget:
                self.mapping_rows.pop(i)
                break
        row_widget.setParent(None)
        row_widget.deleteLater()

    def get_group_data(self):
        group_name = self.group_name_input.text().strip()
        mappings = [
            (src.text().strip().lower(), dst.text().strip().lower())
            for src, dst, _ in self.mapping_rows
            if src.text().strip() and dst.text().strip()
        ]
        return group_name, mappings