#include "plugins/chat/lib/ChatOutput.hpp"

#include "plugins/chat/lib/LatexRenderer.hpp"

#include <QClipboard>
#include <QFontDatabase>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>

namespace {

const QString kBackground = QStringLiteral("#15161a");

const char *kMessageSheet =
    "QFrame#userMessage { background:#23262e; border:1px solid #363b47; border-radius:10px; }"
    "QFrame#modelMessage { background:transparent; border:none; }"
    "QLabel#attachment { color:#8b90a0; font-size:11px; background:transparent; }";

const char *kCodeSheet =
    "QFrame#codeBlock { background:#141519; border:1px solid #2f323b; border-radius:10px; }"
    "QFrame#codeHeader { background:#1c1e24; border-bottom:1px solid #2a2d35;"
    "                   border-top-left-radius:9px; border-top-right-radius:9px; }"
    "QLabel#codeLang { color:#8b90a0; font-size:11px; font-weight:600; background:transparent; }"
    "QLabel#codeText { color:#d4d7e0; background:transparent; }"
    "QPushButton#copyBtn { background:#282b33; color:#c3c7d2; border:none; border-radius:5px;"
    "                     padding:3px 11px; font-size:11px; }"
    "QPushButton#copyBtn:hover { background:#343843; color:#ffffff; }";

const char *kMarkdownSheet =
    "body { color:#e4e6ec; font-size:13px; }"
    "p { margin:0 0 0.55em 0; }"
    "p:first-child { margin-top:0; }"
    "p:last-child { margin-bottom:0; }"
    "h1,h2,h3,h4 { margin:0.7em 0 0.35em 0; font-weight:600; }"
    "h1 { font-size:17px; }"
    "h2 { font-size:15px; }"
    "h3,h4 { font-size:14px; }"
    "ul,ol { margin:0.35em 0 0.55em 1.15em; }"
    "li { margin:0.18em 0; }"
    "a { color:#7cb7ff; }"
    "code { color:#9fd0ff; background-color:#252830; }"
    "pre { color:#d4d7e0; background-color:#141519; }"
    "pre code { background-color:transparent; }"
    "blockquote { color:#a4a8b4; margin:0.45em 0; padding-left:10px; border-left:3px solid #35383f; }"
    "hr { color:#32343d; min-height:1px; }";

QString markdownToHtml(const QString &markdown) {
    QString source = markdown;
    source.replace(QRegularExpression(QStringLiteral("\n")), QStringLiteral("  \n"));

    QTextDocument doc;
    doc.setMarkdown(source);
    const QString html = doc.toHtml();

    const int bodyStart = html.indexOf(QStringLiteral("<body"));
    if (bodyStart < 0)
        return QString();

    const int start = html.indexOf('>', bodyStart);
    const int end = html.lastIndexOf(QStringLiteral("</body>"));
    if (start < 0 || end <= start)
        return QString();

    return html.mid(start + 1, end - start - 1).trimmed();
}

QString pendingStyle(bool isError) {
    return isError ? QStringLiteral(
                         "QLabel#pending { background:#3a1f22; border:1px solid #7f3038;"
                         " border-radius:12px; color:#ffb4b4; font-size:13px; padding:11px 14px; }"
                     )
                   : QStringLiteral(
                         "QLabel#pending { background:#22232a; border:1px solid #32343d;"
                         " border-radius:12px; color:#9a9eab; font-size:13px; padding:11px 14px; }"
                     );
}

}  // namespace

ChatOutput::ChatOutput(QWidget *parent) : QWidget(parent) {
    setStyleSheet("ChatOutput { background:#15161a; }");

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->viewport()->setStyleSheet("background:#15161a;");

    m_column = new QWidget;
    m_column->setStyleSheet("background:#15161a;");

    m_columnLayout = new QVBoxLayout(m_column);
    m_columnLayout->setContentsMargins(14, 14, 14, 14);
    m_columnLayout->setSpacing(12);
    m_columnLayout->addStretch();

    m_scroll->setWidget(m_column);
    outer->addWidget(m_scroll);
}

