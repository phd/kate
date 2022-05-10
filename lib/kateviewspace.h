/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KATE_VIEWSPACE_H
#define KATE_VIEWSPACE_H

#include <ktexteditor/document.h>
#include <ktexteditor/modificationinterface.h>
#include <ktexteditor/view.h>

#include "kateprivate_export.h"
#include "katetabbar.h"

#include <QHash>
#include <QWidget>

class KConfigBase;
class KateViewManager;
class QStackedWidget;
class QToolButton;
class LocationHistoryTest;

class KATE_PRIVATE_EXPORT KateViewSpace : public QWidget
{
    Q_OBJECT

public:
    explicit KateViewSpace(KateViewManager *, QWidget *parent = nullptr, const char *name = nullptr);

    ~KateViewSpace();

    /**
     * Returns \e true, if this view space is currently the active view space.
     */
    bool isActiveSpace() const;

    /**
     * Depending on @p active, mark this view space as active or inactive.
     * Called from the view manager.
     */
    void setActive(bool active);

    /**
     * @return the view manager that this viewspace belongs to
     */
    KateViewManager *viewManger() const
    {
        return m_viewManager;
    }

    /**
     * Create new view for given document
     * @param doc document to create view for
     * @return new created view
     */
    KTextEditor::View *createView(KTextEditor::Document *doc);
    void removeView(KTextEditor::View *v);

    bool showView(KTextEditor::View *view)
    {
        return showView(view->document());
    }
    bool showView(KTextEditor::Document *document);

    // might be nullptr, if there is no view
    KTextEditor::View *currentView();

    void saveConfig(KConfigBase *config, int myIndex, const QString &viewConfGrp);
    void restoreConfig(KateViewManager *viewMan, const KConfigBase *config, const QString &group);

    /**
     * Returns the document list of this tab bar.
     * @return document list in order of tabs
     */
    QVector<KTextEditor::Document *> documentList() const
    {
        return m_tabBar->documentList();
    }

    /**
     * How many documents are registered here?
     */
    int numberOfRegisteredDocuments() const
    {
        return m_registeredDocuments.size();
    }

    /**
     * Register one document for this view space.
     * Each registered document will get e.g. a tab bar button.
     */
    void registerDocument(KTextEditor::Document *doc);

    /**
     * closes the view of the provided doc in this viewspace
     */
    void closeDocument(KTextEditor::Document *doc);

    /*
     * Does this viewspace contain @p doc
     */
    bool hasDocument(KTextEditor::Document *doc) const;

    /**
     * Removes @p doc from this space and returns the associated
     * view
     * Used for dnd
     */
    KTextEditor::View *takeView(KTextEditor::Document *doc);

    /**
     * Adds @p view to this space
     * Used for dnd to add a view from another viewspace
     */
    void addView(KTextEditor::View *v);

    /**
     * Event filter to catch events from view space tool buttons.
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

    /**
     * Focus the previous tab in the tabbar.
     */
    void focusPrevTab();

    /**
     * Focus the next tab in the tabbar.
     */
    void focusNextTab();

    /**
     * Add a non KTE-View as a tab
     */
    void addWidgetAsTab(QWidget *widget);

    /**
     * Does this viewspace contain any not KTextEditor::View widgets?
     */
    bool hasWidgets() const;

    /**
     * Does the current tab contains a widget
     * that is not a KTextEditor::View
     *
     * If the current active tab has a KTE::View
     * this will return nullptr
     */
    QWidget *currentWidget();

    void closeTabWithWidget(QWidget *widget);

    // BEGIN Location History Stuff

    /**
     * go forward in location history
     */
    void goForward();

    /**
     * go back in location history
     */
    void goBack();

    /**
     * Is back history avail?
     */
    bool isHistoryBackEnabled() const;

    /**
     * Is forward history avail?
     */
    bool isHistoryForwardEnabled() const;

