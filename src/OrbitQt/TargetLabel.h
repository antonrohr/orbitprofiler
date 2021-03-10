// Copyright (c) 2021 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ORBIT_QT_TARGET_LABEL_H_
#define ORBIT_QT_TARGET_LABEL_H_

#include <QObject>
#include <QWidget>
#include <memory>
#include <optional>

#include "OrbitClientData/ProcessData.h"
#include "OrbitGgp/Instance.h"
#include "TargetConfiguration.h"

namespace Ui {
class TargetLabel;  // IWYU pragma: keep
}

namespace orbit_qt {

class TargetLabel : public QWidget {
  Q_OBJECT
 public:
  enum class Icon { kProcessAlive, kProcessEnded, kConnectionDead };
  explicit TargetLabel(QWidget* parent = nullptr);
  ~TargetLabel() override;

  void ChangeToFileTarget(const FileTarget& file_target);
  void ChangeToFileTarget(const std::filesystem::path& path);
  void ChangeToStadiaTarget(const StadiaTarget& stadia_target);
  void ChangeToStadiaTarget(const ProcessData& process, const orbit_ggp::Instance& instance);
  void ChangeToStadiaTarget(const QString& process_name, double cpu_usage,
                            const QString& instance_name);
  void ChangeToLocalTarget(const LocalTarget& local_target);
  void ChangeToLocalTarget(const ProcessData& process);
  void ChangeToLocalTarget(const QString& process_name, double cpu_usage);

  bool SetProcessCpuUsage(double cpu_usage);
  bool SetProcessEnded();
  bool SetConnectionDead(const QString& error_message);

  void Clear();

  [[nodiscard]] QColor GetColor() const;
  [[nodiscard]] QString GetText() const;
  [[nodiscard]] QString GetToolTip() const { return toolTip(); }
  [[nodiscard]] std::optional<Icon> GetIcon() const { return icon_; }

 private:
  std::unique_ptr<Ui::TargetLabel> ui_;
  QString process_ = "";
  QString machine_ = "";
  std::optional<Icon> icon_;

  const QPixmap process_alive_icon_;
  const QPixmap process_ended_icon_;
  const QPixmap connection_ended_icon_;

  void SetColor(const QColor& color);
  void SetIcon(Icon icon);
  void ClearIcon();
};

}  // namespace orbit_qt

#endif  // ORBIT_QT_TARGET_LABEL_H_
