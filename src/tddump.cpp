#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "server_io.h"
#include "utils.h"

bool enable_log;

int main(int argc, char *argv[]) {
  if (argc < 2) return 0;
  std::ios::sync_with_stdio(false);
  std::cerr << std::nounitbuf;
  int pid = std::atoi(argv[1]);
  Submission sub;
  sub.problem_id = pid;

  std::string testdata_dir = TdPath(sub.problem_id);
  if (access(testdata_dir.c_str(), F_OK)) Execute("mkdir", "-p", testdata_dir);
  downloadTestdata(sub);

  return 0;
}
