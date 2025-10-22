from PySide6.QtWidgets import QListWidget, QListWidgetItem, QPushButton, QLabel, QWidget, QHBoxLayout, QVBoxLayout
from PySide6.QtGui import QDesktopServices, QPixmap, QFont
from PySide6.QtCore import Qt, QUrl, QObject, QRunnable, QThreadPool, Signal, Slot, QSize

from lib.quol_window import QuolMainWindow
from lib.window_loader import WindowInfo, WindowContext
from lib.anime_fetcher import get_updated_anime

# CONFIG
THUMBNAIL_WIDTH = 40
THUMBNAIL_HEIGHT = 60
LIST_ITEM_HEIGHT = THUMBNAIL_HEIGHT + 20
MAX_PAGE = 100


class FetchWorkerSignals(QObject):
    finished = Signal(list)
    error = Signal(str)


class FetchAnimeWorkerWithPage(QRunnable):
    def __init__(self, page, sub_only):
        super().__init__()
        self.page = page
        self.sub_only = sub_only
        self.signals = FetchWorkerSignals()

    @Slot()
    def run(self):
        try:
            data = get_updated_anime(page=self.page, sub_only=self.sub_only)
            self.signals.finished.emit(data)
        except Exception as e:
            self.signals.error.emit(str(e))


class ImageDownloadWorker(QRunnable):
    def __init__(self, url, item, signal):
        super().__init__()
        self.url = url
        self.item = item
        self.signal = signal

    def run(self):
        import requests
        try:
            headers = {"User-Agent": "Mozilla/5.0"}
            resp = requests.get(self.url, headers=headers, timeout=10)
            resp.raise_for_status()
            img_data = resp.content
            self.signal.emit(self.item, img_data)
        except Exception as e:
            print(f"Failed to download image {self.url}: {e}")


class AnimeListItem(QWidget):
    def __init__(self, title, episode, sub_or_dub, thumbnail_pixmap=None):
        super().__init__()

        layout = QHBoxLayout(self)
        layout.setSpacing(10)
        layout.setContentsMargins(8, 8, 8, 8)

        # Image thumbnail
        self.image_label = QLabel(self)
        self.image_label.setFixedSize(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT)
        self.image_label.setPixmap(thumbnail_pixmap or self._placeholder())
        self.image_label.setScaledContents(True)
        layout.addWidget(self.image_label)

        right_layout = QVBoxLayout()
        right_layout.setSpacing(4)

        self.title_label = QLabel(title)
        self.title_label.setWordWrap(True)
        self.title_label.setFont(QFont("Arial", 11, QFont.Weight.Bold))

        self.info_label = QLabel(f"{episode} {sub_or_dub}")
        self.info_label.setFont(QFont("Arial", 9))
        self.info_label.setStyleSheet("color: gray")

        right_layout.addWidget(self.title_label)
        right_layout.addWidget(self.info_label)
        right_layout.addStretch()

        layout.addLayout(right_layout)

    def set_thumbnail(self, pixmap: QPixmap):
        self.image_label.setPixmap(pixmap)

    @staticmethod
    def _placeholder():
        pixmap = QPixmap(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT)
        pixmap.fill(Qt.GlobalColor.lightGray)
        return pixmap


class MainWindow(QuolMainWindow):
    image_ready = Signal(QListWidgetItem, bytes)  # Signal to safely update UI with image

    def __init__(self, window_info: WindowInfo, window_context: WindowContext):
        super().__init__('Anime', window_info, window_context, default_geometry=(1340, 10, 240, 200))

        self.current_page = 1
        self.max_pages = MAX_PAGE

        self.setFixedHeight(self.config['window_height'])

        nav_layout = QHBoxLayout()
        self.prev_button = QPushButton("🡄", self)
        self.page_label = QLabel(f"Page {self.current_page}", self)
        self.next_button = QPushButton("🡆", self)

        self.prev_button.clicked.connect(self.prev_page)
        self.next_button.clicked.connect(self.next_page)

        nav_layout.addWidget(self.prev_button)
        nav_layout.addWidget(self.page_label, alignment=Qt.AlignmentFlag.AlignCenter)
        nav_layout.addWidget(self.next_button)
        self.layout.addLayout(nav_layout)

        self.list_widget = QListWidget(self)
        self.list_widget.setVerticalScrollMode(QListWidget.ScrollMode.ScrollPerPixel)
        self.list_widget.setStyleSheet("""
            QListWidget::item {
                border-bottom: 1px solid #ccc;
                margin: 0px 0;
                padding: 0px;
            }
        """)
        self.layout.addWidget(self.list_widget)

        self.refresh_button = QPushButton("Refresh", self)
        self.layout.addWidget(self.refresh_button)

        self.refresh_button.clicked.connect(self.refresh_list)
        self.list_widget.itemClicked.connect(self.on_item_clicked)
        self.image_ready.connect(self.set_item_icon)

        self.thread_pool = QThreadPool()

        self.refresh_list()

    def on_update_config(self):
        self.setFixedHeight(self.config['window_height'])
        self.refresh_list()

    def set_controls_enabled(self, enabled: bool):
        self.prev_button.setEnabled(enabled and self.current_page > 1)
        self.next_button.setEnabled(enabled and self.current_page < self.max_pages)
        self.refresh_button.setEnabled(enabled)

    def refresh_list(self):
        self.page_label.setText(f"Page {self.current_page}")
        self.list_widget.clear()

        self.set_controls_enabled(False)

        worker = FetchAnimeWorkerWithPage(self.current_page, self.config['sub_only'])
        worker.signals.finished.connect(self.populate_list)
        worker.signals.error.connect(self.handle_error)
        self.thread_pool.start(worker)

    def populate_list(self, data):
        self.list_widget.clear()

        for entry in data:
            title = entry['title']
            episode = entry['episode']
            # sub_or_dub = f'({entry['sub']}{'/' if entry['sub'] and entry['dub'] else ''}{entry['dub']})'
            sub_or_dub = ''
            img_url = entry['img_url']

            item = QListWidgetItem()
            item.setSizeHint(QSize(0, LIST_ITEM_HEIGHT))
            item.setData(Qt.ItemDataRole.UserRole, entry['anime_url'])

            widget = AnimeListItem(title, episode, sub_or_dub)
            item.setData(Qt.ItemDataRole.UserRole + 1, widget)

            self.list_widget.addItem(item)
            self.list_widget.setItemWidget(item, widget)

            if img_url and img_url != 'No Image':
                worker = ImageDownloadWorker(img_url, item, self.image_ready)
                self.thread_pool.start(worker)

        self.set_controls_enabled(True)

    def set_item_icon(self, item, img_data):
        pixmap = QPixmap()
        if pixmap.loadFromData(img_data):
            scaled_pixmap = pixmap.scaled(
                THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT,
                Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation
            )
            widget = item.data(Qt.ItemDataRole.UserRole + 1)
            if widget:
                widget.set_thumbnail(scaled_pixmap)

    def handle_error(self, error_message):
        print("Failed to fetch anime data:", error_message)
        self.page_label.setText(f"Page {self.current_page} (Load Failed)")
        self.set_controls_enabled(True)

    def prev_page(self):
        if self.current_page > 1:
            self.current_page -= 1
            self.refresh_list()

    def next_page(self):
        if self.current_page < self.max_pages:
            self.current_page += 1
            self.refresh_list()

    @staticmethod
    def on_item_clicked(item: QListWidgetItem):
        url = item.data(Qt.ItemDataRole.UserRole)
        if url:
            QDesktopServices.openUrl(QUrl(url))
