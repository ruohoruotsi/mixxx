#include "library/mixxxlibraryfeature.h"

#include <QtDebug>

#include "library/basetrackcache.h"
#include "library/dao/trackschema.h"
#include "library/dlghidden.h"
#include "library/dlgmissing.h"
#include "library/hiddentablemodel.h"
#include "library/library.h"
#include "library/librarytablemodel.h"
#include "library/missingtablemodel.h"
#include "library/parser.h"
#include "library/queryutil.h"
#include "library/trackcollection.h"
#include "library/trackcollectionmanager.h"
#include "library/treeitem.h"
#include "moc_mixxxlibraryfeature.cpp"
#include "sources/soundsourceproxy.h"
#include "util/dnd.h"
#include "widget/wlibrary.h"

MixxxLibraryFeature::MixxxLibraryFeature(Library* pLibrary,
                                         UserSettingsPointer pConfig)
        : LibraryFeature(pLibrary, pConfig),
          kMissingTitle(tr("Missing Tracks")),
          kHiddenTitle(tr("Hidden Tracks")),
          m_icon(":/images/library/ic_library_tracks.svg"),
          m_pTrackCollection(pLibrary->trackCollections()->internalCollection()),
          m_pLibraryTableModel(nullptr),
          m_pMissingView(nullptr),
          m_pHiddenView(nullptr) {
    QString idColumn = LIBRARYTABLE_ID;
    QStringList columns = {
            LIBRARYTABLE_ID,
            LIBRARYTABLE_PLAYED,
            LIBRARYTABLE_TIMESPLAYED,
            // has to be up here otherwise Played and TimesPlayed are not shown
            LIBRARYTABLE_ALBUMARTIST,
            LIBRARYTABLE_ALBUM,
            LIBRARYTABLE_ARTIST,
            LIBRARYTABLE_TITLE,
            LIBRARYTABLE_YEAR,
            LIBRARYTABLE_RATING,
            LIBRARYTABLE_GENRE,
            LIBRARYTABLE_COMPOSER,
            LIBRARYTABLE_GROUPING,
            LIBRARYTABLE_TRACKNUMBER,
            LIBRARYTABLE_KEY,
            LIBRARYTABLE_KEY_ID,
            LIBRARYTABLE_BPM,
            LIBRARYTABLE_BPM_LOCK,
            LIBRARYTABLE_DURATION,
            LIBRARYTABLE_BITRATE,
            LIBRARYTABLE_REPLAYGAIN,
            LIBRARYTABLE_FILETYPE,
            LIBRARYTABLE_DATETIMEADDED,
            TRACKLOCATIONSTABLE_LOCATION,
            TRACKLOCATIONSTABLE_FSDELETED,
            LIBRARYTABLE_COMMENT,
            LIBRARYTABLE_MIXXXDELETED,
            LIBRARYTABLE_COLOR,
            LIBRARYTABLE_COVERART_SOURCE,
            LIBRARYTABLE_COVERART_TYPE,
            LIBRARYTABLE_COVERART_LOCATION,
            LIBRARYTABLE_COVERART_HASH};
    QStringList searchColumns = {
            LIBRARYTABLE_ARTIST,
            LIBRARYTABLE_ALBUM,
            LIBRARYTABLE_ALBUMARTIST,
            TRACKLOCATIONSTABLE_LOCATION,
            LIBRARYTABLE_GROUPING,
            LIBRARYTABLE_COMMENT,
            LIBRARYTABLE_TITLE,
            LIBRARYTABLE_GENRE,
            LIBRARYTABLE_CRATE};

    QStringList qualifiedTableColumns;
    for (const auto& col : columns) {
        qualifiedTableColumns.append(mixxx::trackschema::tableForColumn(col) +
                QLatin1Char('.') + col);
    }

    QSqlQuery query(m_pTrackCollection->database());
    QString tableName = "library_cache_view";
    QString queryString = QString(
            "CREATE TEMPORARY VIEW IF NOT EXISTS %1 AS "
            "SELECT %2 FROM library "
            "INNER JOIN track_locations ON library.location = track_locations.id")
                                  .arg(tableName, qualifiedTableColumns.join(","));
    query.prepare(queryString);
    if (!query.exec()) {
        LOG_FAILED_QUERY(query);
    }

    BaseTrackCache* pBaseTrackCache = new BaseTrackCache(m_pTrackCollection,
            std::move(tableName),
            std::move(idColumn),
            std::move(columns),
            std::move(searchColumns),
            true);
    m_pBaseTrackCache = QSharedPointer<BaseTrackCache>(pBaseTrackCache);
    m_pTrackCollection->connectTrackSource(m_pBaseTrackCache);

    // These rely on the 'default' track source being present.
    m_pLibraryTableModel = new LibraryTableModel(this, pLibrary->trackCollections(), "mixxx.db.model.library");

    std::unique_ptr<TreeItem> pRootItem = TreeItem::newRoot(this);
    pRootItem->appendChild(kMissingTitle);
    pRootItem->appendChild(kHiddenTitle);

    m_childModel.setRootItem(std::move(pRootItem));
}

