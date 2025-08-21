from PySide6 import QtCore
from PySide6.QtCore import QTimer
from PySide6.QtWidgets import QPushButton, QLabel, QGridLayout
from PySide6.QtGui import QPixmap, Qt, QMovie

import random

from lib.quol_window import QuolMainWindow
from lib.window_loader import WindowInfo, WindowContext

COIN = 'coin-x.png'
COIN_IMAGES = ['coin-h.png', 'coin-t.png']
DICE = 'dice-x.png'
DICE_IMAGES = ['dice-1.png', 'dice-2.png', 'dice-3.png', 'dice-4.png', 'dice-5.png', 'dice-6.png']
CONFETTI = 'confetti2.gif'


class MainWindow(QuolMainWindow):
    def __init__(self, window_info: WindowInfo, window_context: WindowContext):
        super().__init__('Chance', window_info, window_context, default_geometry=(570, 10, 150, 1), show_config=False)
        self.is_coin_flip = True

        self.grid_layout = QGridLayout()
        self.result_label = QLabel()
        self.result_label.setPixmap(QPixmap(self.window_info.path + '/res/img/' + COIN))
        self.result_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.coin_button = QPushButton('Coin')
        self.dice_button = QPushButton('Dice')

        self.grid_layout.addWidget(self.result_label, 0, 0, 1, 2)
        self.grid_layout.addWidget(self.coin_button, 1, 0)
        self.grid_layout.addWidget(self.dice_button, 1, 1)
        self.layout.addLayout(self.grid_layout)

        self.coin_button.clicked.connect(self.set_coin_flip)
        self.dice_button.clicked.connect(self.set_dice_roll)
        self.result_label.mousePressEvent = self.perform_action

        self.update_buttons()
        self.is_running = False

    def set_coin_flip(self):
        if self.is_running:
            return
        self.is_coin_flip = True
        self.update_buttons()

    def set_dice_roll(self):
        if self.is_running:
            return
        self.is_coin_flip = False
        self.update_buttons()

    def update_buttons(self):
        if self.is_coin_flip:
            self.coin_button.setStyleSheet('background-color: #696')
            self.dice_button.setStyleSheet('')
            self.result_label.setPixmap(QPixmap(self.window_info.path + '/res/img/' + COIN))
        else:
            self.coin_button.setStyleSheet('')
            self.dice_button.setStyleSheet('background-color: #696')
            self.result_label.setPixmap(QPixmap(self.window_info.path + '/res/img/' + DICE))

    def perform_action(self, event):
        if self.is_running:
            return
        self.is_running = True

        if self.is_coin_flip:
            self.flip_animation(random.sample(COIN_IMAGES, 2), random.randint(14, 15))
        else:
            self.flip_animation(random.sample(DICE_IMAGES, 6), random.randint(12, 16))

    def flip_animation(self, frames, total_flips, initial_interval=10):
        def update_flip():
            if self.flip_counter < total_flips:
                current_frame = frames[self.flip_counter % len(frames)]
                self.result_label.setPixmap(QPixmap(self.window_info.path + '/res/img/' + current_frame))
                self.flip_counter += 1

                new_interval = initial_interval + (self.flip_counter * 20)
                self.flip_timer.setInterval(new_interval)
            else:
                self.flip_timer.stop()
                self.confetti()

        self.flip_counter = 0
        self.flip_timer = QTimer(self)
        self.flip_timer.timeout.connect(update_flip)
        self.flip_timer.start(initial_interval)

    def confetti(self):
        self.confetti_label = QLabel(self)
        self.confetti_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.grid_layout.addWidget(self.confetti_label, 0, 0, 1, 2)

        confetti_movie = QMovie(self.window_info.path + '/res/img/' + CONFETTI)
        confetti_movie.setSpeed(150)
        confetti_movie.setScaledSize(QtCore.QSize(self.width() - 20, 80))

        self.confetti_label.setMovie(confetti_movie)
        confetti_movie.start()

        QTimer.singleShot(1700, self.hide_confetti)

    def hide_confetti(self):
        self.is_running = False
        self.grid_layout.removeWidget(self.confetti_label)
        self.confetti_label.deleteLater()

