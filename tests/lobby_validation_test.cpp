#include "lobby/lobby_validation.h"

#include <cstddef>
#include <optional>
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

std::string repeat(const std::string& text, std::size_t times) {
    std::string result;
    for (std::size_t i = 0; i < times; ++i) {
        result += text;
    }
    return result;
}

TEST(NameValidationTest, CountsAdmittedCharactersNotBytes) {
    EXPECT_EQ(count_name_characters(""), 0u);
    EXPECT_EQ(count_name_characters("Peñón"), 5u);
    EXPECT_EQ(count_name_characters("áéíóúüñÁÉÍÓÚÜÑ"), 14u);
    EXPECT_EQ(count_name_characters("Ana-Lía O'Brien 2"), 17u);
}

TEST(NameValidationTest, RejectsMalformedUtf8) {
    const std::string stray_continuations(1000, '\x80');
    const std::string truncated_at_end = "\xC3";
    const std::string truncated_after_text = "Pe\xC3";
    const std::string missing_continuation = std::string("\xC3") + "A";
    const std::string overlong_letter = "\xC1\xA1";
    const std::string overlong_null = "\xC0\x80";
    const std::string surrogate_half = "\xED\xA0\x80";

    for (const std::string& name:
         {stray_continuations, truncated_at_end, truncated_after_text, missing_continuation,
          overlong_letter, overlong_null, surrogate_half}) {
        EXPECT_EQ(count_name_characters(name), std::nullopt);
        EXPECT_FALSE(is_valid_game_name(name));
        EXPECT_FALSE(is_valid_character_name(name));
    }
}

TEST(NameValidationTest, RejectsCharactersOutsideTheList) {
    const std::string non_breaking_space = std::string("\xC2\xA0") + "a";
    const std::string null_character(1, '\0');

    for (const std::string& name:
         {std::string("Precio €"), std::string("Granja 🙂"), std::string("ç"), std::string("a_b"),
          std::string("@dipa"), std::string("Tab\there"), std::string("Línea\n"),
          std::string("\x7F"), non_breaking_space, null_character}) {
        EXPECT_EQ(count_name_characters(name), std::nullopt) << name;
    }
}

TEST(NameValidationTest, TrimsOnlySpaces) {
    EXPECT_EQ(trim_spaces("  Granja Girasol  "), "Granja Girasol");
    EXPECT_EQ(trim_spaces("   "), "");
    EXPECT_TRUE(is_valid_character_name("  Juana  "));
    EXPECT_FALSE(is_valid_character_name("\tJuana"));
    EXPECT_FALSE(is_valid_character_name(std::string("\xC2\xA0") + "Juana"));
    EXPECT_FALSE(is_valid_character_name(""));
    EXPECT_FALSE(is_valid_character_name("    "));
}

TEST(NameValidationTest, CharacterNameLimitIsCountedInCharacters) {
    EXPECT_TRUE(is_valid_character_name(std::string(MAX_CHARACTER_NAME_LENGTH, 'a')));
    EXPECT_FALSE(is_valid_character_name(std::string(MAX_CHARACTER_NAME_LENGTH + 1, 'a')));
    EXPECT_TRUE(is_valid_character_name(repeat("ñ", MAX_CHARACTER_NAME_LENGTH)));
    EXPECT_FALSE(is_valid_character_name(repeat("ñ", MAX_CHARACTER_NAME_LENGTH + 1)));
}

TEST(NameValidationTest, GameNameLimitIsCountedInCharacters) {
    EXPECT_TRUE(is_valid_game_name(std::string(MAX_GAME_NAME_LENGTH, 'g')));
    EXPECT_FALSE(is_valid_game_name(std::string(MAX_GAME_NAME_LENGTH + 1, 'g')));
    EXPECT_TRUE(is_valid_game_name(repeat("é", MAX_GAME_NAME_LENGTH)));
    EXPECT_FALSE(is_valid_game_name(repeat("é", MAX_GAME_NAME_LENGTH + 1)));
}

}
