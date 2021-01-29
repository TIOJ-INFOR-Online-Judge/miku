#ifndef SANDBOX_H_
#define SANDBOX_H_

#include <string>
#include <vector>

class SandboxOptions {
 public:
  bool cgroup;                    // --cg
  bool preserve_env;              // --full-env
  std::vector<std::string> dirs;  // --dir
  std::string input;              // --stdin
  std::string output;             // --stdout
  std::string errout;             // --stderr
  std::string meta;               // --meta
  std::vector<std::string> envs;  // --env
  int mem;                        // --cg-mem in kilobytes
  int procs;                      // --processes
  int timeout;                    // --time in ms
  int file_limit;                 // --file-limit number of opened files
  int fsize_limit;                // --fsize in kilobytes
  SandboxOptions()
      : cgroup(true),
        preserve_env(false),
        mem(0),
        procs(1),
        timeout(0),
        file_limit(64),
        fsize_limit(0) {}
};

int sandboxInit(int boxid);

int sandboxExec(int boxid, const SandboxOptions &,
                const std::vector<std::string> &);

int sandboxDele(int boxid);

#endif  // SANDBOX_H_
