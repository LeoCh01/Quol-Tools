import collections

from PySide6.QtGui import QColor, QMouseEvent, QPainter, Qt, QPixmap, QPen, QShortcut, QKeySequence, QCursor, QPainterPath
from PySide6.QtWidgets import QPushButton, QHBoxLayout, QWidget, QApplication, QSlider, QLabel, QVBoxLayout
from PySide6.QtCore import QPoint, Signal

from lib.quol_window import QuolMainWindow
from lib.window_loader import WindowInfo, WindowContext
from lib.color_wheel import ColorWheel


class MainWindow(QuolMainWindow):
    toggle = Signal()

    def __init__(self, window_info: WindowInfo, window_context: WindowContext):
        super().__init__('Draw', window_info, window_context, default_geometry=(930, 10, 190, 1))

        self.drawing_widget = DrawingWidget()

        self.top_layout = QHBoxLayout()
        self.layout.addLayout(self.top_layout)

        self.color_wheel = ColorWheel()
        self.color_wheel.color_changed.connect(self.drawing_widget.set_pen_color)
        self.top_layout.addWidget(self.color_wheel)

        self.control_layout = QVBoxLayout()

        self.clear_button = QPushButton('Clear')
        self.clear_button.clicked.connect(self.drawing_widget.clear_canvas)
        self.control_layout.addWidget(self.clear_button)

        self.start_button = QPushButton('Start')
        self.start_button.clicked.connect(self.on_start_clicked)
        self.control_layout.addWidget(self.start_button)
        self.drawing_widget.close_sc.activated.connect(self.on_start_clicked)

        self.stroke_slider = QSlider(Qt.Orientation.Horizontal)
        self.stroke_slider.setRange(1, 30)
        self.stroke_slider.setValue(2)
        self.stroke_slider.valueChanged.connect(self.update_stroke_size)
        self.stroke_label = QLabel("2")
        self.control_layout.addWidget(self.stroke_label)
        self.control_layout.addWidget(self.stroke_slider)

        self.top_layout.addLayout(self.control_layout)

        self.toggle.connect(self.on_start_clicked)
        self.toggle_id = self.window_context.input_manager.add_hotkey(self.config['draw_toggle'], lambda: self.toggle.emit(), suppressed=True)

    def on_update_config(self):
        self.window_context.input_manager.remove_hotkey(self.toggle_id)
        self.toggle_id = self.window_context.input_manager.add_hotkey(self.config['draw_toggle'], lambda: self.toggle.emit(), suppressed=True)

    def on_start_clicked(self):
        if self.start_button.text() == 'Start':
            self.start_button.setText('Stop')
            self.drawing_widget.start_drawing(self.window_context)
        else:
            self.start_button.setText('Start')
            self.drawing_widget.stop_drawing()

    def update_stroke_size(self, value):
        # update text
        self.stroke_label.setText(f"{value}")
        self.drawing_widget.set_pen_width(value)

    def close(self):
        self.drawing_widget.stop_drawing()
        self.drawing_widget.close()
        super().close()


