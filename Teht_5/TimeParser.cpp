#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "TimeParser.h"

// time format: HHMMSS (6 characters)
int time_parse(const char *time) {

	// Check that string is not null
	if (time == NULL) return TIME_NULL_ERROR;

	// Must be exactly 6 characters
	if (strlen(time) != 6) return TIME_SEC_ERROR;

	// Only digits allowed (also rejects "-", spaces, letters)
	for (int i = 0; i < 6; i++) {
		if (!isdigit((unsigned char)time[i])) return TIME_DIGIT_ERROR;
	}

	// Parse values, e.g. 124033 -> 12h 40min 33sec (string is not modified)
	int values[3];
	values[0] = (time[0] - '0') * 10 + (time[1] - '0'); // hours
	values[1] = (time[2] - '0') * 10 + (time[3] - '0'); // minutes
	values[2] = (time[4] - '0') * 10 + (time[5] - '0'); // seconds

	// Boundary checks
	if (values[0] < 0 || values[0] > 23) return TIME_HOUR_ERROR;
	if (values[1] < 0 || values[1] > 59) return TIME_MIN_ERROR;
	if (values[2] < 0 || values[2] > 59) return TIME_SEC_ERROR;

	// Minutes and seconds only
	int seconds = values[1] * 60 + values[2];

	// Zero seconds makes the timer pointless
	if (seconds == 0) return TIME_ZERO_ERROR;

	return seconds;
}

int seq_parse(const char *seq) {
	if (seq == NULL) return SEQ_NULL_ERROR;

	size_t len = strlen(seq);
	if (len == 0)  return SEQ_EMPTY_ERROR;
	if (len > 19)  return SEQ_LEN_ERROR;

	for (size_t i = 0; i < len; i++) {
		char c = seq[i];
		if (c != 'R' && c != 'Y' && c != 'G' && c != 'D') return SEQ_CHAR_ERROR;
	}
	return 0;
}