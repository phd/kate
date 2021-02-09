/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "branchesdialog.h"
#include "branchesdialogmodel.h"
#include "gitutils.h"

#include <QCoreApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrentRun>

#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/View>

#include <KLocalizedString>

#include <kfts_fuzzy_match.h>

class BranchFilterModel : public QSortFilterProxyModel
{
public:
    BranchFilterModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

    Q_SLOT void setFilterString(const QString &string)
    {
        beginResetModel();
        m_pattern = string;
        endResetModel();
    }

protected:
    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override
    {
        if (m_pattern.isEmpty()) {
            const int l = sourceLeft.data(BranchesDialogModel::OriginalSorting).toInt();
            const int r = sourceRight.data(BranchesDialogModel::OriginalSorting).toInt();
            return l > r;
        }
        const int l = sourceLeft.data(BranchesDialogModel::FuzzyScore).toInt();
        const int r = sourceRight.data(BranchesDialogModel::FuzzyScore).toInt();
        return l < r;
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        if (m_pattern.isEmpty()) {
            return true;
        }

        int score = 0;
        const auto idx = sourceModel()->index(sourceRow, 0, sourceParent);
        const QString string = idx.data().toString();
        const bool res = kfts::fuzzy_match(m_pattern, string, score);
        sourceModel()->setData(idx, score, BranchesDialogModel::FuzzyScore);
        return res;
    }

private:
    QString m_pattern;
};

class StyleDelegate : public QStyledItemDelegate
{
public:
    StyleDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem options = option;
        initStyleOption(&options, index);

        QTextDocument doc;

        auto name = index.data().toString();

        const QString nameColor = option.palette.color(QPalette::Link).name();
        kfts::to_scored_fuzzy_matched_display_string(m_filterString, name, QStringLiteral("<b style=\"color:%1;\">").arg(nameColor), QStringLiteral("</b>"));

        auto refType = (GitUtils::RefType)index.data(BranchesDialogModel::RefType).toInt();
        auto itemType = (BranchesDialogModel::ItemType)index.data(BranchesDialogModel::ItemTypeRole).toInt();
        using RefType = GitUtils::RefType;
        const auto fontSz = option.font.pointSize();
        name = QStringLiteral("<span style=\"font-size:%1pt;\">").arg(fontSz) + name + QStringLiteral("</span>");
        if (itemType == BranchesDialogModel::BranchItem && refType == RefType::Head) {
            name.append(QStringLiteral(" &nbsp;<span style=\"color:gray; font-size:%1pt;\">local</span>").arg(fontSz - 1));
        } else if (itemType == BranchesDialogModel::BranchItem && refType == RefType::Remote) {
            name.append(QStringLiteral(" &nbsp;<span style=\"color:gray; font-size:%1pt;\">remote</span>").arg(fontSz - 1));
        } /*else if (type == RefType::Tag) {
            name.append(QStringLiteral(" &nbsp;<span style=\"color:gray; font-size:%1pt;\">tag at %2</span>").arg(fontSz));
        } */

        doc.setHtml(name);
        doc.setDocumentMargin(2);

        painter->save();

        // paint background
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.highlight());
        } else {
            painter->fillRect(option.rect, option.palette.base());
        }

        options.text = QString(); // clear old text
        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter, options.widget);

        // draw text
        painter->translate(option.rect.x(), option.rect.y());
        // leave space for icon

        painter->translate(25, 0);

        doc.drawContents(painter);

        painter->restore();
    }

public Q_SLOTS:
    void setFilterString(const QString &text)
    {
        m_filterString = text;
    }

private:
    QString m_filterString;
};

