#pragma once

#include "plugins/misc/lib/ToolBase.hpp"

#include <QLabel>
#include <QNetworkAccessManager>
#include <QString>
#include <QVector>
#include <QWidget>

class QScrollArea;
class QPushButton;

class AnimeWidget final : public QWidget, public ToolBase {
    Q_OBJECT

public:
    explicit AnimeWidget(const QString &pluginRootPath, QWidget *parent = nullptr);

    QString label() const override { return QStringLiteral("Anime"); }
    void start(QuolServices *services) override;
    void stop() override;
    QWidget *widget() override;

signals:
    void closed();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    enum class ViewMode { Grid, Episodes };

    struct AnimeEntry {
        QString title;
        QString imageUrl;
        QString episode;
        QString watchUrl;
    };

    struct EpisodeEntry {
        int number = 0;
        QString title;
        QString fullUrl;
    };

    void fetchPage(int page);
    void onPageFetched(QNetworkReply *reply);
    void buildGrid();
    void loadImages();
    void onImageFetched(QNetworkReply *reply, int index, int generation);
    void updateNavButtons();
    void cancelPendingRequests();
    QString buildUrl(int page) const;

    void onCardClicked(int index);
    void fetchEpisodes(const QString &url);
    void onWatchPageFetched(QNetworkReply *reply);
    void onEpisodeListFetched(QNetworkReply *reply);
    void buildEpisodeList();
    void showEpisodeError(const QString &message);
    void showGrid();
    void updateNavVisibility();

    QString m_rootPath;
    QNetworkAccessManager *m_netManager = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_container = nullptr;
    QWidget *m_contentWidget = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_prevBtn = nullptr;
    QPushButton *m_retryBtn = nullptr;
    QPushButton *m_backBtn = nullptr;
    QPushButton *m_nextBtn = nullptr;
    QLabel *m_pageLabel = nullptr;

    QVector<AnimeEntry> m_entries;
    QVector<QLabel *> m_imageLabels;
    QVector<QNetworkReply *> m_pendingReplies;

    int m_currentPage = 1;
    int m_maxPage = 1;
    bool m_fetching = false;
    int m_generation = 0;

    ViewMode m_viewMode = ViewMode::Grid;
    QVector<EpisodeEntry> m_episodes;
    QString m_selectedAnimeTitle;

    QPoint m_dragOffset;
    bool m_dragging = false;
};
