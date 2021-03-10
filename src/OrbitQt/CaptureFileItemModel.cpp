// Copyright (c) 2021 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "CaptureFileItemModel.h"

#include "OrbitBase/Logging.h"
#include "qdatetime.h"

namespace orbit_qt {

int CaptureFileItemModel::columnCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(Column::kEnd);
}

QVariant CaptureFileItemModel::data(const QModelIndex& idx, int role) const {
  CHECK(idx.isValid());
  CHECK(idx.model() == static_cast<const QAbstractItemModel*>(this));
  CHECK(idx.row() >= 0 && idx.row() < static_cast<int>(capture_files_.size()));
  CHECK(idx.column() >= 0 && idx.column() < static_cast<int>(Column::kEnd));

  const std::string& filename = capture_files_[idx.row()];

  // if (role == Qt::UserRole) {
  //   return QVariant::fromValue(&process);
  // }

  if (role == Qt::DisplayRole) {
    switch (static_cast<Column>(idx.column())) {
      case Column::kFilename:
        return QString::fromStdString(filename);
      case Column::kLastUsed:
        return QDateTime::currentDateTime();
      case Column::kCreated:
        return QDateTime::currentDateTime();
      case Column::kEnd:
        UNREACHABLE();
    }
  }

  // // When the EditRole is requested, we return the unformatted raw value, which
  // // means the CPU Usage is returned as a double.
  // if (role == Qt::EditRole) {
  //   switch (static_cast<Column>(idx.column())) {
  //     case Column::kFilename:
  //       return process.pid();
  //     case Column::kName:
  //       return QString::fromStdString(process.name());
  //     case Column::kCpu:
  //       return process.cpu_usage();
  //     case Column::kEnd:
  //       UNREACHABLE();
  //   }
  // }

  // if (role == Qt::ToolTipRole) {
  //   // We don't care about the column when showing tooltip. It's the same for
  //   // the whole row.
  //   return QString::fromStdString(process.command_line());
  // }

  // if (role == Qt::TextAlignmentRole) {
  //   switch (static_cast<Column>(idx.column())) {
  //     case Column::kPid:
  //       return QVariant(Qt::AlignVCenter | Qt::AlignRight);
  //     case Column::kName:
  //       return {};
  //     case Column::kCpu:
  //       return QVariant(Qt::AlignVCenter | Qt::AlignRight);
  //     case Column::kEnd:
  //       UNREACHABLE();
  //   }
  // }

  return {};
}

QVariant CaptureFileItemModel::headerData(int section, Qt::Orientation orientation,
                                          int role) const {
  if (orientation == Qt::Vertical) {
    return {};
  }

  if (role == Qt::DisplayRole) {
    switch (static_cast<Column>(section)) {
      case Column::kFilename:
        return "Filename";
      case Column::kLastUsed:
        return "Last used";
      case Column::kCreated:
        return "Created";
      case Column::kEnd:
        UNREACHABLE();
    }
  }
  return {};
}

QModelIndex CaptureFileItemModel::index(int row, int column, const QModelIndex& parent) const {
  if (parent.isValid()) return {};
  if (row < 0 || row >= static_cast<int>(capture_files_.size())) return {};
  if (column < 0 || column >= static_cast<int>(Column::kEnd)) return {};

  return createIndex(row, column);
}

QModelIndex CaptureFileItemModel::parent(const QModelIndex& /*parent*/) const { return {}; }

int CaptureFileItemModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    return 0;
  }

  return capture_files_.size();
}

}  // namespace orbit_qt