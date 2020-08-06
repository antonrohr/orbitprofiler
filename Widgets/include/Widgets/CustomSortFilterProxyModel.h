// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WIDGETS_CUSTOM_SORT_FILTER_PROXY_MODEL_H_
#define WIDGETS_CUSTOM_SORT_FILTER_PROXY_MODEL_H_

#include <qabstractitemmodel.h>
#include <qobjectdefs.h>
#include <qsortfilterproxymodel.h>

namespace Widgets {

class CustomSortFilterProxyModel : public QSortFilterProxyModel {
  Q_OBJECT;

 public:
  CustomSortFilterProxyModel(QObject* parent = 0)
      : QSortFilterProxyModel(parent) {}

 public slots:
  // void setFilterText(const QString& string);

 protected:
  // bool filterAcceptsRow(int sourceRow,
  //                       const QModelIndex& sourceParent) const override;
  bool canFetchMore(const QModelIndex& parent) const override;
  void fetchMore(const QModelIndex& parent) override;

 private:
  // QString filter_text_;
  int number_loaded_ = 0;
};

}  // namespace Widgets

#endif  // WIDGETS_CUSTOM_SORT_FILTER_PROXY_MODEL_H_