class DrawingWidget(QWidget):
    def __init__(self):
        super().__init__()

        self.setAttribute(Qt.WidgetAttribute.WA_StaticContents)
        self.setWindowFlags(Qt.WindowType.FramelessWindowHint)
        screen_geometry = QApplication.primaryScreen().geometry()
        self.setFixedSize(screen_geometry.width(), screen_geometry.height() - 20)
        self.setMouseTracking(True)

        self.last_point = QPoint()
        self.pen_color = QColor('red')
        self.pen_width = 2

        self.undo_stack: [tuple[LineStroke, str]] = collections.deque(maxlen=30)

        self.undo_sc = QShortcut(QKeySequence('Ctrl+Z'), self)
        self.undo_sc.activated.connect(self.undo)
        self.close_sc = QShortcut(QKeySequence('Esc'), self)
        self.close_sc.activated.connect(self.hide)

        self.screenshot = QPixmap()
        self.setCursor(QCursor(Qt.CursorShape.CrossCursor))

        self.strokes: list[LineStroke] = []
        self.current_stroke = None
        self.eraser_mode = False
        self.eraser_multiplier = 3
        self.is_ctrl_pressed = False

    def mousePressEvent(self, event: QMouseEvent):
        point = event.position().toPoint()

        if event.button() == Qt.MouseButton.LeftButton:
            self.strokes.append(LineStroke(self.pen_color, self.pen_width))
            self.strokes[-1].add_point(point)
            self.current_stroke = self.strokes[-1]
            self.undo_stack.append((self.current_stroke, 'add'))
            self.update()

        elif event.buttons() == Qt.MouseButton.RightButton:
            self.eraser_mode = True
            self.erase_stroke_at(point)
            self.setCursor(QCursor(Qt.CursorShape.BlankCursor))
            self.update()

    def mouseMoveEvent(self, event: QMouseEvent):
        point = event.position().toPoint()

        if event.buttons() & Qt.MouseButton.LeftButton and self.current_stroke:
            if self.is_ctrl_pressed:
                self.current_stroke.type = 'snap'
            else:
                self.current_stroke.type = 'free'
            self.current_stroke.add_point(point)

        elif event.buttons() & Qt.MouseButton.RightButton:
            self.erase_stroke_at(point)

        self.update()

    def mouseReleaseEvent(self, event: QMouseEvent):
        if self.current_stroke:
            self.current_stroke.to_free()

        if event.button() == Qt.MouseButton.RightButton:
            self.eraser_mode = False
            self.current_stroke = None
            self.setCursor(QCursor(Qt.CursorShape.CrossCursor))
            self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.drawPixmap(self.rect(), self.screenshot)

        for stroke in self.strokes:
            stroke.draw(painter)

        if self.eraser_mode:
            self.draw_eraser_indicator(painter)
        else:
            self.draw_cursor_coordinates(painter)

    def keyPressEvent(self, event):
        if event.key() == Qt.Key.Key_Control:
            self.is_ctrl_pressed = True

    def keyReleaseEvent(self, event):
        if event.key() == Qt.Key.Key_Control:
            self.is_ctrl_pressed = False

    @staticmethod
    def draw_path(painter, points, color, width):
        path = QPainterPath()
        path.moveTo(points[0])
        for pt in points[1:]:
            path.lineTo(pt)

        pen = QPen(color, width, Qt.PenStyle.SolidLine, Qt.PenCapStyle.RoundCap, Qt.PenJoinStyle.RoundJoin)
        painter.setPen(pen)
        painter.drawPath(path)

    def erase_stroke_at(self, pos: QPoint):
        for i in reversed(range(len(self.strokes))):
            stroke = self.strokes[i]
            for p in stroke.points:
                t = stroke.width / 2 + (3 + self.pen_width ** 0.8) * self.eraser_multiplier
                dx = p.x() - pos.x()
                dy = p.y() - pos.y()
                distance = (dx * dx + dy * dy) ** 0.5
                if distance <= t:
                    self.undo_stack.append((self.strokes.pop(i), 'remove'))
                    break
        self.update()

    def set_pen_color(self, color: QColor):
        self.pen_color = color
        self.update()

    def clear_canvas(self):
        self.strokes.clear()
        self.update()

    def undo(self):
        if self.undo_stack:
            stroke, action = self.undo_stack.pop()
            if action == 'add':
                self.strokes.remove(stroke)
            elif action == 'remove':
                self.strokes.append(stroke)
            self.update()

    def start_drawing(self, context):
        screen = QApplication.primaryScreen()
        g = screen.geometry()
        g2 = g.adjusted(0, 0, 0, -20)

        context.toggle_windows_instant(True)
        self.screenshot = screen.grabWindow(0, g2.x(), g2.y(), g2.width(), g2.height())
        context.toggle_windows_instant(False)

        self.update()
        self.show()

    def stop_drawing(self):
        self.hide()

    def set_pen_width(self, width: int):
        self.pen_width = width
        self.update()

    def draw_cursor_coordinates(self, painter: QPainter):
        pos = self.mapFromGlobal(QCursor.pos())
        text = f"({pos.x()}, {pos.y()})"

        painter.setCompositionMode(QPainter.CompositionMode.CompositionMode_Difference)

        font = painter.font()
        font.setPointSize(8)
        painter.setFont(font)

        painter.setPen(QColor(255, 255, 255))
        painter.drawText(pos + QPoint(10, -10), text)

        painter.setCompositionMode(QPainter.CompositionMode.CompositionMode_SourceOver)

    def draw_eraser_indicator(self, painter: QPainter):
        eraser_size = (3 + self.pen_width ** 0.8) * self.eraser_multiplier
        cursor_pos = self.mapFromGlobal(QCursor.pos())

        painter.setCompositionMode(QPainter.CompositionMode.CompositionMode_Difference)

        pen = QPen(QColor(255, 255, 255), 2, Qt.PenStyle.SolidLine)
        painter.setPen(pen)
        painter.setBrush(Qt.BrushStyle.NoBrush)
        painter.drawEllipse(cursor_pos, eraser_size, eraser_size)

        painter.setCompositionMode(QPainter.CompositionMode.CompositionMode_SourceOver)


class LineStroke:
    def __init__(self, color, width):
        self.points = []
        self.color = color
        self.width = width
        self.type = 'point'

    def add_point(self, point):
        while self.type == 'snap' and len(self.points) >= 2:
            self.points.pop()
        self.points.append(point)

    def to_free(self):
        if self.type == 'snap':
            self.type = 'free'
        else:
            return

        d = self.points[0] - self.points[-1]
        d = int((d.x() ** 2 + d.y() ** 2) ** 0.5 // 10)

        for i in range(d):
            t = (i + 1) / (d + 1)
            x = int(self.points[0].x() * (1 - t) + self.points[-1].x() * t)
            y = int(self.points[0].y() * (1 - t) + self.points[-1].y() * t)
            self.points.insert(-1, QPoint(x, y))

    def draw(self, painter):
        if not self.points:
            return

        if len(self.points) == 1:
            painter.setBrush(self.color)
            painter.setPen(Qt.PenStyle.NoPen)
            painter.drawEllipse(self.points[0], self.width / 2, self.width / 2)
            painter.setBrush(Qt.BrushStyle.NoBrush)
            return

        path = QPainterPath()
        pen = QPen(self.color, self.width, Qt.PenStyle.SolidLine, Qt.PenCapStyle.RoundCap, Qt.PenJoinStyle.RoundJoin)
        painter.setPen(pen)
        path.moveTo(self.points[0])

        for pt in self.points[1:]:
            path.lineTo(pt)

        painter.drawPath(path)
