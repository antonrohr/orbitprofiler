// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ItemModels/FunctionItemModel.h"

#include <absl/strings/str_format.h>
#include <qabstractitemmodel.h>
#include <qglobal.h>
#include <qnamespace.h>
#include <qvariant.h>

#include <algorithm>
#include <vector>

#include "OrbitBase/Logging.h"
#include "ScopeTimer.h"
#include "google/protobuf/util/message_differencer.h"
#include "symbol.pb.h"

namespace ItemModels {

// TODO(antonrohr) change to type FunctionInfo or FunctionData
const SymbolInfo* FunctionItemModel::FunctionInfoFromModelIndex(
    const QModelIndex& idx) {
  CHECK(idx.isValid());
  return idx.data(Qt::UserRole).value<const SymbolInfo*>();
}

int FunctionItemModel::columnCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(Column::kEnd);
}

QVariant FunctionItemModel::data(const QModelIndex& idx, int role) const {
  CHECK(idx.isValid());
  CHECK(idx.model() == static_cast<const QAbstractItemModel*>(this));
  CHECK(idx.row() >= 0 && idx.row() < functions_.size());
  CHECK(idx.column() >= 0 && idx.column() < static_cast<int>(Column::kEnd));

  const auto& function = functions_[idx.row()];

  if (role == Qt::UserRole) {
    return QVariant::fromValue(&function);
  }

  if (role == Qt::DisplayRole) {
    switch (static_cast<Column>(idx.column())) {
      case Column::kHooked:
        return "dummy hooked";  // TODO(antonrohr) have hooked value
      case Column::kFunctionName:
        return QString::fromStdString(function.demangled_name());
      case Column::kSize:
        return QVariant::fromValue(function.size());
      case Column::kSourceFile:
        return QString::fromStdString(function.source_file());
      case Column::kSourceLine:
        return function.source_line();
      case Column::kModuleName:
        return "dummy module name";
      case Column::kAddress:
        // TODO(antonrohr) have absolute address (not relative in module)
        return QString("0x%1").arg(function.address(), 0, 16);
      case Column::kEnd:
        Q_UNREACHABLE();
    }
  }

  // When the EditRole is requested, in general we return the unformatted raw
  // value. This is used for sorting and filtering.

  if (role == Qt::EditRole) {
    switch (static_cast<Column>(idx.column())) {
      case Column::kHooked:
        return "dummy hooked";  // TODO(antonrohr) have hooked value
      case Column::kFunctionName:
        return QString::fromStdString(function.demangled_name());
      case Column::kSize:
        return static_cast<quint64>(function.size());
      case Column::kSourceFile:
        return QString::fromStdString(function.source_file());
      case Column::kSourceLine:
        return static_cast<quint32>(function.source_line());
      case Column::kModuleName:
        return "dummy module name";  // TODO(antonrohr)
      case Column::kAddress:
        // TODO(antonrohr) have absolute address (not relative in module)
        return static_cast<quint64>(function.address());
      case Column::kEnd:
        Q_UNREACHABLE();
    }
  }

  return {};
}

Qt::ItemFlags FunctionItemModel::flags(const QModelIndex& idx) const {
  CHECK(idx.isValid());

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
}

QVariant FunctionItemModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const {
  if (orientation == Qt::Vertical) {
    return {};
  }

  if (role == Qt::DisplayRole) {
    switch (static_cast<Column>(section)) {
      case Column::kHooked:
        return "Hooked";
      case Column::kFunctionName:
        return "Function";
      case Column::kSize:
        return "Size";
      case Column::kSourceFile:
        return "File";
      case Column::kSourceLine:
        return "Line";
      case Column::kModuleName:
        return "Module";
      case Column::kAddress:
        return "Address";
      case Column::kEnd:
        Q_UNREACHABLE();
    }
  }

  return {};
}

QModelIndex FunctionItemModel::index(int row, int column,
                                     const QModelIndex& parent) const {
  if (parent.isValid()) {
    return {};
  }

  if (row >= 0 && row < functions_.size() && column >= 0 &&
      column < static_cast<int>(Column::kEnd)) {
    return createIndex(row, column);
  }

  return {};
}

QModelIndex FunctionItemModel::parent(const QModelIndex& parent) const {
  return {};
}

int FunctionItemModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    return 0;
  }

  return functions_.size();
}

