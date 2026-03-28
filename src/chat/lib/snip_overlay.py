from PySide6.QtCore import QPoint, QRect, Qt
from PySide6.QtGui import QColor, QPainter, QPen, QPixmap
from PySide6.QtWidgets import QPushButton, QWidget


class SnipOverlay(QWidget):
    def __init__(self, screenshot: QPixmap, on_send):
        super().__init__()
        self.screenshot = screenshot
        self.on_send = on_send

        self.start_point = QPoint()
        self.end_point = QPoint()
        self.selection_rect = QRect()
        self.is_selecting = False

        self.setWindowFlags(
            Qt.WindowType.FramelessWindowHint
            | Qt.WindowType.WindowStaysOnTopHint
            | Qt.WindowType.Tool
        )
        self.setCursor(Qt.CursorShape.CrossCursor)

        self.send_btn = QPushButton('Send', self)
        self.send_btn.hide()
        self.send_btn.clicked.connect(self._send_selection)
        self.send_btn.setCursor(Qt.CursorShape.PointingHandCursor)
        self.send_btn.setStyleSheet(
            'background-color: #4CAF50; color: white; padding: 4px 10px; border-radius: 6px;'
        )

    def _selection_to_screenshot_rect(self) -> QRect:
        if self.selection_rect.isNull() or self.width() <= 0 or self.height() <= 0:
            return QRect()

        sx = self.screenshot.width() / self.width()
        sy = self.screenshot.height() / self.height()

        x = int(round(self.selection_rect.x() * sx))
        y = int(round(self.selection_rect.y() * sy))
        w = int(round(self.selection_rect.width() * sx))
        h = int(round(self.selection_rect.height() * sy))

        return QRect(x, y, w, h).intersected(self.screenshot.rect())

    def mousePressEvent(self, event):
        if event.button() != Qt.MouseButton.LeftButton:
            return

        self.is_selecting = True
        self.start_point = event.position().toPoint()
        self.end_point = self.start_point
        self.selection_rect = QRect()
        self.send_btn.hide()
        self.update()

    def mouseMoveEvent(self, event):
        if not self.is_selecting:
            return

        self.end_point = event.position().toPoint()
        self.selection_rect = QRect(self.start_point, self.end_point).normalized().intersected(self.rect())
        self.update()

    def mouseReleaseEvent(self, event):
        if event.button() != Qt.MouseButton.LeftButton:
            return

        self.is_selecting = False
        self.end_point = event.position().toPoint()
        self.selection_rect = QRect(self.start_point, self.end_point).normalized().intersected(self.rect())

        if self.selection_rect.width() < 8 or self.selection_rect.height() < 8:
            self.selection_rect = QRect()
            self.send_btn.hide()
            self.update()
            return

        self._place_send_button()
        self.send_btn.show()
        self.send_btn.raise_()
        self.update()

    def keyPressEvent(self, event):
        if event.key() == Qt.Key.Key_Escape:
            self.close()
            return
        super().keyPressEvent(event)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.drawPixmap(self.rect(), self.screenshot)

        painter.fillRect(self.rect(), QColor(0, 0, 0, 60))

        if not self.selection_rect.isNull():
            screenshot_rect = self._selection_to_screenshot_rect()
            if not screenshot_rect.isNull():
                cropped = self.screenshot.copy(screenshot_rect)
                painter.drawPixmap(self.selection_rect, cropped)

            pen = QPen(QColor(80, 190, 255), 2)
            painter.setPen(pen)
            painter.setBrush(Qt.BrushStyle.NoBrush)
            painter.drawRect(self.selection_rect)

    def _place_send_button(self):
        btn_size = self.send_btn.sizeHint()
        pad = 8

        x = self.selection_rect.right() - btn_size.width() - pad
        y = self.selection_rect.bottom() - btn_size.height() - pad

        x = max(self.selection_rect.left() + pad, x)
        y = max(self.selection_rect.top() + pad, y)

        x = max(0, min(x, self.width() - btn_size.width()))
        y = max(0, min(y, self.height() - btn_size.height()))

        self.send_btn.setGeometry(x, y, btn_size.width(), btn_size.height())

    def _send_selection(self):
        if self.selection_rect.isNull():
            return

        screenshot_rect = self._selection_to_screenshot_rect()
        if screenshot_rect.isNull():
            return

        cropped = self.screenshot.copy(screenshot_rect)
        self.close()
        self.on_send(cropped)
