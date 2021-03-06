#include "testsuite.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "batchjudge.h"
#include "config.h"
#include "sandbox.h"
#include "server_io.h"
#include "utils.h"

Results Compile(const Submission& target, int boxid, int spBoxid);
void Eval(Submission& sub, int td, int boxid, int spBoxid);
void GetExitStatus(Submission& sub, int td);

int MAXPARNUM = 1;
int BOXOFFSET = 10;
bool AGGUPDATE = false;

Results Testsuite(Submission& sub) {
  Execute("rm", "-f", "./testzone/*");
  const int testBoxid = BOXOFFSET + 0, spBoxid = BOXOFFSET + 1;
  auto DelSandboxes = [&]() {
    SandboxDele(testBoxid);
    SandboxDele(spBoxid);
  };

  SandboxInit(testBoxid);
  SandboxInit(spBoxid);
  Results status = Compile(sub, testBoxid, spBoxid);
  if (status != OK) {
    DelSandboxes();
    return status;
  }

  // anyway, only have batch judge right now
  std::map<pid_t, int> proc;
  int procnum = 0;
  int problem_id = sub.problem_id;

  for (size_t i = 0; i < sub.tds.size(); ++i) {
    auto& nowtd = sub.tds[i];
    while (procnum >= MAXPARNUM) {
      int status;
      pid_t cid = waitpid(-1, &status, 0);
      if (cid == -1) {
        perror("[ERROR] in testsuite,  `waitpid()` failed :");
        DelSandboxes();
        return ER;
      }
      int td = proc[cid];
      Eval(sub, td, BOXOFFSET + 10 + td, spBoxid);
      if (AGGUPDATE) {
        SendResult(sub, OK, false);
      }
      // cerr << "td" << td << " : " << sub.verdict[td] << endl;
      SandboxDele(BOXOFFSET + 10 + td);
      --procnum;
    }

    if (procnum < MAXPARNUM) {
      // batch judge
      int memlim = nowtd.mem_limit;
      if (sub.lang == "haskell") memlim = memlim * 5 + 24 * 1024;
      pid_t pid = fork();
      if (pid == -1) {
        perror("[ERROR] in testsuite, `fork()` failed :");
        DelSandboxes();
        return ER;
      }
      if (pid == 0) {
        // child proc
        int status =
            BatchJudge(problem_id, i, BOXOFFSET + 10 + i, nowtd.time_limit,
                       memlim, nowtd.output_limit, testBoxid, sub.lang);
        exit(status);
      }
      proc[pid] = i;
      ++procnum;
    }
  }
  while (procnum >= 1) {
    int status;
    pid_t cid = waitpid(-1, &status, 0);
    if (cid == -1) {
      perror("[ERROR] in testsuite,  `waitpid()` failed :");
      DelSandboxes();
      return ER;
    }
    const int td = proc[cid];
    // sub.verdict[td] = eval(problem_id, td);
    Eval(sub, td, BOXOFFSET + 10 + td, spBoxid);
    if (AGGUPDATE) {
      SendResult(sub, OK, false);
    }
    // cerr << "td" << td << " : " << sub.verdict[td] << endl;
    SandboxDele(BOXOFFSET + 10 + td);
    --procnum;
  }
  // clear box-10
  DelSandboxes();
  return OK;
}

void SetExitStatus(Submission& sub, int td) {
  std::ifstream fin("./testzone/META" + PadInt(td));
  std::string line;
  std::map<std::string, std::string> META;
  while (!fin.eof() && getline(fin, line)) {
    for (size_t i = 0; i < line.size(); ++i)
      if (line[i] == ':') line[i] = ' ';
    std::istringstream in(line);
    std::string a, b;
    in >> a >> b;
    META[a] = b;
  }

  auto& nowtd = sub.tds[td];
  // mem_used
  nowtd.mem = std::atoi(META["max-rss"].c_str());
  // time_used
  nowtd.time = 1000 * std::atof(META["time"].c_str());
  // verdict
  if (META["status"] == "") {
    nowtd.verdict = OK;
  } else if (META["status"] == "TO") {
    nowtd.verdict = TLE;
  } else if (META["status"] == "SG") {
    switch (std::atoi(META["exitsig"].c_str())) {
      case 11:
        nowtd.verdict = MLE;
        break;
      default:
        nowtd.verdict = RE;
    }
  } else if (META["status"] == "RE") {
    nowtd.verdict = RE;
  } else {
    nowtd.verdict = ER;
  }
}

void Eval(Submission& sub, int td, int boxid, int spBoxid) {
  auto& nowtd = sub.tds[td];
  int problem_id = sub.problem_id;
  SetExitStatus(sub, td);
  if (nowtd.verdict != OK) {
    return;
  }

  if (sub.problem_type == 1) {
    std::string cmd;
    {
      cmd += BoxPath(spBoxid) + "sj.out ";
      cmd += BoxPath(boxid) + "output ";
      cmd += TdInput(problem_id, td) + ' ';
      cmd += TdOutput(problem_id, td) + ' ';
      cmd += (sub.std.empty() ? sub.lang : sub.std) + ' ';

      std::string codefile = BoxPath(boxid) + "code";
      cmd += codefile;
      std::ofstream fout(codefile);
      fout.write(sub.code.c_str(), sub.code.size());
    }

    FILE* Pipe = popen(cmd.c_str(), "r");
    int result = 1;
    fscanf(Pipe, "%d", &result);
    pclose(Pipe);
    Log("[special judge] :", cmd);
    Log("[special judge] td:", td, " result:", result);
    nowtd.verdict = result ? WA : AC;
    return;
  }

  Results status = AC;
  // solution output
  std::fstream tsol(TdOutput(problem_id, td));
  // user output
  std::string output_file = BoxPath(boxid) + "output";
  {  // check if output is regular file
    struct stat output_stat;
    if (stat(output_file.c_str(), &output_stat) < 0 ||
        !S_ISREG(output_stat.st_mode)) {
      nowtd.verdict = WA;
      return;
    }
  }

  std::fstream mout(output_file);
  while (true) {
    if (tsol.eof() != mout.eof()) {
      while (tsol.eof() != mout.eof()) {
        std::string s;
        if (tsol.eof()) {
          getline(mout, s);
        } else {
          getline(tsol, s);
        }
        s.erase(s.find_last_not_of(" \n\r\t") + 1);
        if (s != "") {
          status = WA;
          break;
        }
      }
      break;
    }
    if (tsol.eof() && mout.eof()) {
      break;
    }
    std::string s, t;
    getline(tsol, s);
    getline(mout, t);
    s.erase(s.find_last_not_of(" \n\r\t") + 1);
    t.erase(t.find_last_not_of(" \n\r\t") + 1);
    if (s != t) {
      status = WA;
      break;
    }
  }
  nowtd.verdict = status;
}

