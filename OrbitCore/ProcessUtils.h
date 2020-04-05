//-----------------------------------
// Copyright Pierric Gimmig 2013-2017
//-----------------------------------
#ifndef PROCESS_UTILS_H_
#define PROCESS_UTILS_H_

#include <unordered_map>

#include "OrbitProcess.h"
#include "Serialization.h"

//-----------------------------------------------------------------------------
namespace ProcessUtils {
bool Is64Bit(HANDLE hProcess);
}

//-----------------------------------------------------------------------------
struct ProcessList {
 public:
  ProcessList();
  void Refresh();
  void Clear();
  void SortByID();
  void SortByName();
  void SortByCPU();
  void UpdateCpuTimes();
  void SetRemote(bool a_Value);
  bool Contains(uint32_t pid) const;
  std::shared_ptr<Process> GetProcessById(uint32_t pid) const;
  std::shared_ptr<Process> GetProcessByIndex(size_t index) const;
  std::vector<std::shared_ptr<Process>> GetProcesses() const {
    return m_Processes;
  }
  // TODO: this does not actually update, but just update the cpu utilization
  void UpdateFromRemote(const std::shared_ptr<ProcessList>& remote_list);
  void UpdateProcess(const std::shared_ptr<Process>& newer_version);

  ORBIT_SERIALIZABLE;

 private:
  std::vector<std::shared_ptr<Process>> m_Processes;
  std::unordered_map<uint32_t, std::shared_ptr<Process>> m_ProcessesMap;
};

#endif  // PROCESS_UTILS_H_