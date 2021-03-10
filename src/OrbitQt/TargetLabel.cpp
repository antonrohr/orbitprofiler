// Copyright (c) 2021 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "TargetLabel.h"

#include <QImage>
#include <QPalette>
#include <QPixmap>
#include <memory>
#include <optional>

#include "ui_TargetLabel.h"

namespace {
const QColor kDefaultTextColor{"white"};
const QColor kGreenColor{"#66BB6A"};
const QColor kOrangeColor{"orange"};
const QColor kRedColor{"red"};
const QString kLocalhostName{"localhost"};

QPixmap ColorizeIcon(const QPixmap& pixmap, const QColor& color) {
  QImage colored_image = pixmap.toImage();

  for (int y = 0; y < colored_image.height(); y++) {
    for (int x = 0; x < colored_image.width(); x++) {
      QColor color_with_alpha = color;
      color_with_alpha.setAlpha(colored_image.pixelColor(x, y).alpha());
      colored_image.setPixelColor(x, y, color_with_alpha);
    }
  }

  return QPixmap::fromImage(std::move(colored_image));
}

}  // namespace

namespace orbit_qt {

namespace fs = std::filesystem;

TargetLabel::TargetLabel(QWidget* parent)
    : QWidget(parent),
      ui_(std::make_unique<Ui::TargetLabel>()),
      process_alive_icon_(ColorizeIcon(QPixmap{":/actions/connected"}, kGreenColor)),
      process_ended_icon_(ColorizeIcon(QPixmap{":/actions/disconnected"}, kOrangeColor)),
      connection_ended_icon_(ColorizeIcon(QPixmap{":/actions/disconnected"}, kRedColor)) {
  ui_->setupUi(this);
}

TargetLabel::~TargetLabel() = default;

void TargetLabel::ChangeToFileTarget(const FileTarget& file_target) {
  ChangeToFileTarget(file_target.GetCaptureFilePath());
}

void TargetLabel::ChangeToFileTarget(const fs::path& path) {
  Clear();
  ui_->textLabel->setText(QString::fromStdString(path.filename().string()));
}

void TargetLabel::ChangeToStadiaTarget(const StadiaTarget& stadia_target) {
  ChangeToStadiaTarget(*stadia_target.GetProcess(), stadia_target.GetConnection()->GetInstance());
}

void TargetLabel::ChangeToStadiaTarget(const ProcessData& process,
                                       const orbit_ggp::Instance& instance) {
  ChangeToStadiaTarget(QString::fromStdString(process.name()), process.cpu_usage(),
                       instance.display_name);
}

void TargetLabel::ChangeToStadiaTarget(const QString& process_name, double cpu_usage,
                                       const QString& instance_name) {
  Clear();
  process_ = process_name;
  machine_ = instance_name;
  SetProcessCpuUsage(cpu_usage);
}

void TargetLabel::ChangeToLocalTarget(const LocalTarget& local_target) {
  ChangeToLocalTarget(*local_target.GetProcess());
}

void TargetLabel::ChangeToLocalTarget(const ProcessData& process) {
  ChangeToLocalTarget(QString::fromStdString(process.name()), process.cpu_usage());
}

void TargetLabel::ChangeToLocalTarget(const QString& process_name, double cpu_usage) {
  Clear();
  process_ = process_name;
  machine_ = kLocalhostName;
  SetProcessCpuUsage(cpu_usage);
}

bool TargetLabel::SetProcessCpuUsage(double cpu_usage) {
  if (process_.isEmpty() || machine_.isEmpty()) return false;

  ui_->textLabel->setText(
      QString{"%1 (%2%) @ %3"}.arg(process_, QString::number(cpu_usage, 'f', 0), machine_));
  SetColor(kGreenColor);
  setToolTip({});
  SetIcon(Icon::kProcessAlive);
  return true;
}

bool TargetLabel::SetProcessEnded() {
  if (process_.isEmpty() || machine_.isEmpty()) return false;

  ui_->textLabel->setText(QString{"%1 @ %2"}.arg(process_, machine_));
  SetColor(kOrangeColor);
  setToolTip("The process ended.");
  SetIcon(Icon::kProcessEnded);
  return true;
}

bool TargetLabel::SetConnectionDead(const QString& error_message) {
  if (process_.isEmpty() || machine_.isEmpty()) return false;

  ui_->textLabel->setText(QString{"%1 @ %2"}.arg(process_, machine_));
  SetColor(kRedColor);
  setToolTip(error_message);
  SetIcon(Icon::kConnectionDead);
  return true;
}

void TargetLabel::Clear() {
  process_ = "";
  machine_ = "";
  ui_->textLabel->setText({});
  SetColor(kDefaultTextColor);
  setToolTip({});
  ClearIcon();
}

QColor TargetLabel::GetColor() const {
  return ui_->textLabel->palette().color(QPalette::WindowText);
}

QString TargetLabel::GetText() const { return ui_->textLabel->text(); }

void TargetLabel::SetColor(const QColor& color) {
  // This class is used in a QFrame and in QMenuBar. To make the coloring work in a QFrame the
  // QColorRole QPalette::WindowText needs to be set. For QMenuBar QPalette::ButtonText needs to be
  // set.
  QPalette palette{};
  palette.setColor(QPalette::WindowText, color);
  palette.setColor(QPalette::ButtonText, color);
  ui_->textLabel->setPalette(palette);
}

void TargetLabel::SetIcon(Icon icon) {
  icon_ = icon;
  switch (icon) {
    case Icon::kProcessAlive:
      ui_->iconLabel->setPixmap(process_alive_icon_);
      break;
    case Icon::kProcessEnded:
      ui_->iconLabel->setPixmap(process_ended_icon_);
      break;
    case Icon::kConnectionDead:
      ui_->iconLabel->setPixmap(connection_ended_icon_);
      break;
  }
}

void TargetLabel::ClearIcon() {
  icon_ = std::nullopt;
  ui_->iconLabel->setPixmap(QPixmap{});
}

}  // namespace orbit_qt