BranchesDialog::BranchesDialog(QWidget *parent, KTextEditor::MainWindow *mainWindow, QString projectPath)
    : QMenu(parent)
    , m_mainWindow(mainWindow)
    , m_projectPath(projectPath)
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(4, 4, 4, 4);
    setLayout(layout);

    m_lineEdit = new QLineEdit(this);
    setFocusProxy(m_lineEdit);

    layout->addWidget(m_lineEdit);

    m_treeView = new QTreeView();
    layout->addWidget(m_treeView, 1);
    m_treeView->setTextElideMode(Qt::ElideLeft);
    m_treeView->setUniformRowHeights(true);

    m_model = new BranchesDialogModel(this);

    StyleDelegate *delegate = new StyleDelegate(this);
    m_treeView->setItemDelegateForColumn(0, delegate);

    m_proxyModel = new BranchFilterModel(this);
    m_proxyModel->setFilterRole(Qt::DisplayRole);
    m_proxyModel->setSortRole(Qt::UserRole + 1);

    connect(m_lineEdit, &QLineEdit::returnPressed, this, &BranchesDialog::slotReturnPressed);
    connect(m_lineEdit, &QLineEdit::textChanged, m_proxyModel, &BranchFilterModel::setFilterString);
    connect(m_lineEdit, &QLineEdit::textChanged, delegate, &StyleDelegate::setFilterString);
    connect(m_lineEdit, &QLineEdit::textChanged, this, [this]() {
        m_treeView->viewport()->update();
        reselectFirst();
    });
    connect(m_treeView, &QTreeView::clicked, this, &BranchesDialog::slotReturnPressed);

    m_proxyModel->setSourceModel(m_model);
    m_treeView->setSortingEnabled(true);
    m_treeView->setModel(m_proxyModel);

    m_treeView->installEventFilter(this);
    m_lineEdit->installEventFilter(this);

    m_treeView->setHeaderHidden(true);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_treeView->setSelectionMode(QTreeView::SingleSelection);

    setHidden(true);

    connect(&m_checkoutWatcher, &QFutureWatcher<GitUtils::CheckoutResult>::finished, this, &BranchesDialog::onCheckoutDone);
}

void BranchesDialog::openDialog()
{
    GitUtils::Branch newBranch;
    newBranch.name = QStringLiteral("Create New Branch");
    GitUtils::Branch newBranchFrom;
    newBranchFrom.name = QStringLiteral("Create New Branch From...");
    QVector<GitUtils::Branch> branches{newBranch, newBranchFrom};
    /*QVector<GitUtils::Branch> */ branches << GitUtils::getAllBranches(m_projectPath);
    m_model->refresh(branches);

    reselectFirst();
    updateViewGeometry();
    show();
    setFocus();
}

bool BranchesDialog::eventFilter(QObject *obj, QEvent *event)
{
    // catch key presses + shortcut overrides to allow to have ESC as application wide shortcut, too, see bug 409856
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (obj == m_lineEdit) {
            const bool forward2list = (keyEvent->key() == Qt::Key_Up) || (keyEvent->key() == Qt::Key_Down) || (keyEvent->key() == Qt::Key_PageUp)
                || (keyEvent->key() == Qt::Key_PageDown);
            if (forward2list) {
                QCoreApplication::sendEvent(m_treeView, event);
                return true;
            }

            if (keyEvent->key() == Qt::Key_Escape) {
                m_lineEdit->clear();
                keyEvent->accept();
                hide();
                return true;
            }
        } else {
            const bool forward2input = (keyEvent->key() != Qt::Key_Up) && (keyEvent->key() != Qt::Key_Down) && (keyEvent->key() != Qt::Key_PageUp)
                && (keyEvent->key() != Qt::Key_PageDown) && (keyEvent->key() != Qt::Key_Tab) && (keyEvent->key() != Qt::Key_Backtab);
            if (forward2input) {
                QCoreApplication::sendEvent(m_lineEdit, event);
                return true;
            }
        }
    }

    // hide on focus out, if neither input field nor list have focus!
    else if (event->type() == QEvent::FocusOut && !(m_lineEdit->hasFocus() || m_treeView->hasFocus())) {
        m_lineEdit->clear();
        hide();
        return true;
    }

    return QWidget::eventFilter(obj, event);
}