QVector<ChatOutput::Segment> ChatOutput::splitMarkdown(const QString &markdown) {
    static const QRegularExpression fence(
        QStringLiteral("^```[ \\t]*([^\\n`]*)\\n([\\s\\S]*?)^```[ \\t]*$"), QRegularExpression::MultilineOption
    );

    QVector<Segment> segments;
    int cursor = 0;

    for (auto it = fence.globalMatch(markdown); it.hasNext();) {
        const QRegularExpressionMatch match = it.next();
        if (match.capturedStart() > cursor) {
            segments.append({QString(), markdown.mid(cursor, match.capturedStart() - cursor).trimmed(), false});
        }
        segments.append({match.captured(1).trimmed(), match.captured(2).trimmed(), true});
        cursor = match.capturedEnd();
    }

    const QString tail = markdown.mid(cursor).trimmed();

    // an unterminated fence (truncated response) still deserves a code block
    const int open = tail.indexOf(QStringLiteral("```"));
    if (open < 0) {
        if (!tail.isEmpty())
            segments.append({QString(), tail, false});
    } else {
        const QString head = tail.left(open).trimmed();
        if (!head.isEmpty())
            segments.append({QString(), head, false});

        const QString rest = tail.mid(open + 3);
        const int newline = rest.indexOf('\n');
        segments.append(
            {newline < 0 ? QString() : rest.left(newline).trimmed(), newline < 0 ? rest : rest.mid(newline + 1), true}
        );
    }

    segments.removeIf([](const Segment &s) { return !s.isCode && s.body.isEmpty(); });
    return segments;
}

QVector<ChatOutput::Segment> ChatOutput::splitDisplayMath(const QString &text) {
    static const QRegularExpression display(
        QStringLiteral("(?<!\\$)\\$\\$(.+?)\\$\\$"), QRegularExpression::DotMatchesEverythingOption
    );

    QVector<Segment> segments;
    int cursor = 0;

    for (auto it = display.globalMatch(text); it.hasNext();) {
        const QRegularExpressionMatch match = it.next();
        if (match.capturedStart() > cursor) {
            segments.append({QString(), text.mid(cursor, match.capturedStart() - cursor).trimmed(), false, false});
        }
        segments.append({QString(), match.captured(1).trimmed(), false, true});
        cursor = match.capturedEnd();
    }

    const QString tail = text.mid(cursor).trimmed();
    if (!tail.isEmpty())
        segments.append({QString(), tail, false, false});

    return segments;
}
ChatOutput::Bubble ChatOutput::createBubble(bool isUser) {
    auto *frame = new QFrame;
    frame->setObjectName(isUser ? QStringLiteral("userMessage") : QStringLiteral("modelMessage"));
    frame->setStyleSheet(QString::fromLatin1(kMessageSheet));

    auto *content = new QVBoxLayout(frame);
    content->setContentsMargins(isUser ? 12 : 0, isUser ? 9 : 0, isUser ? 12 : 0, isUser ? 10 : 0);
    content->setSpacing(7);

    auto *row = new QWidget;
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(0);

    if (isUser) {
        rowLayout->addStretch();
        rowLayout->addWidget(frame, 1, Qt::AlignTop);
    } else {
        // model output runs the full width of the panel
        rowLayout->addWidget(frame, 1, Qt::AlignTop);
    }

    return {row, frame, content};
}

void ChatOutput::appendBlock(QVBoxLayout *content, const Segment &segment) {
    content->addWidget(
        segment.isCode ? createCodeBlock(segment.language, segment.body) : createTextBlock(segment.body)
    );
}

QWidget *ChatOutput::createMathBlock(const QString &source) {
    auto *card = new QFrame;
    card->setObjectName(QStringLiteral("mathBlock"));
    card->setStyleSheet("QFrame#mathBlock { background:#141519; border:1px solid #2f323b; border-radius:10px; }");

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 12, 14, 12);

    auto *label = new QLabel(card);
    label->setWordWrap(true);
    label->setTextFormat(Qt::RichText);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("QLabel { background:transparent; color:#dfe2ea; font-size:15px; }");
    label->setText(latex::renderToHtml(source, true));

    layout->addWidget(label);
    return card;
}

QWidget *ChatOutput::createTextBlock(const QString &markdown) {
    // QLabel implements heightForWidth, so the bubble hugs short text and wraps
    // long text at the bubble's clamped width
    auto *label = new QLabel;
    label->setWordWrap(true);
    label->setTextFormat(Qt::RichText);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    label->setOpenExternalLinks(true);
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    // QLabel paints a palette background unless told otherwise
    label->setStyleSheet("QLabel { background:transparent; }");
    label->setText(QStringLiteral("<html><head><style>%1</style></head><body>%2</body></html>")
                       .arg(QString::fromLatin1(kMarkdownSheet), latex::renderMathInHtml(markdownToHtml(markdown))));
    return label;
}

