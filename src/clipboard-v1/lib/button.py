from PySide6.QtCore import QSize
from PySide6.QtGui import Qt
from PySide6.QtWidgets import QPushButton, QApplication


class CustomButton(QPushButton):
    def __init__(self, icon, text, clipboard):
        self.full_text = text
        if len(text) > 18:
            text = text[:16] + '...'

        super().__init__(icon, text)

        self.setIconSize(QSize(12, 12))
        self.setFixedHeight(24)
        self.setStyleSheet('text-align: left;')
        self.copy_clipboard = clipboard

    def mousePressEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            QApplication.clipboard().setText(self.full_text)
        super().mousePressEvent(event)
