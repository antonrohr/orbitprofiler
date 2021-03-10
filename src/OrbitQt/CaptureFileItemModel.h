// Copyright (c) 2021 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ORBIT_QT_CAPTURE_FILE_ITEM_MODEL_H_
#define ORBIT_QT_CAPTURE_FILE_ITEM_MODEL_H_

#include <QAbstractItemModel>
#include <QObject>

namespace orbit_qt {

class CaptureFileItemModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  enum class Column { kFilename, kLastUsed, kCreated, kEnd };

  [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex& idx, int role = Qt::DisplayRole) const override;
  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                    int role = Qt::DisplayRole) const override;
  [[nodiscard]] QModelIndex index(int row, int column,
                                  const QModelIndex& parent = {}) const override;
  [[nodiscard]] QModelIndex parent(const QModelIndex& parent) const override;
  [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;

 private:
  std::vector<std::string> capture_files_{"Capture A.orbit", "Capture B.orbit", "Capture C.orbit"};
};

}  // namespace orbit_qt

#endif  // ORBIT_QT_CAPTURE_FILE_ITEM_MODEL_H_