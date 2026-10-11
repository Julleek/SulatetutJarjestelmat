#ifndef TIMEPARSER_H
#define TIMEPARSER_H

//Error codes

#define TIME_NULL_ERROR   -2
#define TIME_DIGIT_ERROR  -3
#define TIME_HOUR_ERROR   -4
#define TIME_MIN_ERROR    -5
#define TIME_SEC_ERROR    -6
#define TIME_ZERO_ERROR   -7
#define SEQ_NULL_ERROR    -8
#define SEQ_EMPTY_ERROR   -9
#define SEQ_LEN_ERROR     -10
#define SEQ_CHAR_ERROR    -11

using namespace std;

int time_parse(const char *time);
int seq_parse(const char *seq);
#endif