// // TODO(antonrohr) change to type FunctionInfo or FunctionData
// static std::string UniquelyIdentifyFunction(const SymbolInfo& function) {
//   // TODO (antonrohr) use real module data
//   return absl::StrFormat("%s%lu", "dummy module", function.address());
// }

// // TODO(antonrohr) change to type FunctionInfo or FunctionData
// static bool CompareFunctionsByUniqueIdentification(const SymbolInfo& lhs,
//                                                    const SymbolInfo& rhs) {
//   return UniquelyIdentifyFunction(lhs) < UniquelyIdentifyFunction(rhs);
// }

void FunctionItemModel::AddFunctions(std::vector<SymbolInfo> functions) {
  SCOPE_TIMER_LOG("AddFunctions");
  beginInsertRows({}, functions_.size(),
                  functions_.size() + functions.size() - 1);
  // size_t size_before = functions_.size();
  // size_t insert_amount =
  //     std::min(size_before + 100, size_before + functions.size());
  // beginInsertRows({}, size_before, insert_amount - 1);
  // number_loaded_ = insert_amount;
  functions_.insert(functions_.end(), functions.begin(), functions.end());
  endInsertRows();
}

void FunctionItemModel::ClearFunctions() {
  beginRemoveRows({}, 0, functions_.size() - 1);
  functions_.clear();
  endRemoveRows();
}

// void FunctionItemModel::SetFunctions(std::vector<SymbolInfo> new_functions) {
//   SCOPE_TIMER_LOG("SetFunctions");

//   // beginInsertRows({}, 0, new_functions.size() - 1);
//   // functions_ = new_functions;
//   // endInsertRows();

//   {
//     SCOPE_TIMER_LOG("SetFunctions, sort");
//     std::sort(new_functions.begin(), new_functions.end(),
//               &CompareFunctionsByUniqueIdentification);
//   }
//   auto old_iter = functions_.begin();
//   auto new_iter = new_functions.begin();

//   while (old_iter != functions_.end() && new_iter != new_functions.end()) {
//     const int current_row =
//         static_cast<int>(std::distance(functions_.begin(), old_iter));

//     if (UniquelyIdentifyFunction(*old_iter) ==
//         UniquelyIdentifyFunction(*new_iter)) {
//       if (!google::protobuf::util::MessageDifferencer::Equivalent(*old_iter,
//                                                                   *new_iter))
//                                                                   {
//         *old_iter = *new_iter;
//         emit dataChanged(
//             index(current_row, 0, {}),
//             index(current_row, static_cast<int>(Column::kEnd) - 1, {}));
//       }
//       ++old_iter;
//       ++new_iter;
//     } else if (CompareFunctionsByUniqueIdentification(*old_iter, *new_iter))
//     {
//       beginRemoveRows({}, current_row, current_row);
//       old_iter = functions_.erase(old_iter);
//       endRemoveRows();
//     } else {
//       beginInsertRows({}, current_row, current_row);
//       old_iter = functions_.insert(old_iter, *new_iter);
//       ++old_iter;
//       ++new_iter;
//       endInsertRows();
//     }
//   }

//   if (old_iter == functions_.end() && new_iter != new_functions.end()) {
//     beginInsertRows({}, functions_.size(), new_functions.size() - 1);
//     std::copy(new_iter, new_functions.end(), std::back_inserter(functions_));
//     CHECK(functions_.size() == new_functions.size());
//     endInsertRows();
//   } else if (old_iter != functions_.end() && new_iter == new_functions.end())
//   {
//     beginRemoveRows({}, new_functions.size(), functions_.size() - 1);
//     functions_.erase(old_iter, functions_.end());
//     CHECK(functions_.size() == new_functions.size());
//     endRemoveRows();
//   }
// }

// bool FunctionItemModel::canFetchMore(const QModelIndex& parent) const {
//   if (parent.isValid()) return false;
//   return (number_loaded_ < functions_.size());
// }

// void FunctionItemModel::fetchMore(const QModelIndex& parent) {
//   if (parent.isValid()) return;
//   int remainder = functions_.size() - number_loaded_;
//   int items_to_fetch = qMin(remainder, 100);

//   if (items_to_fetch <= 0) return;

//   beginInsertRows({}, number_loaded_, number_loaded_ + items_to_fetch - 1);
//   number_loaded_ += items_to_fetch;
//   endInsertRows();
// }

}  // namespace ItemModels