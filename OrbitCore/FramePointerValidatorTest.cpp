// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <vector>

#include "ElfFile.h"
#include "FramePointerValidator.h"
#include "OrbitBase/Logging.h"
#include "Path.h"

TEST(FramePointerValidator, GetFpoFunctions) {
  std::string executable_path = Path::GetExecutablePath();
  std::string test_elf_file = executable_path + "/testdata/hello_world_elf";

  auto elf_file = ElfFile::Create(test_elf_file);
  ASSERT_NE(elf_file, nullptr);

  const auto symbols_result = elf_file->LoadSymbols();
  ASSERT_TRUE(symbols_result);
  uint64_t load_bias = symbols_result.value().load_bias();
  const std::vector<SymbolInfo> symbol_infos(
      symbols_result.value().symbol_infos().begin(),
      symbols_result.value().symbol_infos().end());

  std::vector<CodeBlock> function_infos;

  std::transform(symbol_infos.begin(), symbol_infos.end(),
                 std::back_inserter(function_infos),
                 [&load_bias](const SymbolInfo& s) -> CodeBlock {
                   CodeBlock result;
                   uint64_t symbol_offset = s.address() - load_bias;
                   result.set_offset(symbol_offset);
                   result.set_size(s.size());
                   return result;
                 });

  std::optional<std::vector<CodeBlock>> fpo_functions =
      FramePointerValidator::GetFpoFunctions(function_infos, test_elf_file,
                                             true);

  ASSERT_TRUE(fpo_functions.has_value());

  std::vector<std::string> fpo_function_names;

  // Retrieve the names of all fpo-functions.
  std::transform(
      fpo_functions->begin(), fpo_functions->end(),
      std::back_inserter(fpo_function_names),
      [&symbol_infos, &load_bias](const CodeBlock& code_block) -> std::string {
        // Find the function with that offset to extract the name.
        auto symbol_it =
            std::find_if(symbol_infos.begin(), symbol_infos.end(),
                         [&code_block, &load_bias](const SymbolInfo& s) {
                           uint64_t symbol_offset = s.address() - load_bias;
                           return symbol_offset == code_block.offset();
                         });

        CHECK(symbol_it != symbol_infos.end());
        return (*symbol_it).pretty_name();
      });

  EXPECT_THAT(fpo_function_names, testing::UnorderedElementsAre(
                                      "_start", "main", "__libc_csu_init"));
}
