// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "Widgets/CustomSortFilterProxyModel.h"

#include <qabstractitemmodel.h>

#include "OrbitBase/Logging.h"

namespace Widgets {

// bool CustomSortFilterProxyModel::filterAcceptsRow(
//     int sourceRow, const QModelIndex& sourceParent) const {
//   QModelIndex index_name = sourceModel()->index(sourceRow, 1, sourceParent);
//   return sourceModel()->data(index_name).toString().contains(filter_text_);
//   // return true;
// }

// void CustomSortFilterProxyModel::setFilterText(const QString& string) {
//   LOG("new filter");
//   filter_text_ = string;
//   invalidate();
//   // invalidateFilter();
//   // if (canFetchMore({})) fetchMore({});
// }

bool CustomSortFilterProxyModel::canFetchMore(const QModelIndex& parent) const {
  LOGRED("can fetch more proxy called");
  if (parent.isValid()) return false;

  return sourceModel()->canFetchMore(mapToSource(parent));

  // return (number_loaded_ < functions_.size());
}

void CustomSortFilterProxyModel::fetchMore(const QModelIndex& parent) {
  LOGRED("fetch more proxy called");
  if (parent.isValid()) return;

  sourceModel()->fetchMore(mapToSource(parent));
}

}  // namespace Widgets