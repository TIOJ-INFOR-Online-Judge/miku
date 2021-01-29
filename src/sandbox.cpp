#include "sandbox.h"

#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "utils.h"

int SandboxExec(int boxid, const SandboxOptions& opt,
                const std::vector<std::string>& comm) {
  std::vector<std::string> args{"isolate", "--box-id=" + PadInt(boxid)};
  if (opt.cgroup) args.emplace_back("--cg");
  if (opt.preserve_env) {
    args.emplace_back("--full-env");
  } else {
    for (const auto& env : opt.envs) {
      args.emplace_back("--env=" + env);
    }
  }
  for (const auto& dir : opt.dirs) args.emplace_back("--dir=" + dir);
  if (!opt.input.empty()) args.emplace_back("--stdin=" + opt.input);
  if (!opt.output.empty()) args.emplace_back("--stdout=" + opt.output);
  if (!opt.errout.empty()) args.emplace_back("--stderr=" + opt.errout);
  if (!opt.meta.empty()) args.emplace_back("--meta=" + opt.meta);
  if (opt.mem != 0) args.emplace_back("--mem=" + PadInt(opt.mem));
  args.emplace_back("--processes=" + PadInt(opt.procs));
  auto TimeStr = [](int64_t x) {
    return PadInt(x / 1000) + '.' + PadInt(x % 1000, 3);
  };
  if (opt.timeout != 0) {
    args.emplace_back("--time=" + TimeStr(opt.timeout));
    args.emplace_back("--wall-time=" + TimeStr(opt.timeout * 2));
  }
  if (opt.fsize_limit) args.emplace_back("--fsize=" + PadInt(opt.fsize_limit));
  args.emplace_back("--file-limit=" + PadInt(opt.file_limit));
  args.emplace_back("--extra-time=0.2");
  args.emplace_back("--run");
  args.emplace_back("--");
  for (const auto& str : comm) args.emplace_back(str);
  Log("[debug] box-", boxid, "start exec");
  Execute(args);
  return 0;
}

int SandboxInit(int boxid) {
  Log("[debug] box-", boxid, "start init");
  ExecuteRedir("", "/dev/null", "/dev/null", "isolate",
               "--box-id=" + PadInt(boxid), "--cg", "--init");
  return 0;
}

int SandboxDele(int boxid) {
  Log("[debug] box-", boxid, "start clean");
  ExecuteRedir("", "/dev/null", "/dev/null", "isolate",
               "--box-id=" + PadInt(boxid), "--cg", "--cleanup");
  return 0;
}