    /**
     * Add a jump location for jumping back and forth between history
     */
    void addPositionToHistory(const QUrl &url, KTextEditor::Cursor, bool calledExternally = false);

    // END Location History Stuff

    void focusNavigationBar();

protected:
    // DND
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void dropEvent(QDropEvent *e) override;

Q_SIGNALS:
    void viewSpaceEmptied(KateViewSpace *vs);

public Q_SLOTS:
    void documentDestroyed(QObject *doc);
    void updateDocumentName(KTextEditor::Document *doc);
    void updateDocumentUrl(KTextEditor::Document *doc);
    void updateDocumentState(KTextEditor::Document *doc);

private Q_SLOTS:
    void tabBarToggled();
    void urlBarToggled(bool);
    void changeView(int buttonId);

    /**
     * Calls this slot to make this view space the currently active view space.
     * Making it active goes through the KateViewManager.
     * @param focusCurrentView if @e true, the current view will get focus
     */
    void makeActive(bool focusCurrentView = true);

    /**
     * This slot is called by the tabbar, if tab @p id was closed through the
     * context menu.
     */
    void closeTabRequest(int id);

    /**
     * This slot is called when the context menu is requested for button
     * @p id at position @p globalPos.
     * @param id the button, or -1 if the context menu was requested on
     *        at a place where no tab exists
     * @param globalPos the position of the context menu in global coordinates
     */
    void showContextMenu(int id, const QPoint &globalPos);

    /**
     * Called to create a new empty document.
     */
    void createNewDocument();

    /**
     * Read and apply the config for this view space.
     */
    void readConfig();

    /**
     * Document created or deleted, used to auto hide/show the tabs
     */
    void documentCreatedOrDeleted(KTextEditor::Document *);

private:
    bool acceptsDroppedTab(const class QMimeData *tabMimeData);
    /**
     * Returns the amount of documents in KateDocManager that currently
     * have no tab in this tab bar.
     */
    int hiddenDocuments() const;

    // The following functions are for unit-testing purposes
    size_t &currentLoc()
    {
        return currentLocation;
    }

    auto &locationHistoryBuffer()
    {
        return m_locations;
    }

private:
    // Kate's view manager
    KateViewManager *m_viewManager;

    // config group string, used for restoring View session configuration
    QString m_group;

    // flag that indicates whether this view space is the active one.
    // correct setter: m_viewManager->setActiveSpace(this);
    bool m_isActiveSpace;

    // widget stack that contains all KTE::Views
    QStackedWidget *stack;

    // jump location history for this view-space
    struct Location {
        QUrl url;
        KTextEditor::Cursor cursor;
    };

    std::vector<Location> m_locations;
    size_t currentLocation = 0;

    /**
     * all documents this view space is aware of
     * depending on the limit of tabs, not all will have a corresponding
     * tab in the KateTabBar
     * these are stored in used order (MRU last)
     */
    QVector<KTextEditor::Document *> m_registeredDocuments;

    // the list of views that are contained in this view space,
    // mapped through a hash from Document to View.
    // note: the number of entries match stack->count();
    std::unordered_map<KTextEditor::Document *, KTextEditor::View *> m_docToView;

    // tab bar that contains viewspace tabs
    KateTabBar *m_tabBar;

    // split action
    QToolButton *m_split;

    // quick open action
    QToolButton *m_quickOpen;

    // go back in history button (only visible when the tab bar is visible)
    QToolButton *m_historyBack;

    // go forward in history button (only visible when the tab bar is visible)
    QToolButton *m_historyForward;

    // rubber band to indicate drag and drop
    std::unique_ptr<class QRubberBand> m_dropIndicator;

    class KateUrlBar *m_urlBar = nullptr;

    struct TopBarLayout {
        class QHBoxLayout *tabBarLayout = nullptr;
        class QVBoxLayout *mainLayout = nullptr;
    } m_layout;

    friend class LocationHistoryTest;

    // should the tab bar be auto hidden if just one document is open?
    bool m_autoHideTabBar = false;
};

#endif
