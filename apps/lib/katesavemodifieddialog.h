/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004 Joseph Wenninger <jowenn@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <ktexteditor/document.h>

#include <QDialog>

class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;

class KateSaveModifiedDialog : public QDialog
{
    Q_OBJECT
public:
    KateSaveModifiedDialog(QWidget *parent, const std::vector<KTextEditor::Document *> &documents);
    ~KateSaveModifiedDialog() override;
    static bool queryClose(QWidget *parent, const std::vector<KTextEditor::Document *> &documents);

protected:
    bool doSave();
protected Q_SLOTS:
    void slotSelectAll();
    void slotItemActivated(QTreeWidgetItem *, int);
    void slotSaveSelected();
    void slotDoNotSave();

private:
    QTreeWidget *m_list;
    QPushButton *m_saveButton;
};
