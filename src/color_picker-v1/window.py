import keyboard

from PySide6.QtCore import QTimer, QSize, Qt
from PySide6.QtGui import QPixmap, QColor, QGuiApplication, QCursor, QPainter
from PySide6.QtWidgets import QLabel, QGridLayout, QPushButton

from lib.quol_window import QuolMainWindow
from lib.window_loader import WindowInfo, WindowContext


class MainWindow(QuolMainWindow):
    def __init__(self, window_info: WindowInfo, window_context: WindowContext):
        super().__init__('Color', window_info, window_context, default_geometry=(200, 10, 180, 1), show_config=False)

        self.grid_layout = QGridLayout()

        self.color_label = QLabel()
        self.grid_layout.addWidget(self.color_label, 0, 0)

        self.hex = QLabel()
        self.hex.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse)
        self.grid_layout.addWidget(self.hex, 0, 1)

        self.rgb = QLabel()
        self.rgb.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.rgb.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse)
        self.grid_layout.addWidget(self.rgb, 0, 2)

        self.pixmap_label = QLabel()
        self.grid_layout.addWidget(self.pixmap_label, 1, 0, 3, 2)

        self.copy_rgb = QPushButton('copy RGB')
        self.copy_rgb.setFixedWidth(79)
        self.copy_rgb.clicked.connect(lambda: self.copy_color('rgb'))
        self.grid_layout.addWidget(self.copy_rgb, 1, 2)

        self.copy_hex = QPushButton('copy HEX')
        self.copy_hex.setFixedWidth(79)
        self.copy_hex.clicked.connect(lambda: self.copy_color('hex'))
        self.grid_layout.addWidget(self.copy_hex, 2, 2)

        self.select_btn = QPushButton('pick color')
        self.select_btn.setFixedWidth(79)
        self.select_btn.setCheckable(True)
        self.select_btn.clicked.connect(self.select_color)
        self.grid_layout.addWidget(self.select_btn, 3, 2)

        self.layout.addLayout(self.grid_layout)
        self.sf = QGuiApplication.primaryScreen().devicePixelRatio()
        self.timer = QTimer()
        self.update_color()

        self.esc_key = None

    def copy_color(self, t):
        clipboard = QGuiApplication.clipboard()
        if t == 'hex':
            clipboard.setText(self.hex.text())
        elif t == 'rgb':
            clipboard.setText(self.rgb.text())

    def close(self):
        super().close()

    def select_color(self):
        self.select_btn.setText('Esc to stop')
        self.select_btn.setStyleSheet('background-color: #eee; color: #000')
        self.select_btn.setChecked(True)
        self.timer.timeout.connect(self.update_color)
        self.timer.start(100)
        self.esc_key = keyboard.on_press_key('esc', lambda _: self.on_color_select())

    def on_color_select(self):
        keyboard.unhook_key(self.esc_key)
        self.timer.stop()
        self.select_btn.setText('pick color')
        self.select_btn.setStyleSheet('')
        self.select_btn.setChecked(False)

    def update_color(self):
        pos = QCursor.pos()
        screen = QGuiApplication.primaryScreen()

        pixmap = screen.grabWindow(0, pos.x() - 2.5 / self.sf, pos.y() - 2.5 / self.sf, 5 / self.sf, 5 / self.sf)
        image = pixmap.toImage()
        scaled_pixmap = QPixmap.fromImage(image).scaled(QSize(75 * self.sf, 75 * self.sf))

        self.draw_frame(scaled_pixmap)
        self.pixmap_label.setPixmap(scaled_pixmap)

        center_color = QColor(image.pixel(2, 2))  # The center of the 5x5 image
        self.hex.setText(center_color.name())
        self.rgb.setText(f'({center_color.red()}, {center_color.green()}, {center_color.blue()})')

        self.color_label.setFixedSize(15, 15)
        self.color_label.setStyleSheet(f'background-color: {center_color.name()}; border: 1px solid black;')

    def draw_frame(self, pixmap):
        painter = QPainter(pixmap)
        pen = painter.pen()
        pen.setColor('white')
        pen.setWidth(2)
        painter.setPen(pen)

        cx = pixmap.width() / (self.sf * 2)
        cy = pixmap.height() / (self.sf * 2)
        sq = pixmap.width() / self.sf

        painter.drawRect(sq / 5 * 2, sq / 5 * 2, sq / 5 + 2, sq / 5 + 2)
        painter.drawRect(0, 0, sq, sq)
        pen.setWidth(1)
        painter.setPen(pen)
        painter.drawLine(cx, 0, cx, cx - sq / 10 - 1)
        painter.drawLine(0, cy, cy - sq / 10 - 1, cy)
        painter.drawLine(cx, sq, cx, cx + sq / 10 + 1)
        painter.drawLine(sq, cy, cy + sq / 10 + 1, cy)
        painter.end()
