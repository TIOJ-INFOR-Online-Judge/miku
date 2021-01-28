#ifndef SERVER_IO
#define SERVER_IO

#include "config.h"
#include "utils.h"

bool InitServerIO();

int fetchSubmission(submission&);

int downloadTestdata(submission&);

int fetchProblem(submission&);

int sendResult(submission&, int verdict, bool done);

int sendMessage(const submission&, const std::string&);

void respondValidating(int submission_id);

#endif