void BranchesDialog::onCheckoutDone()
{
    const GitUtils::CheckoutResult res = m_checkoutWatcher.result();
    auto msgType = KTextEditor::Message::Positive;
    QString msgStr = i18n("Branch %1 checked out", res.branch);
    if (res.returnCode > 0) {
        msgType = KTextEditor::Message::Warning;
        msgStr = i18n("Failed to checkout branch: %1", res.branch);
    } else {
        Q_EMIT branchChanged(res.branch);
    }

    sendMessage(msgStr, msgType == KTextEditor::Message::Warning);
}

void BranchesDialog::slotReturnPressed()
{
    if (m_model->rowCount() == 0) {
        createNewBranch(m_lineEdit->text());
        ;
        return;
    }

    const auto branch = m_proxyModel->data(m_treeView->currentIndex(), BranchesDialogModel::CheckoutName).toString();
    const auto itemType = (BranchesDialogModel::ItemType)m_proxyModel->data(m_treeView->currentIndex(), BranchesDialogModel::ItemTypeRole).toInt();

    if (itemType == BranchesDialogModel::BranchItem) {
        QFuture<GitUtils::CheckoutResult> future = QtConcurrent::run(&GitUtils::checkoutBranch, m_projectPath, branch);
        m_checkoutWatcher.setFuture(future);
    } else if (itemType == BranchesDialogModel::CreateBranch) {
        m_model->clear();
        m_lineEdit->setPlaceholderText(i18n("Enter new branch name. Press 'Esc' to cancel."));
        return;
    } else if (itemType == BranchesDialogModel::CreateBranchFrom) {
    }

    m_lineEdit->clear();
    hide();
}

void BranchesDialog::reselectFirst()
{
    QModelIndex index = m_proxyModel->index(0, 0);
    m_treeView->setCurrentIndex(index);
}

void BranchesDialog::sendMessage(const QString &message, bool warn)
{
    KTextEditor::Message *msg = new KTextEditor::Message(message, warn ? KTextEditor::Message::Warning : KTextEditor::Message::Positive);
    msg->setPosition(KTextEditor::Message::TopInView);
    msg->setAutoHide(3000);
    msg->setAutoHideMode(KTextEditor::Message::Immediate);
    msg->setView(m_mainWindow->activeView());
    m_mainWindow->activeView()->document()->postMessage(msg);
}

void BranchesDialog::createNewBranch(const QString &branch)
{
    if (branch.isEmpty()) {
        m_lineEdit->clear();
        hide();
        return;
    }
    // the branch name might be invalid, let git handle it
    const GitUtils::CheckoutResult r = GitUtils::checkoutNewBranch(m_projectPath, branch);
    const bool warn = true;
    if (r.returnCode == 0) {
        sendMessage(i18n("Checked out to new branch: %1", r.branch), !warn);
    } else {
        sendMessage(i18n("Failed to create new branch. Error \"%1\"", r.error), warn);
    }

    m_lineEdit->clear();
    hide();
}

void BranchesDialog::updateViewGeometry()
{
    m_treeView->resizeColumnToContents(0);
    m_treeView->resizeColumnToContents(1);

    QWidget *window = m_mainWindow->window();
    const QSize centralSize = window->size();

    // width: 2.4 of editor, height: 1/2 of editor
    const QSize viewMaxSize(centralSize.width() / 2.4, centralSize.height() / 2);

    // Position should be central over window
    const int xPos = std::max(0, (centralSize.width() - viewMaxSize.width()) / 2);
    const int yPos = std::max(0, (centralSize.height() - viewMaxSize.height()) * 1 / 4);
    const QPoint p(xPos, yPos);
    move(p + window->pos());

    this->setFixedSize(viewMaxSize);
}
