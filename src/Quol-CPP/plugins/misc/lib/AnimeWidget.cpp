#include "plugins/misc/lib/AnimeWidget.hpp"

#include <QCloseEvent>
#include <QDesktopServices>
#include <QEvent>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QPixmap>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QScreen>
#include <QUrl>
#include <QVBoxLayout>

namespace {

const QString kContainerStyle = QStringLiteral(
    "#animeContainer { background: rgba(18, 18, 18, 220); border-radius: 12px; }"
);

const QString kCardStyle = QStringLiteral(
    "#anime-card { background: rgba(30, 30, 30, 200); border-radius: 10px; }"
    "#anime-card:hover { background: rgba(50, 50, 50, 220); }"
);

const QString kTitleStyle = QStringLiteral(
    "color: #E0E0E0; font-size: 11px; font-weight: 500; "
    "font-family: 'Segoe UI', sans-serif; background: transparent;"
);

const QString kEpisodeStyle = QStringLiteral(
    "color: #4FC3F7; font-size: 10px; font-weight: bold; "
    "font-family: 'Consolas', monospace; background: transparent;"
);

const QString kNavBtnStyle = QStringLiteral(
    "QPushButton { background: rgba(255, 255, 255, 20); border: none; border-radius: 6px; "
    "  padding: 4px 10px; color: #E0E0E0; font-size: 12px; }"
    "QPushButton:hover { background: rgba(255, 255, 255, 40); }"
    "QPushButton:pressed { background: rgba(255, 255, 255, 55); }"
    "QPushButton:disabled { background: rgba(255, 255, 255, 8); color: rgba(255,255,255,50); }"
);

const QString kPageLabelStyle = QStringLiteral(
    "color: rgba(255, 255, 255, 150); font-size: 11px; "
    "font-family: 'Consolas', monospace; background: transparent;"
);

const QString kStatusLabelStyle = QStringLiteral(
    "color: rgba(255, 255, 255, 100); font-size: 12px; "
    "font-family: 'Segoe UI', sans-serif; background: transparent;"
);

const QString kEpisodeBtnStyle = QStringLiteral(
    "QPushButton { background: rgba(255, 255, 255, 15); border: none; border-radius: 8px; "
    "  padding: 10px 14px; color: #E0E0E0; font-size: 12px; text-align: left; "
    "  font-family: 'Segoe UI', sans-serif; }"
    "QPushButton:hover { background: rgba(79, 195, 247, 40); }"
    "QPushButton:pressed { background: rgba(79, 195, 247, 60); }"
);

const QString kBackBtnStyle = QStringLiteral(
    "QPushButton { background: rgba(255, 255, 255, 20); border: none; border-radius: 6px; "
    "  padding: 6px 14px; color: #E0E0E0; font-size: 12px; "
    "  font-family: 'Segoe UI', sans-serif; }"
    "QPushButton:hover { background: rgba(255, 255, 255, 40); }"
    "QPushButton:pressed { background: rgba(255, 255, 255, 55); }"
);

const QString kEpisodeTitleStyle = QStringLiteral(
    "color: rgba(255, 255, 255, 150); font-size: 13px; font-weight: 600; "
    "font-family: 'Segoe UI', sans-serif; background: transparent; padding: 4px 0;"
);

const int kCardWidth = 140;
const int kPosterHeight = 195;
const int kColumns = 4;
const int kSpacing = 8;

}  // namespace

