#ifndef POPPEL_TEST_CORE_OPERATIONS_HPP
#define POPPEL_TEST_CORE_OPERATIONS_HPP

#include <catch2/catch.hpp>

#include <poppel/core/operations.hpp>

TEST_CASE("Poppel operations", "[operation]") {
    using namespace poppel;
    using namespace poppel::core;

    // Prepare test data.
    FileStates fs_r  { FileOpenState::ReadOnly };
    FileStates fs_rw { FileOpenState::ReadWrite };
    FileStates fs_c  { FileOpenState::Closed };

    auto temp_dir = std::filesystem::temp_directory_path();
    INFO("Temp directory path is " << temp_dir);
    auto pfile1   = temp_dir / "file1.poppel";
    auto pfile2   = temp_dir / "file2.poppel";
    auto pfile3   = temp_dir / "file3.poppel";
    auto tfile1   = temp_dir / "file1.txt";
    auto cleanup  = [&]() {
        std::filesystem::remove_all(pfile1);
        std::filesystem::remove_all(pfile2);
        std::filesystem::remove_all(pfile3);
        std::filesystem::remove_all(tfile1);
    };
    ScopeGuard cleanup_guard { cleanup };

    // Tests.
    SECTION("Utility functions.") {
        CHECK_NOTHROW(assert_file_open(fs_r));
        CHECK_NOTHROW(assert_file_open(fs_rw));
        CHECK_THROWS (assert_file_open(fs_c));

        CHECK_THROWS (assert_file_writable(fs_r));
        CHECK_NOTHROW(assert_file_writable(fs_rw));
        CHECK_THROWS (assert_file_writable(fs_c));

        CHECK(!is_valid_node_normalized_relpath(""));
        CHECK(!is_valid_node_normalized_relpath("."));
        CHECK(!is_valid_node_normalized_relpath(".."));
        CHECK(!is_valid_node_normalized_relpath("/"));
        CHECK(!is_valid_node_normalized_relpath("//"));
        CHECK(!is_valid_node_normalized_relpath("\\"));
        CHECK(!is_valid_node_normalized_relpath("C:\\"));
        CHECK(!is_valid_node_normalized_relpath("c/"));
        CHECK(!is_valid_node_normalized_relpath("../c"));
        CHECK( is_valid_node_normalized_relpath("c"));
        CHECK( is_valid_node_normalized_relpath("c/c"));

        CHECK_THROWS (assert_is_valid_node_normalized_relpath(""));
        CHECK_NOTHROW(assert_is_valid_node_normalized_relpath("c"));

        {
            Node node1 { NodeMeta { 1, NodeType::File }, };
            Node node2 { NodeMeta { 1, NodeType::Group }, };
            Node node3 { NodeMeta { 1, NodeType::Dataset }, };
            Node node4 { NodeMeta { 1, NodeType::Raw }, };
            Node node5 { NodeMeta { 1, NodeType::Unknown }, };

            CHECK( is_node_group(node1));
            CHECK( is_node_group(node2));
            CHECK(!is_node_group(node3));
            CHECK(!is_node_group(node4));
            CHECK(!is_node_group(node5));

            CHECK(!is_node_dataset(node1));
            CHECK(!is_node_dataset(node2));
            CHECK( is_node_dataset(node3));
            CHECK(!is_node_dataset(node4));
            CHECK(!is_node_dataset(node5));
        }

        cleanup();
        CHECK_NOTHROW(assert_not_exists(pfile1));
        CHECK_NOTHROW(assert_not_exists(tfile1));
        CHECK_THROWS (assert_exists_directory(pfile1));
        CHECK_THROWS (assert_exists_directory(tfile1));

        std::filesystem::create_directories(pfile1);
        { std::ofstream ofs(tfile1); REQUIRE(ofs.is_open()); }
        CHECK_THROWS (assert_not_exists(pfile1));
        CHECK_THROWS (assert_not_exists(tfile1));
        CHECK_NOTHROW(assert_exists_directory(pfile1));
        CHECK_THROWS (assert_exists_directory(tfile1));
    }

    SECTION("Node operations.") {
        cleanup();
        CHECK_THROWS(get_file_node(pfile1));
        // TODO.
    }
}

#endif
