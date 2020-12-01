// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ORBIT_QT_ORBIT_MAIN_WINDOW_H_
#define ORBIT_QT_ORBIT_MAIN_WINDOW_H_

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QString>
#include <QTimer>
#include <memory>
#include <outcome.hpp>
#include <string>
#include <vector>

#include "CallStackDataView.h"
#include "CallTreeView.h"
#include "Connections.h"
#include "DisassemblyReport.h"
#include "OrbitBase/Logging.h"
#include "StatusListener.h"
#include "TargetConfiguration.h"
#include "servicedeploymanager.h"

namespace Ui {
class OrbitMainWindow;
}

class OrbitMainWindow : public QMainWindow {
  Q_OBJECT

 public:
  OrbitMainWindow(QApplication* app, orbit_qt::ConnectionConfiguration connection_configuration,
                  uint32_t font_size);
  // TODO (170468590) remove when not needed anymore
  OrbitMainWindow(QApplication* app, orbit_qt::ServiceDeployManager* service_deploy_manager,
                  uint32_t font_size);
  ~OrbitMainWindow() override;

  void RegisterGlWidget(class OrbitGLWidget* a_GlWidget) { m_GlWidgets.push_back(a_GlWidget); }
  void OnRefreshDataViewPanels(DataViewType a_Type);
  void UpdatePanel(DataViewType a_Type);

  void OnNewSamplingReport(DataView* callstack_data_view,
                           std::shared_ptr<class SamplingReport> sampling_report);
  void OnNewSelectionReport(DataView* callstack_data_view,
                            std::shared_ptr<class SamplingReport> sampling_report);

  void OnNewTopDownView(std::unique_ptr<CallTreeView> top_down_view);
  void OnNewSelectionTopDownView(std::unique_ptr<CallTreeView> selection_top_down_view);

  void OnNewBottomUpView(std::unique_ptr<CallTreeView> bottom_up_view);
  void OnNewSelectionBottomUpView(std::unique_ptr<CallTreeView> selection_bottom_up_view);

  std::string OnGetSaveFileName(const std::string& extension);
  void OnSetClipboard(const std::string& text);
  void OpenDisassembly(std::string a_String, DisassemblyReport report);
  void OpenCapture(const std::string& filepath);
  void OnCaptureCleared();

  Ui::OrbitMainWindow* GetUi() { return ui; }

  std::optional<orbit_qt::ConnectionConfiguration> ClearConnectionConfiguration() {
    return std::move(connection_configuration_);
  }

 protected:
  void closeEvent(QCloseEvent* event) override;

 private slots:
  void on_actionAbout_triggered();

  void on_actionReport_Missing_Feature_triggered();
  void on_actionReport_Bug_triggered();

  void OnTimer();
  void OnLiveTabFunctionsFilterTextChanged(const QString& text);
  void OnFilterFunctionsTextChanged(const QString& text);
  void OnFilterTracksTextChanged(const QString& text);

  void on_actionOpen_Preset_triggered();
  void on_actionEnd_Session_triggered();
  void on_actionQuit_triggered();
  void on_actionSave_Preset_As_triggered();

  void on_actionToggle_Capture_triggered();
  void on_actionSave_Capture_triggered();
  void on_actionOpen_Capture_triggered();
  void on_actionClear_Capture_triggered();
  void on_actionHelp_triggered();

  void on_actionCheckFalse_triggered();
  void on_actionNullPointerDereference_triggered();
  void on_actionStackOverflow_triggered();
  void on_actionServiceCheckFalse_triggered();
  void on_actionServiceNullPointerDereference_triggered();
  void on_actionServiceStackOverflow_triggered();

  void ShowCaptureOnSaveWarningIfNeeded();
  void ShowEmptyFrameTrackWarningIfNeeded(std::string_view function);

 private:
  void StartMainTimer();
  void SetupCaptureToolbar();
  void SetupCodeView();
  void SetupMainWindow(uint32_t font_size);

 private:
  QApplication* m_App;
  Ui::OrbitMainWindow* ui;
  QTimer* m_MainTimer = nullptr;
  std::vector<OrbitGLWidget*> m_GlWidgets;
  QFrame* hint_frame_ = nullptr;

  // Capture toolbar.
  QIcon icon_start_capture_;
  QIcon icon_stop_capture_;

  // Status listener
  std::unique_ptr<StatusListener> status_listener_;

  std::optional<orbit_qt::ConnectionConfiguration> connection_configuration_;
};

#endif  // ORBIT_QT_ORBIT_MAIN_WINDOW_H_
