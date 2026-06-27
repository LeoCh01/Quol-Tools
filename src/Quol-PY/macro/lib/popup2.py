from PySide6.QtCore import QTimer, QPropertyAnimation, QRect, QEasingCurve
from PySide6.QtGui import Qt
from PySide6.QtWidgets import QVBoxLayout, QWidget, QLabel, QApplication


class Popup:
    def __init__(self, text):
        screen_w = QApplication.primaryScreen().size().width()
        screen_h = QApplication.primaryScreen().size().height()

        self.top = PopupSub('', QRect(0, 0, screen_w, 50), 30, [0, -1])
        self.bottom = PopupSub(text, QRect(0, screen_h - 50, screen_w, 50), 30, [0, 1])
        self.left = PopupSub('', QRect(0, 0, 50, screen_h), 30, [-1, 0])
        self.right = PopupSub('', QRect(screen_w - 50, 0, 50, screen_h), 30, [1, 0])

    def play(self):
        print("Popup play start")
        self.top.play()
        self.bottom.play()
        self.left.play()
        self.right.play()


class PopupSub(QWidget):
    def __init__(self, text, geometry, h, d):
        super().__init__()
        self.setWindowFlags(Qt.WindowType.FramelessWindowHint | Qt.WindowType.Tool | Qt.WindowType.WindowStaysOnTopHint)
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground)
        self.setGeometry(geometry)
        self.setStyleSheet(
            f'background: qlineargradient(x1:{int(d[0] == -1)}, y1:{int(d[1] == -1)}, x2:{int(d[0] == 1)}, y2:{int(d[1] == 1)}, stop:0 #00000000, stop:1 #000000);'
        )

        layout = QVBoxLayout(self)
        self.text = QLabel(text, self)
        self.text.setStyleSheet('color: #fff;')
        self.text.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(self.text)
        layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(layout)

        self.fade_in_animation = QPropertyAnimation(self, b"windowOpacity")
        self.fade_out_animation = QPropertyAnimation(self, b"windowOpacity")

        self.t1 = 300
        self.t2 = 500
        self.h = h
        self.d = d

    def play(self):
        self.show()
        self.raise_()

        self.start_fade()

    def start_fade(self):
        self.fade_in_animation.setDuration(self.t1)
        self.fade_in_animation.setStartValue(0)
        self.fade_in_animation.setEndValue(1)
        self.fade_in_animation.start()

        QTimer.singleShot(self.t1 + 200, self.start_fade_out)

    def start_fade_out(self):
        self.fade_out_animation.setDuration(self.t1)
        self.fade_out_animation.setStartValue(1)
        self.fade_out_animation.setEndValue(0)
        self.fade_out_animation.start()

        QTimer.singleShot(self.t2, self.hide)


