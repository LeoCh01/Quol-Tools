#include "plugins/chat/lib/OcrTextOverlay.hpp"

#include <QClipboard>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QShowEvent>

namespace {
const QColor kAccent(86, 156, 255, 235);
const QColor kDim(0, 0, 0, 90);
const QColor kHover(86, 156, 255, 55);
const QColor kSelected(86, 156, 255, 130);

const char *kActionStyle =
    "QPushButton { background:#2e7d32; color:white; border:none; padding:6px 16px;"
    " border-radius:6px; font-size:12px; font-weight:600; }"
    "QPushButton:hover:!disabled { background:#388e3c; }"
    "QPushButton:disabled { background:#3a3a3e; color:#888888; }";

const char *kCancelStyle =
    "QPushButton { background:#3a3a3e; color:white; border:none; padding:6px 16px;"
    " border-radius:6px; font-size:12px; font-weight:600; }"
    "QPushButton:hover { background:#4a4a4f; }";
}  // namespace

OcrTextOverlay::OcrTextOverlay(const QPixmap &screenshot, const QRect &cropRect, QWidget *parent)
    : QWidget(parent), m_screenshot(screenshot), m_cropRect(cropRect) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setCursor(Qt::CrossCursor);
    setMouseTracking(true);
    setAttribute(Qt::WA_DeleteOnClose);

    m_toolbar = new QWidget(this);
    m_toolbar->setObjectName(QStringLiteral("ocr-toolbar"));
    m_toolbar->setStyleSheet(QStringLiteral(
        "#ocr-toolbar { background: rgba(24,24,26,0.92); border: 1px solid rgba(255,255,255,0.12);"
        " border-radius: 10px; }"
        "QLabel { color: #e6e6e6; font-size: 11px; }"
    ));
    m_toolbar->hide();

    auto *toolbarLayout = new QHBoxLayout(m_toolbar);
    toolbarLayout->setContentsMargins(12, 6, 8, 6);
    toolbarLayout->setSpacing(8);

    m_statusLabel = new QLabel(QStringLiteral("Recognizing..."), m_toolbar);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setFixedWidth(170);
    toolbarLayout->addWidget(m_statusLabel);

    m_copyButton = new QPushButton(QStringLiteral("Copy"), m_toolbar);
    m_copyButton->setCursor(Qt::PointingHandCursor);
    m_copyButton->setStyleSheet(QLatin1String(kActionStyle));
    m_copyButton->setEnabled(false);
    toolbarLayout->addWidget(m_copyButton);

    m_cancelButton = new QPushButton(QStringLiteral("Cancel"), m_toolbar);
    m_cancelButton->setCursor(Qt::PointingHandCursor);
    m_cancelButton->setStyleSheet(QLatin1String(kCancelStyle));
    toolbarLayout->addWidget(m_cancelButton);

    connect(m_copyButton, &QPushButton::clicked, this, &OcrTextOverlay::copySelection);
    connect(m_cancelButton, &QPushButton::clicked, this, &OcrTextOverlay::close);

    m_tipLabel = new QLabel(
        QStringLiteral(
            "Drag to select text  ·  Shift+drag adds  ·  Click picks a word  ·  Enter copy  ·  Ctrl+A all  ·  Esc close"
        ),
        this
    );
    m_tipLabel->setStyleSheet(QStringLiteral(
        "QLabel { background: rgba(24,24,26,0.85); color: #dcdcdc; border: 1px solid rgba(255,255,255,0.12);"
        " border-radius: 8px; padding: 8px 18px; font-size: 12px; }"
    ));
}

QRectF OcrTextOverlay::cropWidgetRect() const {
    if (m_screenshot.isNull() || m_cropRect.isNull() || width() <= 0 || height() <= 0)
        return {};

    const qreal sx = static_cast<qreal>(width()) / m_screenshot.width();
    const qreal sy = static_cast<qreal>(height()) / m_screenshot.height();
    return QRectF(
        m_cropRect.left() * sx, m_cropRect.top() * sy, m_cropRect.width() * sx, m_cropRect.height() * sy
    );
}

QRect OcrTextOverlay::wordWidgetRect(int index) const {
    const Word &word = m_words.at(index);
    const qreal sx = static_cast<qreal>(width()) / m_screenshot.width();
    const qreal sy = static_cast<qreal>(height()) / m_screenshot.height();
    return QRect(
        qRound(word.rect.left() * sx),
        qRound(word.rect.top() * sy),
        qMax(1, qRound(word.rect.width() * sx)),
        qMax(1, qRound(word.rect.height() * sy))
    );
}

int OcrTextOverlay::wordAt(const QPoint &pos) const {
    for (int i = 0; i < m_words.size(); ++i) {
        if (wordWidgetRect(i).contains(pos))
            return i;
    }
    return -1;
}

void OcrTextOverlay::applySelection() {
    QSet<int> selected = m_pressedSelection;
    for (int i = 0; i < m_words.size(); ++i) {
        if (wordWidgetRect(i).intersects(m_selectionRect))
            selected.insert(i);
    }
    m_selected = selected;
    refreshStatus();
    update();
}

void OcrTextOverlay::selectAll() {
    QSet<int> selected;
    for (int i = 0; i < m_words.size(); ++i)
        selected.insert(i);
    m_selected = selected;
    refreshStatus();
    update();
}

void OcrTextOverlay::clearSelection() {
    m_selected.clear();
    refreshStatus();
    update();
}

void OcrTextOverlay::refreshStatus() {
    if (m_words.isEmpty())
        return;

    m_statusLabel->setText(
        m_selected.isEmpty()
            ? QStringLiteral("%1 words").arg(m_words.size())
            : QStringLiteral("%1 of %2 selected").arg(m_selected.size()).arg(m_words.size())
    );
    m_copyButton->setEnabled(!m_selected.isEmpty());
}

