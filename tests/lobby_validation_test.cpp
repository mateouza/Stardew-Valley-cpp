#include "lobby/lobby_validation.h"

#include <string>

#include "gtest/gtest.h"
#include "lobby/lobby_types.h"

namespace lobby {

TEST(PortValidationTest, AcceptsRangeLimits) {
    EXPECT_TRUE(is_valid_port("1"));
    EXPECT_TRUE(is_valid_port("8080"));
    EXPECT_TRUE(is_valid_port("65535"));
}

TEST(PortValidationTest, RejectsOutOfRange) {
    for (const char* port: {"0", "65536", "99999", "4294967296"}) {
        EXPECT_FALSE(is_valid_port(port)) << port;
    }
}

TEST(PortValidationTest, RejectsTextThatIsNotOnlyANumber) {
    for (const char* port: {"", "abc", "80a", "a80", "-1", "+80", " 80", "80 ", "8.080"}) {
        EXPECT_FALSE(is_valid_port(port)) << port;
    }
}

TEST(UsernameValidationTest, AcceptsLettersDigitsAndUnderscore) {
    EXPECT_TRUE(is_valid_username("Juanceto01"));
    EXPECT_TRUE(is_valid_username("a_b"));
    EXPECT_TRUE(is_valid_username(std::string(MAX_USERNAME_LENGTH, 'a')));
}

TEST(UsernameValidationTest, RejectsEmptyLongOrOtherCharacters) {
    EXPECT_FALSE(is_valid_username(""));
    EXPECT_FALSE(is_valid_username(std::string(MAX_USERNAME_LENGTH + 1, 'a')));
    for (const char* username: {"con espacio", "ñandu", "@dipa", "a-b"}) {
        EXPECT_FALSE(is_valid_username(username)) << username;
    }
}

}
