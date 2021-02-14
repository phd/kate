#ifndef GITSTATUSMODEL_H
#define GITSTATUSMODEL_H

#include <QAbstractItemModel>

#include "git/gitstatus.h"

class GitStatusModel : public QAbstractItemModel
{
public:
    explicit GitStatusModel(QObject *parent);

public:
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void addItems(const QVector<GitUtils::StatusItem> &staged,
                  const QVector<GitUtils::StatusItem> &changed,
                  const QVector<GitUtils::StatusItem> &unmerge,
                  const QVector<GitUtils::StatusItem> &untracked);
    QVector<int> emptyRows();

private:
    QVector<GitUtils::StatusItem> m_nodes[4];
};

#endif // GITSTATUSMODEL_H