AnimeWidget::AnimeWidget(const QString &pluginRootPath, QWidget *parent)
    : QWidget(parent), m_rootPath(pluginRootPath) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(480, 350);
    resize(620, 520);

    m_netManager = new QNetworkAccessManager(this);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    m_container = new QWidget(this);
    m_container->setObjectName(QStringLiteral("animeContainer"));
    m_container->setStyleSheet(kContainerStyle);

    auto *containerLayout = new QVBoxLayout(m_container);
    containerLayout->setContentsMargins(8, 8, 8, 8);
    containerLayout->setSpacing(6);

    // Nav bar
    auto *navBar = new QHBoxLayout();
    navBar->setContentsMargins(4, 0, 4, 0);
    navBar->setSpacing(6);

    m_prevBtn = new QPushButton(m_container);
    m_prevBtn->setIcon(QIcon(m_rootPath + QStringLiteral("/res/anime/img/prev.svg")));
    m_prevBtn->setIconSize(QSize(14, 14));
    m_prevBtn->setFixedSize(30, 26);
    m_prevBtn->setToolTip(QStringLiteral("Previous page"));
    m_prevBtn->setStyleSheet(kNavBtnStyle);
    m_prevBtn->setEnabled(false);
    connect(m_prevBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentPage > 1 && !m_fetching) {
            m_currentPage--;
            fetchPage(m_currentPage);
        }
    });
    navBar->addWidget(m_prevBtn);

    m_pageLabel = new QLabel(m_container);
    m_pageLabel->setStyleSheet(kPageLabelStyle);
    m_pageLabel->setAlignment(Qt::AlignCenter);
    m_pageLabel->setText(QStringLiteral("1 / 1"));
    navBar->addWidget(m_pageLabel, 1);

    m_nextBtn = new QPushButton(m_container);
    m_nextBtn->setIcon(QIcon(m_rootPath + QStringLiteral("/res/anime/img/next.svg")));
    m_nextBtn->setIconSize(QSize(14, 14));
    m_nextBtn->setFixedSize(30, 26);
    m_nextBtn->setToolTip(QStringLiteral("Next page"));
    m_nextBtn->setStyleSheet(kNavBtnStyle);
    m_nextBtn->setEnabled(false);
    connect(m_nextBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentPage < m_maxPage && !m_fetching) {
            m_currentPage++;
            fetchPage(m_currentPage);
        }
    });
    navBar->addWidget(m_nextBtn);

    m_retryBtn = new QPushButton(m_container);
    m_retryBtn->setIcon(QIcon(m_rootPath + QStringLiteral("/res/anime/img/retry.svg")));
    m_retryBtn->setIconSize(QSize(14, 14));
    m_retryBtn->setFixedSize(30, 26);
    m_retryBtn->setToolTip(QStringLiteral("Refresh"));
    m_retryBtn->setStyleSheet(kNavBtnStyle);
    connect(m_retryBtn, &QPushButton::clicked, this, [this]() {
        if (!m_fetching)
            fetchPage(m_currentPage);
    });
    navBar->addWidget(m_retryBtn);

    m_backBtn = new QPushButton(QStringLiteral("\u2190 Back"), m_container);
    m_backBtn->setStyleSheet(kBackBtnStyle);
    m_backBtn->setFixedHeight(26);
    m_backBtn->setVisible(false);
    connect(m_backBtn, &QPushButton::clicked, this, [this]() {
        if (m_viewMode == ViewMode::Episodes)
            showGrid();
    });
    navBar->addWidget(m_backBtn);

    containerLayout->addLayout(navBar);

    // Scroll area
    m_scrollArea = new QScrollArea(m_container);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { width: 6px; background: transparent; }"
        "QScrollBar::handle:vertical { background: rgba(255,255,255,40); border-radius: 3px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: rgba(255,255,255,70); }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    ));

    // Initial content with status label
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet(QStringLiteral("background: transparent;"));
    m_statusLabel = new QLabel(QStringLiteral("Loading..."), m_contentWidget);
    m_statusLabel->setStyleSheet(kStatusLabelStyle);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    auto *initLayout = new QVBoxLayout(m_contentWidget);
    initLayout->addWidget(m_statusLabel);
    m_scrollArea->setWidget(m_contentWidget);

    containerLayout->addWidget(m_scrollArea, 1);
    outer->addWidget(m_container);
}

QWidget *AnimeWidget::widget() {
    return this;
}

void AnimeWidget::start(QuolServices *services) {
    Q_UNUSED(services);
    if (QGuiApplication::primaryScreen()) {
        QRect g = QGuiApplication::primaryScreen()->availableGeometry();
        move(g.center().x() - width() / 2, g.center().y() - height() / 2);
    }
    show();
    raise();
    m_currentPage = 1;
    fetchPage(1);
}

