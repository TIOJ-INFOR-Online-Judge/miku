#ifndef SERVER_IO_H_
#define SERVER_IO_H_

#include <string>

#include "config.h"
#include "utils.h"

bool InitServerIO();

int FetchSubmission(Submission&);

int DownloadTestdata(Submission&);

int FetchProblem(Submission&);

int SendResult(Submission&, int verdict, bool done);

int SendMessage(const Submission&, const std::string&);

void RespondValidating(int submission_id);

#endif  // SERVER_IO_H_
