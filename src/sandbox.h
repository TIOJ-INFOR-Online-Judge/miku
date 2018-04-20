#ifndef SANDBOX
#define SANDBOX

#include <string>
#include <vector>

using namespace std;

class sandboxOptions{
   public:
      bool cgroup;         //--cg
      bool preserve_env;   //--full-env
      vector<string> dirs; //--dir
      string input;        //--stdin
      string output;       //--stdout
      string errout;       //--stderr
      string meta;         //--meta
      vector<string> envs; //--env
      int mem;             //--cg-mem in kilobytes
      int procs;           //--processes
      int timeout;         //--time in ms
      int file_limit;      //--file-limit number of opened files
      int fsize_limit;     //--fsize in kilobytes
      sandboxOptions() : cgroup(true), preserve_env(false), mem(0), procs(1), timeout(0), file_limit(64), fsize_limit(0) {}
};

int sandboxInit(int boxid);

int sandboxExec(int boxid, const sandboxOptions &, const string &);

int sandboxDele(int boxid);

#endif