QString OcrTextOverlay::selectedText() const {
    QString text;
    int lastLine = -1;
    for (int i = 0; i < m_words.size(); ++i) {
        if (!m_selected.contains(i))
            continue;

        const Word &word = m_words.at(i);
        if (!text.isEmpty() && word.line != lastLine)
            text += QLatin1Char('\n');
        else if (!text.isEmpty())
            text += QLatin1Char(' ');
        text += word.text;
        lastLine = word.line;
    }
    return text;
}

void OcrTextOverlay::copySelection() {
    const QString text = selectedText();
    if (text.isEmpty())
        return;
    QGuiApplication::clipboard()->setText(text);
    m_statusLabel->setText(QStringLiteral("Copied"));
}

void OcrTextOverlay::setResult(const QVector<Word> &words) {
    m_words = words;
    m_selected.clear();
    m_hoverIndex = -1;
    m_copyButton->setEnabled(false);
    m_statusLabel->setText(
        m_words.isEmpty() ? QStringLiteral("No text found") : QStringLiteral("%1 words").arg(m_words.size())
    );
    placeToolbar();
    update();
}

void OcrTextOverlay::setFailed(const QString &message) {
    const QString text = QStringLiteral("OCR failed: %1").arg(message);
    m_statusLabel->setText(QFontMetrics(m_statusLabel->font()).elidedText(text, Qt::ElideMiddle, 170));
    m_copyButton->setEnabled(false);
    placeToolbar();
}

void OcrTextOverlay::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton)
        return;

    // shift starts an additive drag: words already selected stay selected
    m_pressedSelection = event->modifiers() & Qt::ShiftModifier ? m_selected : QSet<int>();
    m_isDragging = true;
    m_startPoint = event->position().toPoint();
    m_selectionRect = {};
    m_toolbar->hide();
    if (m_pressedSelection.isEmpty())
        clearSelection();
}

void OcrTextOverlay::mouseMoveEvent(QMouseEvent *event) {
    if (m_isDragging) {
        m_selectionRect = QRect(m_startPoint, event->position().toPoint()).normalized().intersected(rect());
        applySelection();
        return;
    }

    const int hover = wordAt(event->position().toPoint());
    if (hover != m_hoverIndex) {
        m_hoverIndex = hover;
        update();
    }
    if (!m_words.isEmpty())
        setCursor(hover >= 0 ? Qt::IBeamCursor : Qt::CrossCursor);
}

void OcrTextOverlay::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton || !m_isDragging)
        return;

    m_isDragging = false;
    m_selectionRect = QRect(m_startPoint, event->position().toPoint()).normalized().intersected(rect());

    if (m_selectionRect.width() < 6 && m_selectionRect.height() < 6) {
        m_selectionRect = {};
        const int word = wordAt(event->position().toPoint());
        if (word >= 0) {
            m_selected = m_pressedSelection + QSet<int>{word};
        } else if (m_pressedSelection.isEmpty()) {
            m_selected.clear();
        }
        refreshStatus();
        update();
    } else {
        applySelection();
    }

    placeToolbar();
}

void OcrTextOverlay::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    if (event->matches(QKeySequence::Copy) || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        copySelection();
        return;
    }
    if (event->matches(QKeySequence::SelectAll)) {
        selectAll();
        return;
    }
    QWidget::keyPressEvent(event);
}

void OcrTextOverlay::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)

    QPainter painter(this);
    painter.drawPixmap(rect(), m_screenshot);

    const QRect crop = cropWidgetRect().toAlignedRect().intersected(rect());
    if (!crop.isNull()) {
        painter.fillRect(QRect(0, 0, width(), crop.top()), kDim);
        painter.fillRect(QRect(0, crop.bottom() + 1, width(), height() - crop.bottom() - 1), kDim);
        painter.fillRect(QRect(0, crop.top(), crop.left(), crop.height()), kDim);
        painter.fillRect(QRect(crop.right() + 1, crop.top(), width() - crop.right() - 1, crop.height()), kDim);
    }

    for (int i = 0; i < m_words.size(); ++i) {
        const QRect wordRect = wordWidgetRect(i);
        if (!crop.contains(wordRect.center()))
            continue;
        if (m_selected.contains(i))
            painter.fillRect(wordRect, kSelected);
        else if (i == m_hoverIndex)
            painter.fillRect(wordRect, kHover);
    }

    if (m_isDragging && !m_selectionRect.isNull()) {
        painter.setPen(QPen(kAccent, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(QRectF(m_selectionRect).adjusted(1, 1, -1, -1));
    }
}

void OcrTextOverlay::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    positionTip();
    placeToolbar();
}

void OcrTextOverlay::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    positionTip();
    placeToolbar();
}

void OcrTextOverlay::placeToolbar() {
    m_toolbar->adjustSize();
    const QRectF crop = cropWidgetRect();

    int x = qRound(crop.center().x() - m_toolbar->width() / 2.0);
    int y = qRound(crop.isNull() ? height() - m_toolbar->height() - 20 : crop.bottom() + 10);

    x = qBound(0, x, width() - m_toolbar->width());
    y = qBound(0, y, height() - m_toolbar->height());

    m_toolbar->move(x, y);
    m_toolbar->show();
    m_toolbar->raise();
    m_tipLabel->raise();
}

void OcrTextOverlay::positionTip() {
    m_tipLabel->adjustSize();
    m_tipLabel->move((width() - m_tipLabel->width()) / 2, 26);
    m_tipLabel->raise();
}
