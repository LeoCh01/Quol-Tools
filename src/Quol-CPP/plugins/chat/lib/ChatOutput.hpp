#pragma once

#include <QString>
#include <QVector>
#include <QWidget>

class QLabel;
class QScrollArea;
class QVBoxLayout;

class ChatOutput : public QWidget {
    Q_OBJECT

public:
    explicit ChatOutput(QWidget *parent = nullptr);

    void addMessage(const QString &role, const QString &text, bool hasImage = false);
    void setPending(const QString &text, bool isError = false);
    void clear();

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct Segment {
        QString language;
        QString body;
        bool isCode = false;
    };

    struct Bubble {
        QWidget *row = nullptr;    // wrapper aligned left/right in the scroll column
        QWidget *frame = nullptr;  // the bubble itself, and the max-width target
        QVBoxLayout *content = nullptr;
    };

    static QVector<Segment> splitMarkdown(const QString &markdown);

    Bubble createBubble(bool isUser);
    void appendBlock(QVBoxLayout *content, const Segment &segment);
    QWidget *createCodeBlock(const QString &language, const QString &code);
    QWidget *createTextBlock(const QString &markdown);
    QWidget *createPendingBubble(bool isError);

    void destroyPending();
    void scrollToBottom();
    void applyMaxBubbleWidth();

    QScrollArea *m_scroll = nullptr;
    QWidget *m_column = nullptr;
    QVBoxLayout *m_columnLayout = nullptr;

    QVector<QWidget *> m_userBubbles;
    QWidget *m_pendingBubble = nullptr;
    QLabel *m_pendingLabel = nullptr;
    bool m_pendingIsError = false;
};
