// Copyright (c) 2021 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "LoadCaptureWidget.h"

#include <QObject>
#include <QRadioButton>
#include <memory>

#include "qheaderview.h"

namespace orbit_qt {

LoadCaptureWidget::LoadCaptureWidget(QWidget* parent)
    : QWidget(parent), ui_(std::make_unique<Ui::LoadCaptureWidget>()) {
  ui_->setupUi(this);
  ui_->tableView->setModel(&capture_file_model_);
  ui_->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

  QObject::connect(ui_->radioButton, &QRadioButton::clicked, this, [this](bool checked) {
    ui_->tableView->setEnabled(checked);
    ui_->selectFileButton->setEnabled(checked);
  });

  ui_->tableView->setEnabled(false);
  ui_->selectFileButton->setEnabled(false);
}

}  // namespace orbit_qt