QWidget *ChatOutput::createCodeBlock(const QString &language, const QString &code) {
    auto *block = new QFrame;
    block->setObjectName(QStringLiteral("codeBlock"));
    block->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    block->setStyleSheet(QString::fromLatin1(kCodeSheet));

    auto *layout = new QVBoxLayout(block);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new QFrame(block);
    header->setObjectName(QStringLiteral("codeHeader"));

    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(10, 5, 6, 5);
    headerLayout->setSpacing(8);

    auto *langLabel = new QLabel(language.isEmpty() ? QStringLiteral("code") : language, header);
    langLabel->setObjectName(QStringLiteral("codeLang"));
    headerLayout->addWidget(langLabel);
    headerLayout->addStretch();

    auto *copyBtn = new QPushButton(QStringLiteral("Copy"), header);
    copyBtn->setObjectName(QStringLiteral("copyBtn"));
    copyBtn->setCursor(Qt::PointingHandCursor);
    copyBtn->setFocusPolicy(Qt::NoFocus);
    headerLayout->addWidget(copyBtn);

    QObject::connect(copyBtn, &QPushButton::clicked, this, [copyBtn, code]() {
        QGuiApplication::clipboard()->setText(code);
        copyBtn->setText(QStringLiteral("Copied"));
        QTimer::singleShot(1200, copyBtn, [copyBtn]() { copyBtn->setText(QStringLiteral("Copy")); });
    });

    layout->addWidget(header);

    QFont fixed = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    fixed.setPointSize(10);

    auto *codeLabel = new QLabel(code, block);
    codeLabel->setObjectName(QStringLiteral("codeText"));
    codeLabel->setFont(fixed);
    codeLabel->setTextFormat(Qt::PlainText);
    codeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    codeLabel->setWordWrap(true);
    codeLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    codeLabel->setContentsMargins(11, 9, 11, 10);

    layout->addWidget(codeLabel);
    return block;
}

QWidget *ChatOutput::createPendingBubble(bool isError) {
    auto *bubble = new QWidget;
    auto *row = new QHBoxLayout(bubble);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    if (!isError)
        row->addStretch();

    m_pendingLabel = new QLabel;
    m_pendingLabel->setObjectName(QStringLiteral("pending"));
    m_pendingLabel->setWordWrap(true);
    m_pendingLabel->setTextFormat(Qt::PlainText);
    m_pendingLabel->setStyleSheet(pendingStyle(isError));
    row->addWidget(m_pendingLabel, 0, Qt::AlignTop);

    if (isError)
        row->addStretch();

    return bubble;
}

void ChatOutput::addMessage(const QString &role, const QString &text, bool hasImage) {
    const bool isUser = (role != QStringLiteral("model"));
    const Bubble bubble = createBubble(isUser);

    if (isUser && hasImage) {
        auto *attachment = new QLabel(QStringLiteral("Screenshot attached"), bubble.frame);
        attachment->setObjectName(QStringLiteral("attachment"));
        bubble.content->addWidget(attachment);
    }

    // fenced code first, then pull $$...$$ out of the remaining prose
    for (const Segment &segment : splitMarkdown(text)) {
        if (segment.isCode) {
            bubble.content->addWidget(createCodeBlock(segment.language, segment.body));
            continue;
        }

        for (const Segment &block : splitDisplayMath(segment.body)) {
            if (block.isMath)
                bubble.content->addWidget(createMathBlock(block.body));
            else
                bubble.content->addWidget(createTextBlock(block.body));
        }
    }

    m_columnLayout->insertWidget(m_columnLayout->count() - 1, bubble.row);
    if (isUser)
        m_userBubbles.append(bubble.frame);

    applyMaxBubbleWidth();
    scrollToBottom();
}

void ChatOutput::destroyPending() {
    if (!m_pendingBubble)
        return;

    m_pendingBubble->deleteLater();
    m_pendingBubble = nullptr;
    m_pendingLabel = nullptr;
}

void ChatOutput::setPending(const QString &text, bool isError) {
    if (text.isEmpty()) {
        destroyPending();
        return;
    }

    // a request either loads or errors, never both, but the style differs
    if (m_pendingBubble && isError != m_pendingIsError)
        destroyPending();

    if (!m_pendingBubble) {
        m_pendingIsError = isError;
        m_pendingBubble = createPendingBubble(isError);
        m_columnLayout->insertWidget(m_columnLayout->count() - 1, m_pendingBubble);
    }

    m_pendingLabel->setText(text);
    applyMaxBubbleWidth();
    scrollToBottom();
}

void ChatOutput::clear() {
    for (QWidget *bubble : m_userBubbles)
        bubble->deleteLater();

    m_userBubbles.clear();
    m_pendingBubble = nullptr;
    m_pendingLabel = nullptr;
}

void ChatOutput::scrollToBottom() {
    QScrollBar *bar = m_scroll->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void ChatOutput::applyMaxBubbleWidth() {
    if (!m_scroll)
        return;

    const int maxWidth = int(m_scroll->viewport()->width() * 0.72);
    for (QWidget *bubble : m_userBubbles)
        bubble->setMaximumWidth(maxWidth);
}
void ChatOutput::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    // the viewport is only full width once the scroll area has been laid out
    applyMaxBubbleWidth();
}

void ChatOutput::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    applyMaxBubbleWidth();
}
