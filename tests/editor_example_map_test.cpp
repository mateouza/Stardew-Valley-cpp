#include "editor/model/map_file.h"
#include "editor/model/map_validation.h"
#include "gtest/gtest.h"

namespace editor {

// maps/example.toml es el mapa que se le pasa al resto del grupo: tiene que abrirse en el editor
// sin ningún problema.
TEST(ExampleMapTest, RepositoryExampleLoadsWithoutProblems) {
    const WorldMap map = load_map(EXAMPLE_MAP_PATH);

    EXPECT_TRUE(validate(map).empty());
}

}
