#include "batchjudge.h"

#include <algorithm>
#include <iostream>

#include "sandbox.h"
#include "utils.h"

int BatchJudge(int pid, int td, int boxid, int tl, int ml, int ol, int testee,
               std::string lang) {
  std::ios::sync_with_stdio(false);
  std::cerr << std::nounitbuf;

  std::string target;
  if (lang.find_first_of("python") == 0) {  
    target = "main.pyc";
  } else {
    target = "main.out";
  }

  // init box
  SandboxInit(boxid);
  std::string boxpath = BoxPath(boxid);
  std::string tdinput = TdInput(pid, td);
  std::string boxinput = BoxInput(boxid);
  std::string boxoutput = BoxOutput(boxid);
  int ret = Execute("cp", tdinput, boxinput);
  if (ret) return ret;
  chmod(boxpath.c_str(), 0755);
  chmod(boxinput.c_str(), 0666);
  chmod(boxoutput.c_str(), 0666);
  ret = Execute("cp", BoxPath(testee) + target, boxpath + target);
  if (ret) return ret;

  // set options
  SandboxOptions opt;
  opt.cgroup = true;
  opt.procs = 1;
  opt.input = "input";
  opt.output = "output";
  opt.errout = "/dev/null";
  opt.meta = "./testzone/META" + PadInt(td);
  opt.timeout = tl;
  opt.mem = ml;
  opt.fsize_limit = ol;
  opt.envs.push_back(std::string("PATH=") + getenv("PATH"));
  opt.file_limit = 48;
  if (lang.find_first_of("python") == 0) {
    opt.envs.push_back("HOME=" + BoxPath(boxid));
    opt.envs.push_back("PYTHONIOENCODING=utf-8");
    SandboxExec(boxid, opt, {"/usr/bin/env", std::move(lang), "main.pyc"});
  } else {
    SandboxExec(boxid, opt, {target});
  }
  return 0;
}