void AnimeWidget::stop() {
    cancelPendingRequests();
    m_entries.clear();
    m_imageLabels.clear();
    m_episodes.clear();
    m_fetching = false;
    m_viewMode = ViewMode::Grid;
    m_selectedAnimeTitle.clear();
    hide();
}

void AnimeWidget::cancelPendingRequests() {
    for (QNetworkReply *r : std::as_const(m_pendingReplies)) {
        r->disconnect();
        r->abort();
        r->deleteLater();
    }
    m_pendingReplies.clear();
}

QString AnimeWidget::buildUrl(int page) const {
    if (page <= 1)
        return QStringLiteral("https://hianime.at/recently-updated");
    return QStringLiteral("https://hianime.at/recently-updated?page=%1").arg(page);
}

void AnimeWidget::fetchPage(int page) {
    cancelPendingRequests();
    m_generation++;
    m_fetching = true;
    updateNavButtons();
    m_entries.clear();
    m_imageLabels.clear();

    QNetworkRequest request(QUrl(buildUrl(page)));
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    request.setRawHeader("Accept", "text/html,application/xhtml+xml");
    request.setRawHeader("Accept-Language", "en-US,en;q=0.9");
    QNetworkReply *reply = m_netManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onPageFetched(reply); });
}

void AnimeWidget::onPageFetched(QNetworkReply *reply) {
    m_fetching = false;

    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        buildGrid();
        m_statusLabel->setText(QStringLiteral("Failed to load. Tap retry."));
        m_statusLabel->setVisible(true);
        updateNavButtons();
        return;
    }

    QByteArray html = reply->readAll();
    reply->deleteLater();

    // Extract the film_list-wrap section
    int wrapStart = html.indexOf("film_list-wrap");
    if (wrapStart == -1) {
        buildGrid();
        m_statusLabel->setText(QStringLiteral("No content found. Tap retry."));
        m_statusLabel->setVisible(true);
        updateNavButtons();
        return;
    }

    // Find the closing div for film_list-wrap (count nested divs)
    QByteArray wrapSection;
    {
        int searchFrom = html.indexOf('>', wrapStart) + 1;
        int depth = 1;
        int pos = searchFrom;
        while (depth > 0 && pos < html.size()) {
            int nextOpen = html.indexOf("<div", pos);
            int nextClose = html.indexOf("</div>", pos);
            if (nextClose == -1) break;
            if (nextOpen != -1 && nextOpen < nextClose) {
                depth++;
                pos = nextOpen + 4;
            } else {
                depth--;
                if (depth == 0) {
                    wrapSection = html.mid(searchFrom, nextClose - searchFrom);
                }
                pos = nextClose + 6;
            }
        }
    }

    if (wrapSection.isEmpty()) {
        buildGrid();
        m_statusLabel->setText(QStringLiteral("Parse error. Tap retry."));
        m_statusLabel->setVisible(true);
        updateNavButtons();
        return;
    }

    QString wrapStr = QString::fromUtf8(wrapSection);

    // Parse max page from pagination
    QRegularExpression lastPageRx(QStringLiteral(R"rx(recently-updated\?page=(\d+))rx"));
    auto lastMatches = lastPageRx.globalMatch(QString::fromUtf8(html));
    int detectedMax = 1;
    while (lastMatches.hasNext()) {
        auto m = lastMatches.next();
        int p = m.captured(1).toInt();
        if (p > detectedMax) detectedMax = p;
    }
    m_maxPage = detectedMax;

    // Parse anime entries from the film_list-wrap section
    QRegularExpression titleRx(
        QStringLiteral(R"rx(class="dynamic-name"[^>]*>([^<]+)</a>)rx")
    );
    QRegularExpression epsRx(
        QStringLiteral(R"rx(tick-eps">(\d+)</div>)rx")
    );
    QRegularExpression imgRx(
        QStringLiteral(R"rx(src="(https://cdn[^"]+\.(?:jpg|jpeg|png|webp)[^"]*)")rx")
    );
    QRegularExpression watchRx(
        QStringLiteral(R"rx(href="(https://hianime\.at/watch/[^"]+)"[^>]*class="film-poster-ahref)rx")
    );

    // Split by flw-item boundaries
    QRegularExpression itemRx(
        QStringLiteral(R"rx(<div class="flw-item[^"]*">)rx"),
        QRegularExpression::MultilineOption
    );

    auto items = itemRx.globalMatch(wrapStr);
    int lastPos = -1;
    QVector<QString> itemBlocks;

    while (items.hasNext()) {
        auto match = items.next();
        if (lastPos != -1) {
            itemBlocks.append(wrapStr.mid(lastPos, match.capturedStart() - lastPos));
        }
        lastPos = match.capturedStart();
    }
    if (lastPos != -1)
        itemBlocks.append(wrapStr.mid(lastPos));

    for (const auto &block : itemBlocks) {
        AnimeEntry entry;

        auto titleMatch = titleRx.match(block);
        if (titleMatch.hasMatch())
            entry.title = titleMatch.captured(1).trimmed();

        auto epsMatch = epsRx.match(block);
        if (epsMatch.hasMatch())
            entry.episode = QStringLiteral("Ep %1").arg(epsMatch.captured(1));

        auto imgMatch = imgRx.match(block);
        if (imgMatch.hasMatch())
            entry.imageUrl = imgMatch.captured(1);

        auto watchMatch = watchRx.match(block);
        if (watchMatch.hasMatch())
            entry.watchUrl = watchMatch.captured(1);

        if (!entry.title.isEmpty())
            m_entries.append(entry);
    }

    updateNavButtons();
    buildGrid();

    if (m_entries.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("No anime found. Tap retry."));
        m_statusLabel->setVisible(true);
    }

    loadImages();
}

void AnimeWidget::updateNavButtons() {
    m_prevBtn->setEnabled(m_currentPage > 1 && !m_fetching);
    m_nextBtn->setEnabled(m_currentPage < m_maxPage && !m_fetching);
    m_pageLabel->setText(QStringLiteral("%1 / %2").arg(m_currentPage).arg(m_maxPage));
}

void AnimeWidget::buildGrid() {
    m_imageLabels.clear();

    // Create new content widget. scrollArea->setWidget() deletes the old one.
    auto *newContent = new QWidget();
    newContent->setStyleSheet(QStringLiteral("background: transparent;"));

    auto *grid = new QGridLayout(newContent);
    grid->setContentsMargins(4, 4, 4, 4);
    grid->setSpacing(kSpacing);

    for (int i = 0; i < m_entries.size(); ++i) {
        const auto &entry = m_entries[i];

        auto *card = new QWidget();
        card->setObjectName(QStringLiteral("anime-card"));
        card->setStyleSheet(kCardStyle);
        card->setFixedSize(kCardWidth, kPosterHeight + 50);
        card->setCursor(Qt::PointingHandCursor);
        card->setProperty("animeIndex", i);
        card->installEventFilter(this);

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(0, 0, 0, 6);
        cardLayout->setSpacing(0);

        auto *poster = new QLabel(card);
        poster->setFixedSize(kCardWidth - 8, kPosterHeight);
        poster->setObjectName(QStringLiteral("poster"));
        poster->setAlignment(Qt::AlignCenter);
        poster->setStyleSheet(QStringLiteral(
            "background: rgba(255, 255, 255, 8); border-radius: 6px;"
        ));
        poster->setCursor(Qt::PointingHandCursor);
        poster->setProperty("animeIndex", i);
        poster->installEventFilter(this);
        cardLayout->addWidget(poster, 0, Qt::AlignHCenter);
        m_imageLabels.append(poster);

        auto *infoWidget = new QWidget(card);
        infoWidget->setStyleSheet(QStringLiteral("background: transparent;"));
        infoWidget->setCursor(Qt::PointingHandCursor);
        infoWidget->setProperty("animeIndex", i);
        infoWidget->installEventFilter(this);
        auto *infoLayout = new QVBoxLayout(infoWidget);
        infoLayout->setContentsMargins(6, 4, 6, 0);
        infoLayout->setSpacing(2);

        auto *titleLabel = new QLabel(entry.title, infoWidget);
        titleLabel->setStyleSheet(kTitleStyle);
        titleLabel->setFixedHeight(28);
        titleLabel->setWordWrap(true);
        titleLabel->setCursor(Qt::PointingHandCursor);
        titleLabel->setProperty("animeIndex", i);
        titleLabel->installEventFilter(this);
        infoLayout->addWidget(titleLabel);

        if (!entry.episode.isEmpty()) {
            auto *epLabel = new QLabel(entry.episode, infoWidget);
            epLabel->setStyleSheet(kEpisodeStyle);
            epLabel->setCursor(Qt::PointingHandCursor);
            epLabel->setProperty("animeIndex", i);
            epLabel->installEventFilter(this);
            infoLayout->addWidget(epLabel);
        }

        cardLayout->addWidget(infoWidget);

        int row = i / kColumns;
        int col = i % kColumns;
        grid->addWidget(card, row, col);
    }

    grid->setRowStretch(grid->rowCount(), 1);
    grid->setColumnStretch(kColumns, 1);

    // Add status label to the new content
    m_statusLabel = new QLabel(newContent);
    m_statusLabel->setStyleSheet(kStatusLabelStyle);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setVisible(false);
    grid->addWidget(m_statusLabel, m_entries.size() / kColumns + 1, 0, 1, kColumns);

    // setWidget() takes ownership and deletes the old widget
    m_scrollArea->setWidget(newContent);
    m_contentWidget = newContent;
}

void AnimeWidget::loadImages() {
    int gen = m_generation;
    for (int i = 0; i < m_entries.size(); ++i) {
        const auto &entry = m_entries[i];
        if (entry.imageUrl.isEmpty() || i >= m_imageLabels.size())
            continue;

        QNetworkRequest request(QUrl(entry.imageUrl));
        request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
        request.setRawHeader("Referer", "https://hianime.at/");
        QNetworkReply *reply = m_netManager->get(request);
        m_pendingReplies.append(reply);
        connect(reply, &QNetworkReply::finished, this, [this, reply, i, gen]() { onImageFetched(reply, i, gen); });
    }
}

void AnimeWidget::onImageFetched(QNetworkReply *reply, int index, int gen) {
    m_pendingReplies.removeAll(reply);

    if (gen != m_generation || reply->error() != QNetworkReply::NoError || index >= m_imageLabels.size()) {
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QPixmap pix;
    pix.loadFromData(data);
    if (pix.isNull())
        return;

    QLabel *label = m_imageLabels[index];
    int w = kCardWidth - 8;
    int h = kPosterHeight;
    label->setPixmap(pix.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void AnimeWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void AnimeWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragOffset);
        event->accept();
    }
}

bool AnimeWidget::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            auto *widget = qobject_cast<QWidget *>(obj);
            if (widget) {
                QVariant idx = widget->property("animeIndex");
                if (idx.isValid()) {
                    onCardClicked(idx.toInt());
                    return true;
                }
                QVariant epIdx = widget->property("epIndex");
                if (epIdx.isValid()) {
                    int i = epIdx.toInt();
                    if (i >= 0 && i < m_episodes.size()) {
                        QDesktopServices::openUrl(QUrl(m_episodes[i].fullUrl));
                        close();
                    }
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void AnimeWidget::onCardClicked(int index) {
    if (index < 0 || index >= m_entries.size())
        return;
    const auto &entry = m_entries[index];
    if (entry.watchUrl.isEmpty())
        return;

    m_viewMode = ViewMode::Episodes;
    m_selectedAnimeTitle = entry.title;
    m_episodes.clear();
    updateNavVisibility();

    // Replace grid with loading placeholder immediately
    auto *loadingWidget = new QWidget();
    loadingWidget->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *loadingLayout = new QVBoxLayout(loadingWidget);
    loadingLayout->setAlignment(Qt::AlignCenter);
    auto *loadingLabel = new QLabel(
        QStringLiteral("Loading episodes for %1...").arg(entry.title), loadingWidget);
    loadingLabel->setStyleSheet(kStatusLabelStyle);
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLayout->addWidget(loadingLabel);
    m_scrollArea->setWidget(loadingWidget);
    m_contentWidget = loadingWidget;
    m_statusLabel = loadingLabel;

    fetchEpisodes(entry.watchUrl);
}

void AnimeWidget::fetchEpisodes(const QString &url) {
    cancelPendingRequests();
    m_generation++;
    m_fetching = true;

    QNetworkRequest request(QUrl{url});
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    request.setRawHeader("Accept", "text/html,application/xhtml+xml");
    request.setRawHeader("Accept-Language", "en-US,en;q=0.9");
    QNetworkReply *reply = m_netManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onWatchPageFetched(reply); });
}

void AnimeWidget::onWatchPageFetched(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        m_fetching = false;
        showEpisodeError(QStringLiteral("Failed to load anime page."));
        return;
    }

    QByteArray html = reply->readAll();
    reply->deleteLater();

    // Extract data-anime-id from #ani_detail
    QRegularExpression idRx(QStringLiteral(R"rx(data-anime-id="(\d+)")rx"));
    auto idMatch = idRx.match(QString::fromUtf8(html));
    if (!idMatch.hasMatch()) {
        m_fetching = false;
        showEpisodeError(QStringLiteral("Could not find anime ID."));
        return;
    }

    QString animeId = idMatch.captured(1);

    // Fetch episode list from API
    QNetworkRequest apiRequest(QUrl{QStringLiteral("https://hianime.at/api/theme/episode/list/%1").arg(animeId)});
    apiRequest.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    apiRequest.setRawHeader("Accept", "application/json");
    apiRequest.setRawHeader("X-Requested-With", "XMLHttpRequest");
    apiRequest.setRawHeader("Referer", reply->url().toEncoded());
    QNetworkReply *apiReply = m_netManager->get(apiRequest);
    connect(apiReply, &QNetworkReply::finished, this, [this, apiReply]() { onEpisodeListFetched(apiReply); });
}

void AnimeWidget::onEpisodeListFetched(QNetworkReply *reply) {
    m_fetching = false;

    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        showEpisodeError(QStringLiteral("Failed to load episode list."));
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        showEpisodeError(QStringLiteral("Invalid API response."));
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.value("status").toBool()) {
        showEpisodeError(QStringLiteral("API returned error status."));
        return;
    }

    QString epHtml = obj.value("html").toString();
    if (epHtml.isEmpty()) {
        showEpisodeError(QStringLiteral("No episode HTML in response."));
        return;
    }

    // Parse episodes from the HTML
    QRegularExpression epRx(
        QStringLiteral(R"rx(data-number="(\d+)"[^>]*href="([^"]+\?ep=\d+)")rx")
    );
    QRegularExpression epTitleRx(
        QStringLiteral(R"rx(data-jname="([^"]*)")rx")
    );

    // Split by ep-item boundaries
    QRegularExpression itemRx(
        QStringLiteral(R"rx(<a\s[^>]*class="ssl-item ep-item[^"]*"[^>]*>)rx"),
        QRegularExpression::MultilineOption
    );

    auto items = itemRx.globalMatch(epHtml);
    int lastPos = -1;
    QVector<QString> epBlocks;

    while (items.hasNext()) {
        auto match = items.next();
        if (lastPos != -1)
            epBlocks.append(epHtml.mid(lastPos, match.capturedStart() - lastPos));
        lastPos = match.capturedStart();
    }
    if (lastPos != -1)
        epBlocks.append(epHtml.mid(lastPos));

    for (const auto &block : epBlocks) {
        auto numMatch = epRx.match(block);
        if (!numMatch.hasMatch())
            continue;

        EpisodeEntry ep;
        ep.number = numMatch.captured(1).toInt();
        ep.fullUrl = numMatch.captured(2);

        auto titleMatch = epTitleRx.match(block);
        if (titleMatch.hasMatch())
            ep.title = titleMatch.captured(1);
        if (ep.title.isEmpty())
            ep.title = QStringLiteral("Episode %1").arg(ep.number);

        m_episodes.append(ep);
    }

    if (m_episodes.isEmpty()) {
        showEpisodeError(QStringLiteral("No episodes parsed."));
        return;
    }

    // Sort newest first
    std::sort(m_episodes.begin(), m_episodes.end(),
              [](const EpisodeEntry &a, const EpisodeEntry &b) { return a.number > b.number; });

    // Cap at 100 episodes (latest)
    if (m_episodes.size() > 100)
        m_episodes.resize(100);

    buildEpisodeList();
}

void AnimeWidget::showEpisodeError(const QString &message) {
    auto *errWidget = new QWidget();
    errWidget->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *errLayout = new QVBoxLayout(errWidget);
    errLayout->setAlignment(Qt::AlignCenter);
    auto *errLabel = new QLabel(message, errWidget);
    errLabel->setStyleSheet(kStatusLabelStyle);
    errLabel->setAlignment(Qt::AlignCenter);
    errLayout->addWidget(errLabel);
    m_scrollArea->setWidget(errWidget);
    m_contentWidget = errWidget;
    m_statusLabel = errLabel;
}

void AnimeWidget::buildEpisodeList() {
    m_imageLabels.clear();

    auto *newContent = new QWidget();
    newContent->setStyleSheet(QStringLiteral("background: transparent;"));

    auto *layout = new QVBoxLayout(newContent);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(4);

    // Anime title label
    auto *titleLabel = new QLabel(m_selectedAnimeTitle, newContent);
    titleLabel->setStyleSheet(kEpisodeTitleStyle);
    titleLabel->setWordWrap(true);
    layout->addWidget(titleLabel);

    // Scroll area for episodes
    auto *epScroll = new QScrollArea(newContent);
    epScroll->setWidgetResizable(true);
    epScroll->setFrameShape(QFrame::NoFrame);
    epScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    epScroll->setStyleSheet(QStringLiteral(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { width: 6px; background: transparent; }"
        "QScrollBar::handle:vertical { background: rgba(255,255,255,40); border-radius: 3px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: rgba(255,255,255,70); }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    ));

    auto *epContainer = new QWidget();
    epContainer->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *epLayout = new QVBoxLayout(epContainer);
    epLayout->setContentsMargins(2, 2, 2, 2);
    epLayout->setSpacing(3);

    for (int i = 0; i < m_episodes.size(); ++i) {
        const auto &ep = m_episodes[i];
        bool hasCustomName = !ep.title.isEmpty() && ep.title != QStringLiteral("Episode %1").arg(ep.number);
        QString label = hasCustomName
            ? QStringLiteral("Ep %1 - %2").arg(ep.number).arg(ep.title)
            : QStringLiteral("Episode %1").arg(ep.number);
        auto *btn = new QPushButton(label, epContainer);
        btn->setStyleSheet(kEpisodeBtnStyle);
        btn->setFixedHeight(34);
        btn->setProperty("epIndex", i);
        btn->installEventFilter(this);
        epLayout->addWidget(btn);
    }

    epLayout->addStretch();
    epScroll->setWidget(epContainer);
    layout->addWidget(epScroll, 1);

    m_scrollArea->setWidget(newContent);
    m_contentWidget = newContent;
}

void AnimeWidget::showGrid() {
    m_viewMode = ViewMode::Grid;
    m_episodes.clear();
    m_selectedAnimeTitle.clear();
    updateNavVisibility();
    buildGrid();
    if (m_entries.isEmpty()) {
        m_statusLabel->setText(QStringLiteral("No anime found. Tap retry."));
        m_statusLabel->setVisible(true);
    }
    loadImages();
}

void AnimeWidget::updateNavVisibility() {
    bool isGrid = (m_viewMode == ViewMode::Grid);
    m_prevBtn->setVisible(isGrid);
    m_nextBtn->setVisible(isGrid);
    m_pageLabel->setVisible(isGrid);
    m_retryBtn->setVisible(isGrid);
    m_backBtn->setVisible(!isGrid);
    updateNavButtons();
}

void AnimeWidget::closeEvent(QCloseEvent *event) {
    stop();
    emit closed();
    QWidget::closeEvent(event);
}
