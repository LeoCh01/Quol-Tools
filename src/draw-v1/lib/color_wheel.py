import math

from PySide6.QtCore import Signal, QPoint, QRectF, Qt
from PySide6.QtGui import QColor, QPainter, QPen, QMouseEvent
from PySide6.QtWidgets import QWidget


class ColorWheel(QWidget):
    color_changed = Signal(QColor)

    def __init__(self, circle_radius=45, square_size=40, thickness=13):
        super().__init__()
        self.setMinimumSize(circle_radius * 2, circle_radius * 2)
        self.hue = 0
        self.saturation = 1.0
        self.value = 1.0
        self.radius = circle_radius
        self.sq_size = square_size
        self.thickness = thickness

        self.is_hue_wheel = None

    def get_color(self):
        return QColor.fromHsv(self.hue, int(self.saturation * 255), int(self.value * 255))

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        self.radius = min(self.width(), self.height()) // 2 - 10
        center = QPoint(self.width() // 2, self.height() // 2)

        # ring
        for angle in range(360):
            color = QColor.fromHsv(angle, 255, 255)
            painter.setPen(QPen(color, self.thickness))
            painter.drawArc(QRectF(center.x() - self.radius, center.y() - self.radius,
                                   self.radius * 2, self.radius * 2), angle * 16, 16)

        # square
        square_size = self.sq_size
        square_top_left = QPoint(center.x() - square_size // 2, center.y() - square_size // 2)
        for x in range(int(square_size)):
            for y in range(int(square_size)):
                s = x / square_size
                v = 1 - y / square_size
                color = QColor.fromHsv(self.hue, int(s * 255), int(v * 255))
                painter.setPen(color)
                painter.drawPoint(square_top_left + QPoint(x, y))

        painter.setPen(QPen(Qt.GlobalColor.white, 2))
        painter.setBrush(Qt.BrushStyle.NoBrush)

        # Hue
        angle_rad = math.radians(self.hue)
        hx = center.x() + self.radius * math.cos(angle_rad)
        hy = center.y() - self.radius * math.sin(angle_rad)
        painter.drawEllipse(QPoint(hx, hy), 5, 5)

        # SV
        sv_x = int(square_top_left.x() + self.saturation * square_size)
        sv_y = int(square_top_left.y() + (1 - self.value) * square_size)
        painter.drawEllipse(QPoint(sv_x, sv_y), 5, 5)

    def mousePressEvent(self, event: QMouseEvent):
        center = QPoint(self.width() // 2, self.height() // 2)
        dx = event.position().x() - center.x()
        dy = event.position().y() - center.y()
        dist = math.hypot(dx, dy)
        if self.radius - 10 < dist < self.radius + 10:
            self.is_hue_wheel = True
        else:
            self.is_hue_wheel = False

        self.handle_mouse(event)

    def mouseMoveEvent(self, event: QMouseEvent):
        self.handle_mouse(event)

    def handle_mouse(self, event: QMouseEvent):
        center = QPoint(self.width() // 2, self.height() // 2)
        dx = event.position().x() - center.x()
        dy = event.position().y() - center.y()

        if self.is_hue_wheel:
            # Hue ring
            angle = math.degrees(math.atan2(-dy, dx)) % 360
            self.hue = int(angle)
            self.update()
        else:
            # SV square
            top_l = QPoint(center.x() - self.sq_size // 2, center.y() - self.sq_size // 2)
            x = event.position().x() - top_l.x()
            y = event.position().y() - top_l.y()
            self.saturation = min(max(x / self.sq_size, 0), 1)
            self.value = 1 - min(max(y / self.sq_size, 0), 1)
            self.update()

        self.color_changed.emit(self.get_color())