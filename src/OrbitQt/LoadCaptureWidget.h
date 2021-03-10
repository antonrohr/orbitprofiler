// Copyright (c) 2021 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ORBIT_QT_LOAD_CAPTURE_WIDGET_H_
#define ORBIT_QT_LOAD_CAPTURE_WIDGET_H_

#include <QWidget>
#include <memory>

#include "CaptureFileItemModel.h"
#include "ui_LoadCaptureWidget.h"

namespace orbit_qt {

class LoadCaptureWidget : public QWidget {
 public:
  explicit LoadCaptureWidget(QWidget* parent = nullptr);

 private:
  std::unique_ptr<Ui::LoadCaptureWidget> ui_;
  CaptureFileItemModel capture_file_model_;
};

}  // namespace orbit_qt

#endif  // ORBIT_QT_LOAD_CAPTURE_WIDGET_H_