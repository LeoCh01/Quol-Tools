from PySide6.QtCore import QTimer, QSize, Qt
from PySide6.QtGui import QPixmap, QColor, QGuiApplication, QCursor, QPainter, QIcon
from PySide6.QtWidgets import QLabel, QGridLayout, QPushButton

from qlib.windows.quol_window import QuolMainWindow
from qlib.windows.tool_loader import ToolSpec


class MainWindow(QuolMainWindow):
    def __init__(self, tool_spec: ToolSpec):
        super().__init__('Color', tool_spec, default_geometry=(200, 10, 150, 1), show_config=False)

        self.grid_layout = QGridLayout()

        self.hex = QLabel()
        self.hex.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.hex.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse)
        self.grid_layout.addWidget(self.hex, 0, 2)

        self.rgb = QLabel()
        self.rgb.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.rgb.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse)
        self.grid_layout.addWidget(self.rgb, 1, 2)

        self.pixmap_label = QLabel()
        self.grid_layout.addWidget(self.pixmap_label, 0, 0, 3, 1)

        self.select_icon = QIcon(self.tool_spec.path + '/res/img/pick.svg')
        self.select_btn = QPushButton()
        self.select_btn.setIcon(self.select_icon)
        self.select_btn.setCheckable(True)
        self.select_btn.clicked.connect(self.select_color)
        self.grid_layout.addWidget(self.select_btn, 2, 2)

        self.layout.addLayout(self.grid_layout)

        screen = QGuiApplication.primaryScreen()
        self.sf = screen.devicePixelRatio() if screen else 1.0

        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_color)
        self.esc_id = None
        self.update_color()

    def copy_color(self, t):
        clipboard = QGuiApplication.clipboard()
        if t == 'hex':
            clipboard.setText(self.hex.text())
        elif t == 'rgb':
            clipboard.setText(self.rgb.text())

    def select_color(self):
        if self.timer.isActive():
            return

        self.select_btn.setText('Esc to stop')
        self.select_btn.setIcon(QIcon())
        self.select_btn.setStyleSheet('background-color: #eee; color: #000')
        self.select_btn.setChecked(True)

        self.timer.start(100)

        self.esc_id = self.tool_spec.input_manager.add_key_press_listener(
            self.on_key_press, suppressed=('esc',)
        )

    def on_key_press(self, key):
        if key.lower() != 'esc':
            return

        if self.esc_id is not None:
            self.tool_spec.input_manager.remove_key_press_listener(self.esc_id)
            self.esc_id = None

        self.timer.stop()
        self.select_btn.setText('')
        self.select_btn.setIcon(self.select_icon)
        self.select_btn.setStyleSheet('')
        self.select_btn.setChecked(False)

    def update_color(self):
        screen = QGuiApplication.primaryScreen()
        if not screen:
            return

        pos = QCursor.pos()

        ps = 5

        x = (pos.x() - (ps / 2) / self.sf)
        y = (pos.y() - (ps / 2) / self.sf)
        w = (ps / self.sf)
        h = (ps / self.sf)

        if w <= 0 or h <= 0:
            return

        pixmap = screen.grabWindow(0, x, y, w, h)
        if pixmap.isNull():
            return

        image = pixmap.toImage()
        if image.isNull() or image.width() < 3 or image.height() < 3:
            return

        scaled_pixmap = QPixmap.fromImage(image).scaled(
            QSize(int(55 * self.sf), int(55 * self.sf)),
            Qt.AspectRatioMode.IgnoreAspectRatio,
            Qt.TransformationMode.FastTransformation,
        )

        self.draw_frame(scaled_pixmap)
        self.pixmap_label.setPixmap(scaled_pixmap)

        center_color = QColor(image.pixel(ps // 2, ps // 2))
        self.hex.setText(center_color.name())
        self.rgb.setText(
            f'{center_color.red()},{center_color.green()},{center_color.blue()}'
        )

    def draw_frame(self, pixmap: QPixmap):
        painter = QPainter()
        if not painter.begin(pixmap):
            return

        pen = painter.pen()
        pen.setColor(Qt.GlobalColor.white)
        pen.setWidth(2)
        painter.setPen(pen)

        cx = pixmap.width() / (self.sf * 2)
        cy = pixmap.height() / (self.sf * 2)
        sq = pixmap.width() / self.sf

        a = 5
        b = 10

        painter.drawRect(sq / a * 2, sq / a * 2, sq / a + 2, sq / a + 2)
        painter.drawRect(0, 0, sq, sq)

        pen.setWidth(1)
        painter.setPen(pen)

        painter.drawLine(cx, 0, cx, cx - sq / b - 1)
        painter.drawLine(0, cy, cy - sq / b - 1, cy)
        painter.drawLine(cx, sq, cx, cx + sq / b + 1)
        painter.drawLine(sq, cy, cy + sq / b + 1, cy)

        painter.end()

    def closeEvent(self, event):
        if self.timer.isActive():
            self.timer.stop()

        if self.esc_id is not None:
            self.tool_spec.input_manager.remove_key_press_listener(self.esc_id)
            self.esc_id = None

        super().closeEvent(event)