void MixxxLibraryFeature::bindLibraryWidget(WLibrary* pLibraryWidget,
                                     KeyboardEventFilter* pKeyboard) {
    m_pHiddenView = new DlgHidden(pLibraryWidget, m_pConfig, m_pLibrary,
                                  pKeyboard);
    pLibraryWidget->registerView(kHiddenTitle, m_pHiddenView);
    connect(m_pHiddenView,
            &DlgHidden::trackSelected,
            this,
            &MixxxLibraryFeature::trackSelected);

    m_pMissingView = new DlgMissing(pLibraryWidget, m_pConfig, m_pLibrary,
                                    pKeyboard);
    pLibraryWidget->registerView(kMissingTitle, m_pMissingView);
    connect(m_pMissingView,
            &DlgMissing::trackSelected,
            this,
            &MixxxLibraryFeature::trackSelected);
}

QVariant MixxxLibraryFeature::title() {
    return tr("Tracks");
}

QIcon MixxxLibraryFeature::getIcon() {
    return m_icon;
}

TreeItemModel* MixxxLibraryFeature::getChildModel() {
    return &m_childModel;
}

void MixxxLibraryFeature::refreshLibraryModels() {
    if (m_pLibraryTableModel) {
        m_pLibraryTableModel->select();
    }
    if (m_pMissingView) {
        m_pMissingView->onShow();
    }
    if (m_pHiddenView) {
        m_pHiddenView->onShow();
    }
}

void MixxxLibraryFeature::activate() {
    qDebug() << "MixxxLibraryFeature::activate()";
    emit showTrackModel(m_pLibraryTableModel);
    emit enableCoverArtDisplay(true);
}

void MixxxLibraryFeature::activateChild(const QModelIndex& index) {
    QString itemName = index.data().toString();
    emit switchToView(itemName);
    if (m_pMissingView && itemName == kMissingTitle) {
        emit restoreSearch(m_pMissingView->currentSearch());
    } else if (m_pHiddenView && itemName == kHiddenTitle) {
        emit restoreSearch(m_pHiddenView->currentSearch());
    }
    emit enableCoverArtDisplay(true);
}

bool MixxxLibraryFeature::dropAccept(const QList<QUrl>& urls, QObject* pSource) {
    if (pSource) {
        return false;
    } else {
        QList<TrackId> trackIds = m_pTrackCollection->resolveTrackIdsFromUrls(
                urls, true);
        return trackIds.size() > 0;
    }
}

bool MixxxLibraryFeature::dragMoveAccept(const QUrl& url) {
    return SoundSourceProxy::isUrlSupported(url) ||
            Parser::isPlaylistFilenameSupported(url.toLocalFile());
}
