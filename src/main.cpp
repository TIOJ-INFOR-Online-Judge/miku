#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "config.h"
#include "server_io.h"
#include "testsuite.h"
#include "utils.h"

bool enable_log;

void usage() {
  std::cout << R"(Usage:

-v(, or --verbose) for extra verbosity
-p(, or --parallel) [NUMBER] to have maxium [NUMBER] of parallel
processes to evaluate usercode. However this may cause verbosity
messages unreadable.
-b [NUMBER] to set sandbox indexing offset. Need to be set to an
appropriate number if running multiple judgse on one computer.
-a(, or --aggressive-update) add this to aggressivly update
verdict and result.)"
            << std::endl;
  exit(2);
}

const char optstring[] = "vp:b:a";

const struct option longopts[] = {{"verbose", no_argument, NULL, 'v'},
                                  {"parallel", required_argument, NULL, 'p'},
                                  {"boxoffset", required_argument, NULL, 'b'},
                                  {"aggressive-update", no_argument, NULL, 'a'},
                                  {NULL, 0, NULL, 0}};

int main(int argc, char *argv[]) {
  std::ios::sync_with_stdio(false);
  std::cerr << std::nounitbuf;
  // initialize
  bool verbose = false;
  int ac;
  while ((ac = getopt_long(argc, argv, optstring, longopts, NULL)) != -1) {
    switch (ac) {
      case 'v':
        verbose = true;
        break;
      case 'p':
        MAXPARNUM = std::atoi(optarg);
        break;
      case 'b':
        BOXOFFSET = std::atoi(optarg);
        break;
      case 'a':
        AGGUPDATE = true;
        break;
      default:
        usage();
    }
  }
  enable_log = verbose;

  // initialize done

  if (geteuid() != 0) {
    std::cerr << "Must be started as root !" << std::endl;
    return 1;
  }

  if (!InitServerIO()) {
    Log("Error starting server_io.py");
    return 1;
  }

  while (true) {
    Submission sub;
    int status = FetchSubmission(sub);
    if (status != 0) {
      if (status == -2) {
        Log("[ERROR] Unable to connect to TIOJ url");
      }
      usleep(1000000);
      continue;
    }
    Log("Recieved submission [", sub.submission_id, "]");
    RespondValidating(sub.submission_id);
    if (FetchProblem(sub) == -1) {
      usleep(1000000);
      Log("[ERROR] Unable to fetch problem meta");
      continue;
    }

    int verdict = Testsuite(sub);
    SendResult(sub, verdict, true);
  }

  return 0;
}
