// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ITEM_MODEL_FUNCTION_ITEM_MODEL_H_
#define ITEM_MODEL_FUNCTION_ITEM_MODEL_H_

#include <qabstractitemmodel.h>

#include <QAbstractItemModel>
#include <vector>

#include "symbol.pb.h"

using orbit_grpc_protos::SymbolInfo;

namespace ItemModels {

class FunctionItemModel : public QAbstractItemModel {
  Q_OBJECT;

 public:
  enum class Column {
    kHooked,
    kFunctionName,
    kSize,
    kSourceFile,
    kSourceLine,
    kModuleName,
    kAddress,
    kEnd
  };
  using QAbstractItemModel::QAbstractItemModel;
  int columnCount(const QModelIndex& parent = {}) const override;
  QVariant data(const QModelIndex& idx, int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex& idx) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
  QModelIndex parent(const QModelIndex& parent) const override;
  int rowCount(const QModelIndex& parent = {}) const override;

  // TODO(antonrohr) change to type FunctionInfo or FunctionData
  void AddFunctions(std::vector<SymbolInfo> functions);
  void ClearFunctions();

  // TODO(antonrohr) change to type FunctionInfo or FunctionData
  static const SymbolInfo* FunctionInfoFromModelIndex(const QModelIndex&);

 protected:
  // bool canFetchMore(const QModelIndex& parent) const override;
  // void fetchMore(const QModelIndex& parent) override;

 private:
  // TODO(antonrohr) change to type FunctionInfo or FunctionData
  std::vector<SymbolInfo> functions_;
  int number_loaded_ = 0;
};

}  // namespace ItemModels

// TODO(antonrohr) change to type FunctionInfo or FunctionData

Q_DECLARE_METATYPE(const SymbolInfo*);

#endif  // ITEM_MODEL_FUNCTION_ITEM_MODEL_H_
