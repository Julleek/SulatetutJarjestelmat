#include <gtest/gtest.h>
#include "../TimeParser.h"

TEST(TimeParserTest, ValidExample) {
    EXPECT_EQ(80, time_parse("000120"));
}

TEST(TimeParserTest, BoundaryValid) {
    EXPECT_EQ(1,    time_parse("000001"));
    EXPECT_EQ(59,   time_parse("000059"));
    EXPECT_EQ(3540, time_parse("005900"));
    EXPECT_EQ(3599, time_parse("235959"));
}

TEST(TimeParserTest, BoundaryInvalid) {
    EXPECT_EQ(TIME_HOUR_ERROR, time_parse("240000"));
    EXPECT_EQ(TIME_MIN_ERROR,  time_parse("006000"));
    EXPECT_EQ(TIME_SEC_ERROR,  time_parse("000060"));
    EXPECT_EQ(TIME_HOUR_ERROR, time_parse("990000"));
}

TEST(TimeParserTest, Length) {
    EXPECT_EQ(80, time_parse("000120"));
    EXPECT_EQ(TIME_SEC_ERROR, time_parse("00012"));
    EXPECT_EQ(TIME_SEC_ERROR, time_parse("0001200"));
    EXPECT_EQ(TIME_SEC_ERROR, time_parse(""));
}

TEST(TimeParserTest, OnlyDigits) {
    EXPECT_EQ(10, time_parse("000010"));
    EXPECT_EQ(TIME_DIGIT_ERROR, time_parse("00a010"));
    EXPECT_EQ(TIME_DIGIT_ERROR, time_parse("-00010"));
    EXPECT_EQ(TIME_DIGIT_ERROR, time_parse("00 010"));
}

TEST(TimeParserTest, NullPointer) {
    EXPECT_EQ(5, time_parse("000005"));
    EXPECT_EQ(TIME_NULL_ERROR, time_parse(NULL));
}

TEST(TimeParserTest, NotZero) {
    EXPECT_EQ(1, time_parse("000001"));
    EXPECT_EQ(TIME_ZERO_ERROR, time_parse("000000"));
}

TEST(SeqParserTest, ValidSequences) {
    EXPECT_EQ(0, seq_parse("R"));
    EXPECT_EQ(0, seq_parse("RYG"));
    EXPECT_EQ(0, seq_parse("RYGRYGD"));
    EXPECT_EQ(0, seq_parse("RRRRRRRRRRRRRRRRRRR"));   // 19 merkkiä, raja-arvo
}

TEST(SeqParserTest, InvalidCharacters) {
    EXPECT_EQ(SEQ_CHAR_ERROR, seq_parse("RYX"));
    EXPECT_EQ(SEQ_CHAR_ERROR, seq_parse("ryg"));      // pienet kirjaimet
    EXPECT_EQ(SEQ_CHAR_ERROR, seq_parse("R Y"));
    EXPECT_EQ(SEQ_CHAR_ERROR, seq_parse("R,1000"));
    EXPECT_EQ(SEQ_CHAR_ERROR, seq_parse("123"));
}

TEST(SeqParserTest, EmptyAndNull) {
    EXPECT_EQ(SEQ_EMPTY_ERROR, seq_parse(""));
    EXPECT_EQ(SEQ_NULL_ERROR,  seq_parse(NULL));
}

TEST(SeqParserTest, TooLong) {
    EXPECT_EQ(SEQ_LEN_ERROR, seq_parse("RRRRRRRRRRRRRRRRRRRR"));  // 20 merkkiä
}

// https://google.github.io/googletest/reference/testing.html
// https://google.github.io/googletest/reference/assertions.html
