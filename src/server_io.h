#ifndef SERVER_IO_H_
#define SERVER_IO_H_

#include <string>

#include "config.h"
#include "utils.h"

bool InitServerIO();

int fetchSubmission(Submission&);

int downloadTestdata(Submission&);

int fetchProblem(Submission&);

int sendResult(Submission&, int verdict, bool done);

int sendMessage(const Submission&, const std::string&);

void respondValidating(int submission_id);

#endif  // SERVER_IO_H_
