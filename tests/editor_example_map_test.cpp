#include "editor/model/map_file.h"
#include "editor/model/map_validation.h"
#include "gtest/gtest.h"

namespace editor {

TEST(ExampleMapTest, RepositoryExampleLoadsWithoutProblems) {
    const WorldMap map = load_map(EXAMPLE_MAP_PATH);

    EXPECT_TRUE(validate(map).empty());
}

}