Results Compile(const Submission& target, int boxid, int spBoxid) {
  std::string boxdir = BoxPath(boxid);

  std::ofstream fout;
  if (target.lang == "c++") {
    fout.open(boxdir + "main.cpp");
  } else if (target.lang == "c") {
    fout.open(boxdir + "main.c");
  } else if (target.lang == "haskell") {
    fout.open(boxdir + "main.hs");
  } else if (target.lang.find_first_of("python") == 0) {
    fout.open(boxdir + "main.py");
  } else {
    return CE;
  }
  fout.write(target.code.c_str(), target.code.size());
  fout.close();

  if (target.problem_type == 2) {
    fout.open(boxdir + "lib" + PadInt(target.problem_id, 4) + ".h");
    fout << target.interlib;
    fout.close();
  }

  std::vector<std::string> args;
  if (target.lang == "c++") {
    args = {"/usr/bin/env", "g++", "./main.cpp", "-o",
            "./main.out",   "-O2", "-w"};
  } else if (target.lang == "c") {
    if (target.std == "" || target.std == "c90") {
      args = {"/usr/bin/env", "gcc",   "./main.c", "-o", "./main.out",
              "-O2",          "-ansi", "-lm",      "-w"};
    } else {
      args = {"/usr/bin/env", "gcc", "./main.c", "-o",
              "./main.out",   "-O2", "-lm",      "-w"};
    }
  } else if (target.lang == "haskell") {
    args = {"/usr/bin/env", "ghc",     "./main.hs", "-o", "./main.out",
            "-O",           "-tmpdir", ".",         "-w"};
  } else if (target.lang == "python2") {
    args = {"/usr/bin/env", "python2", "-m", "py_compile", "main.py"};
  } else if (target.lang == "python3") {
    args = {"/usr/bin/env", "python3", "-c",
            R"(import py_compile;py_compile.compile('main.py','main.pyc'))"};
  }
  if (!target.std.empty() && target.std != "c90") {
    args.push_back("-std=" + target.std);
  }

  SandboxOptions opt;
  opt.dirs.push_back("/etc/alternatives");
  if (target.lang == "haskell") opt.dirs.push_back("/var");
  opt.procs = 50;
  opt.preserve_env = true;
  opt.errout = "compile_error";
  opt.output = "/dev/null";
  opt.timeout = 30 * 1000;
  opt.meta = "./testzone/metacomp";
  opt.fsize_limit = 32768;

  SandboxExec(boxid, opt, args);
  std::string compiled_target;
  if (target.lang.find_first_of("python") == 0)
    compiled_target = "main.pyc";
  else
    compiled_target = "main.out";
  if (access((boxdir + compiled_target).c_str(), F_OK) == -1) {
    if (access((boxdir + "compile_error").c_str(), F_OK) == 0) {
      const int MAX_MSG_LENGTH = 3000;
      std::ifstream compile_error_msg((boxdir + "compile_error").c_str());
      char cerr_msg_cstring[MAX_MSG_LENGTH + 5] = "";
      compile_error_msg.read(cerr_msg_cstring, MAX_MSG_LENGTH + 1);
      std::string cerr_msg(cerr_msg_cstring);
      if (cerr_msg.size() > MAX_MSG_LENGTH) {
        cerr_msg = std::regex_replace(
            cerr_msg.substr(0, MAX_MSG_LENGTH),
            std::regex("(^|\\n)In file included "
                       "from[\\S\\s]*?((\\n\\./main\\.c)|($))"),
            "$1[Error messages from headers removed]$2");
        cerr_msg += "\n(Error message truncated after " +
                    std::to_string(MAX_MSG_LENGTH) + " Bytes.)";
      } else {
        cerr_msg = std::regex_replace(
            cerr_msg,
            std::regex("(^|\\n)In file included "
                       "from[\\S\\s]*?((\\n\\./main\\.c)|($))"),
            "$1[Error messages from headers removed]$2");
      }
      SendMessage(target, cerr_msg);
    }
    return CE;
  }

  if (target.problem_type == 1) {
    std::string spBoxdir(BoxPath(spBoxid));

    fout.open(spBoxdir + "sj.cpp");
    fout << target.sjcode;
    fout.close();

    std::vector<std::string> args{"/usr/bin/env", "g++", "./sj.cpp",  "-o",
                                  "./sj.out",     "-O2", "-std=c++14"};
    opt.meta = "./testzone/metacompsj";

    SandboxExec(spBoxid, opt, args);
    if (access((spBoxdir + "sj.out").c_str(), F_OK) == -1) {
      return ER;
    }
  }

  return OK;
}
