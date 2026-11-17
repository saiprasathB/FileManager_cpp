// tests/test_all.cpp
// Google Test suite for Smart File Explorer.
// Build: see CMakeLists.txt. Run: ./run_tests
//
// Coverage targets (line coverage per file):
//   FileManager         >= 85%
//   DirectoryManager    >= 85%
//   ClipboardManager    >= 90%
//   SearchEngine        >= 85%
//   StorageAnalyzer     >= 90%
//   FileViewer          >= 85%
//   ContextMenu         >= 95%
//   FileSystemItem+FileItem+FolderItem >= 85%
//   FileExplorer        >= 75%
//   (main.cpp is not linked into tests; excluded from coverage)

#include <gtest/gtest.h>

// Include dependency headers normally so their `private` stays private.
#include "DirectoryManager.h"
#include "ContextMenu.h"
#include "FileManager.h"
#include "ClipboardManager.h"
#include "SearchEngine.h"
#include "FileViewer.h"
#include "StorageAnalyzer.h"
#include "FileSystemItem.h"
#include "FileItem.h"
#include "FolderItem.h"

// The FileExplorer test seam. Only FileExplorer.h is affected.
#define private public
#include "FileExplorer.h"
#undef private

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

std::string uniqueSuffix() {
    static std::atomic<int> n{0};
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::to_string(::getpid()) + "_" + std::to_string(now) + "_" +
           std::to_string(n.fetch_add(1));
}

// Redirect std::cout for the lifetime of the object; restores on destruction.
class CoutCapture {
public:
    CoutCapture() : old_(std::cout.rdbuf(buf_.rdbuf())) {}
    ~CoutCapture() { std::cout.rdbuf(old_); }
    std::string str() const { return buf_.str(); }
private:
    std::ostringstream buf_;
    std::streambuf* old_;
};

// Redirect std::cin to a fixed string; restores on destruction.
class CinRedirect {
public:
    explicit CinRedirect(const std::string& input)
        : buf_(input), old_(std::cin.rdbuf(buf_.rdbuf())) {}
    ~CinRedirect() { std::cin.rdbuf(old_); }
private:
    std::istringstream buf_;
    std::streambuf* old_;
};

// Creates a fresh temp dir + workspace/ subdir per test.
class TempWorkspaceFixture : public ::testing::Test {
protected:
    fs::path tempRoot_;
    fs::path workspace_;

    void SetUp() override {
        tempRoot_ = fs::temp_directory_path() / ("sfx_test_" + uniqueSuffix());
        workspace_ = tempRoot_ / "workspace";
        fs::create_directories(workspace_);
    }
    void TearDown() override {
        std::error_code ec;
        fs::remove_all(tempRoot_, ec);
    }

    fs::path touchFile(const fs::path& dir, const std::string& name,
                       const std::string& content = "x") {
        fs::path p = dir / name;
        std::ofstream f(p);
        f << content;
        return p;
    }
    fs::path mkdir(const fs::path& dir, const std::string& name) {
        fs::path p = dir / name;
        fs::create_directories(p);
        return p;
    }
};

// Adds a chdir to tempRoot_ during the test (needed for FileExplorer and
// DirectoryManager, whose initialize() uses fs::current_path()).
class ChdirFixture : public TempWorkspaceFixture {
protected:
    fs::path originalCwd_;
    void SetUp() override {
        TempWorkspaceFixture::SetUp();
        originalCwd_ = fs::current_path();
        fs::current_path(tempRoot_);
    }
    void TearDown() override {
        fs::current_path(originalCwd_);
        TempWorkspaceFixture::TearDown();
    }
};

} // namespace

// ---------------------------------------------------------------------------
// FileSystemItem / FileItem / FolderItem
// ---------------------------------------------------------------------------

TEST(FileItemTest, GetType_Name_Size_Extension) {
    auto dir = fs::temp_directory_path() / ("sfx_item_" + uniqueSuffix());
    fs::create_directories(dir);
    fs::path f = dir / "hello.txt";
    { std::ofstream o(f); o << "abcd"; }

    FileItem item(f);
    EXPECT_EQ(item.getType(), "File");
    EXPECT_EQ(item.getName(), "hello.txt");
    EXPECT_EQ(item.getExtension(), ".txt");
    EXPECT_EQ(item.getSize(), 4u);
    EXPECT_EQ(item.getPath(), f);

    std::error_code ec;
    fs::remove_all(dir, ec);
}

TEST(FolderItemTest, GetType_Count_Empty) {
    auto dir = fs::temp_directory_path() / ("sfx_fitem_" + uniqueSuffix());
    fs::create_directories(dir);

    FolderItem empty(dir);
    EXPECT_EQ(empty.getType(), "Folder");
    EXPECT_EQ(empty.getItemCount(), 0u);
    EXPECT_TRUE(empty.isEmpty());

    { std::ofstream o(dir / "a"); o << "x"; }
    { std::ofstream o(dir / "b"); o << "x"; }

    FolderItem nonEmpty(dir);
    EXPECT_EQ(nonEmpty.getItemCount(), 2u);
    EXPECT_FALSE(nonEmpty.isEmpty());

    std::error_code ec;
    fs::remove_all(dir, ec);
}

TEST(FolderItemTest, NonexistentFolderReturnsZero) {
    FolderItem missing(fs::temp_directory_path() / ("sfx_missing_" + uniqueSuffix()));
    CoutCapture out;
    EXPECT_EQ(missing.getItemCount(), 0u);
    EXPECT_TRUE(missing.isEmpty());
}

// ---------------------------------------------------------------------------
// FileManager
// ---------------------------------------------------------------------------

class FileManagerTest : public TempWorkspaceFixture {
protected:
    std::unique_ptr<FileManager> fm_;
    void SetUp() override {
        TempWorkspaceFixture::SetUp();
        fm_ = std::make_unique<FileManager>(workspace_);
    }
};

TEST_F(FileManagerTest, CreateFile_Valid) {
    CoutCapture out;
    EXPECT_TRUE(fm_->createFile(workspace_, "a.txt"));
    EXPECT_TRUE(fs::exists(workspace_ / "a.txt"));
    EXPECT_NE(out.str().find("File created successfully"), std::string::npos);
}

TEST_F(FileManagerTest, CreateFile_InvalidNames) {
    CoutCapture out;
    for (auto& bad : {std::string(""), std::string("."), std::string(".."),
                      std::string("a/b"), std::string("a\\b")}) {
        EXPECT_FALSE(fm_->createFile(workspace_, bad)) << "name=" << bad;
    }
    EXPECT_NE(out.str().find("Invalid file name"), std::string::npos);
}

TEST_F(FileManagerTest, CreateFile_Duplicate) {
    CoutCapture out;
    ASSERT_TRUE(fm_->createFile(workspace_, "dup.txt"));
    EXPECT_FALSE(fm_->createFile(workspace_, "dup.txt"));
    EXPECT_NE(out.str().find("already exists"), std::string::npos);
}

TEST_F(FileManagerTest, CreateFile_InvalidDirectory) {
    CoutCapture out;
    EXPECT_FALSE(fm_->createFile(tempRoot_, "x"));          // outside workspace
    EXPECT_FALSE(fm_->createFile(workspace_ / "nope", "x")); // doesn't exist
    EXPECT_NE(out.str().find("Invalid destination"), std::string::npos);
}

TEST_F(FileManagerTest, CreateFolder_Valid) {
    CoutCapture out;
    EXPECT_TRUE(fm_->createFolder(workspace_, "sub"));
    EXPECT_TRUE(fs::is_directory(workspace_ / "sub"));
}

TEST_F(FileManagerTest, CreateFolder_InvalidAndDuplicate) {
    CoutCapture out;
    EXPECT_FALSE(fm_->createFolder(workspace_, ".."));
    ASSERT_TRUE(fm_->createFolder(workspace_, "sub"));
    EXPECT_FALSE(fm_->createFolder(workspace_, "sub"));
}

TEST_F(FileManagerTest, Rename_Valid) {
    CoutCapture out;
    fs::path p = touchFile(workspace_, "old.txt");
    EXPECT_TRUE(fm_->renameItem(p, "new.txt"));
    EXPECT_TRUE(fs::exists(workspace_ / "new.txt"));
    EXPECT_FALSE(fs::exists(p));
}

TEST_F(FileManagerTest, Rename_InvalidName) {
    CoutCapture out;
    fs::path p = touchFile(workspace_, "a.txt");
    for (auto& bad : {std::string(""), std::string("."), std::string("a/b")})
        EXPECT_FALSE(fm_->renameItem(p, bad));
}

TEST_F(FileManagerTest, Rename_Nonexistent) {
    CoutCapture out;
    EXPECT_FALSE(fm_->renameItem(workspace_ / "ghost", "x"));
}

TEST_F(FileManagerTest, Rename_DuplicateTarget) {
    CoutCapture out;
    fs::path a = touchFile(workspace_, "a.txt");
    touchFile(workspace_, "b.txt");
    EXPECT_FALSE(fm_->renameItem(a, "b.txt"));
}

TEST_F(FileManagerTest, Rename_RootRefused) {
    CoutCapture out;
    EXPECT_FALSE(fm_->renameItem(workspace_, "renamed"));
}

TEST_F(FileManagerTest, Rename_OutsideRefused) {
    CoutCapture out;
    fs::path outside = touchFile(tempRoot_, "o.txt");
    EXPECT_FALSE(fm_->renameItem(outside, "x"));
}

TEST_F(FileManagerTest, Delete_Valid) {
    CoutCapture out;
    fs::path p = touchFile(workspace_, "gone.txt");
    EXPECT_TRUE(fm_->deleteItem(p));
    EXPECT_FALSE(fs::exists(p));
}

TEST_F(FileManagerTest, Delete_Nonexistent) {
    CoutCapture out;
    EXPECT_FALSE(fm_->deleteItem(workspace_ / "ghost"));
}

TEST_F(FileManagerTest, Delete_RootRefused) {
    CoutCapture out;
    EXPECT_FALSE(fm_->deleteItem(workspace_));
}

TEST_F(FileManagerTest, Delete_OutsideRefused) {
    CoutCapture out;
    fs::path outside = touchFile(tempRoot_, "o.txt");
    EXPECT_FALSE(fm_->deleteItem(outside));
}

TEST_F(FileManagerTest, Copy_File) {
    CoutCapture out;
    fs::path src = touchFile(workspace_, "a.txt", "content");
    fs::path dst = mkdir(workspace_, "dest");
    EXPECT_TRUE(fm_->copyItem(src, dst));
    EXPECT_TRUE(fs::exists(dst / "a.txt"));
}

TEST_F(FileManagerTest, Copy_DuplicateTarget) {
    CoutCapture out;
    fs::path src = touchFile(workspace_, "a.txt");
    fs::path dst = mkdir(workspace_, "dest");
    touchFile(dst, "a.txt");
    EXPECT_FALSE(fm_->copyItem(src, dst));
}

TEST_F(FileManagerTest, Copy_OutsideSource) {
    CoutCapture out;
    fs::path outside = touchFile(tempRoot_, "o.txt");
    fs::path dst = mkdir(workspace_, "dest");
    EXPECT_FALSE(fm_->copyItem(outside, dst));
}

TEST_F(FileManagerTest, Copy_OutsideDest) {
    CoutCapture out;
    fs::path src = touchFile(workspace_, "a.txt");
    EXPECT_FALSE(fm_->copyItem(src, tempRoot_));
}

TEST_F(FileManagerTest, Copy_RootRefused) {
    CoutCapture out;
    EXPECT_FALSE(fm_->copyItem(workspace_, workspace_));
}

// Characterization test for the known bug in FileManager::copyItem().
// Once the `fs::relative(...).begin() != .end()` guard is replaced with a
// proper component-walk containment test, change this to EXPECT_TRUE.
TEST_F(FileManagerTest, Copy_Folder_DocumentsCurrentBug) {
    CoutCapture out;
    fs::path src = mkdir(workspace_, "folderA");
    touchFile(src, "f.txt");
    fs::path dst = mkdir(workspace_, "folderB");
    EXPECT_FALSE(fm_->copyItem(src, dst))
        << "BUG: if this passes, the folder-copy guard has been fixed -- "
           "flip assertion to EXPECT_TRUE";
}

TEST_F(FileManagerTest, Move_File) {
    CoutCapture out;
    fs::path src = touchFile(workspace_, "a.txt");
    fs::path dst = mkdir(workspace_, "dest");
    EXPECT_TRUE(fm_->moveItem(src, dst));
    EXPECT_TRUE(fs::exists(dst / "a.txt"));
    EXPECT_FALSE(fs::exists(src));
}

TEST_F(FileManagerTest, Move_DuplicateTarget) {
    CoutCapture out;
    fs::path src = touchFile(workspace_, "a.txt");
    fs::path dst = mkdir(workspace_, "dest");
    touchFile(dst, "a.txt");
    EXPECT_FALSE(fm_->moveItem(src, dst));
}

TEST_F(FileManagerTest, Move_OutsideSource) {
    CoutCapture out;
    fs::path outside = touchFile(tempRoot_, "o.txt");
    fs::path dst = mkdir(workspace_, "dest");
    EXPECT_FALSE(fm_->moveItem(outside, dst));
}

TEST_F(FileManagerTest, Move_RootRefused) {
    CoutCapture out;
    EXPECT_FALSE(fm_->moveItem(workspace_, workspace_));
}

// Same characterization note as Copy_Folder_DocumentsCurrentBug.
TEST_F(FileManagerTest, Move_Folder_DocumentsCurrentBug) {
    CoutCapture out;
    fs::path src = mkdir(workspace_, "folderA");
    fs::path dst = mkdir(workspace_, "folderB");
    EXPECT_FALSE(fm_->moveItem(src, dst))
        << "BUG: if this passes, the folder-move guard has been fixed -- "
           "flip assertion to EXPECT_TRUE";
}

TEST_F(FileManagerTest, ShowProperties_File) {
    CoutCapture out;
    fs::path p = touchFile(workspace_, "a.txt", "hello");
    fm_->showProperties(p);
    EXPECT_NE(out.str().find("PROPERTIES"), std::string::npos);
    EXPECT_NE(out.str().find("Size: 5"), std::string::npos);
}

TEST_F(FileManagerTest, ShowProperties_Folder) {
    CoutCapture out;
    fs::path d = mkdir(workspace_, "d");
    touchFile(d, "one");
    touchFile(d, "two");
    fm_->showProperties(d);
    EXPECT_NE(out.str().find("Items inside: 2"), std::string::npos);
}

TEST_F(FileManagerTest, ShowProperties_Invalid) {
    CoutCapture out;
    fm_->showProperties(workspace_ / "ghost");
    EXPECT_NE(out.str().find("Invalid item"), std::string::npos);
}

TEST_F(FileManagerTest, WriteFile_Valid) {
    CoutCapture out;
    fs::path p = touchFile(workspace_, "w.txt", "old");
    EXPECT_TRUE(fm_->writeFile(p, "brand new"));
    std::ifstream f(p); std::stringstream ss; ss << f.rdbuf();
    EXPECT_EQ(ss.str(), "brand new");
}

TEST_F(FileManagerTest, WriteFile_Invalid) {
    CoutCapture out;
    EXPECT_FALSE(fm_->writeFile(workspace_ / "ghost.txt", "x"));
    fs::path d = mkdir(workspace_, "d");
    EXPECT_FALSE(fm_->writeFile(d, "x"));
}

TEST_F(FileManagerTest, AppendFile_Valid) {
    CoutCapture out;
    fs::path p = touchFile(workspace_, "a.txt", "first");
    EXPECT_TRUE(fm_->appendFile(p, " second"));
    std::ifstream f(p); std::stringstream ss; ss << f.rdbuf();
    EXPECT_EQ(ss.str(), "first second");
}

TEST_F(FileManagerTest, AppendFile_Invalid) {
    CoutCapture out;
    EXPECT_FALSE(fm_->appendFile(workspace_ / "ghost.txt", "x"));
    fs::path d = mkdir(workspace_, "d");
    EXPECT_FALSE(fm_->appendFile(d, "x"));
}

// ---------------------------------------------------------------------------
// DirectoryManager
// ---------------------------------------------------------------------------

class DirectoryManagerTest : public ChdirFixture {
protected:
    std::unique_ptr<DirectoryManager> dm_;
    void SetUp() override {
        ChdirFixture::SetUp();
        dm_ = std::make_unique<DirectoryManager>();
        CoutCapture out;
        ASSERT_TRUE(dm_->initialize());
    }
};

TEST_F(DirectoryManagerTest, Initialize_CreatesWorkspace) {
    EXPECT_TRUE(fs::is_directory(workspace_));
    EXPECT_EQ(dm_->getCurrentPath(), workspace_);
}

TEST_F(DirectoryManagerTest, Refresh_SortsAlphabetically) {
    touchFile(workspace_, "zeta.txt");
    touchFile(workspace_, "alpha.txt");
    touchFile(workspace_, "mid.txt");
    CoutCapture out;
    ASSERT_TRUE(dm_->refresh());
    const auto& entries = dm_->getEntries();
    ASSERT_EQ(entries.size(), 3u);
    EXPECT_EQ(entries[0].path().filename(), "alpha.txt");
    EXPECT_EQ(entries[1].path().filename(), "mid.txt");
    EXPECT_EQ(entries[2].path().filename(), "zeta.txt");
}

TEST_F(DirectoryManagerTest, OpenDirectory_Valid) {
    mkdir(workspace_, "sub");
    touchFile(workspace_, "a.txt");
    dm_->refresh();
    int subIndex = -1;
    for (size_t i = 0; i < dm_->getEntries().size(); ++i)
        if (dm_->getEntries()[i].path().filename() == "sub") subIndex = i + 1;
    ASSERT_NE(subIndex, -1);
    CoutCapture out;
    EXPECT_TRUE(dm_->openDirectory(subIndex));
    EXPECT_EQ(dm_->getCurrentPath().filename(), "sub");
}

TEST_F(DirectoryManagerTest, OpenDirectory_InvalidIndex) {
    CoutCapture out;
    EXPECT_FALSE(dm_->openDirectory(0));
    EXPECT_FALSE(dm_->openDirectory(999));
}

TEST_F(DirectoryManagerTest, OpenDirectory_NotAFolder) {
    touchFile(workspace_, "a.txt");
    dm_->refresh();
    int fileIndex = -1;
    for (size_t i = 0; i < dm_->getEntries().size(); ++i)
        if (dm_->getEntries()[i].path().filename() == "a.txt") fileIndex = i + 1;
    ASSERT_NE(fileIndex, -1);
    CoutCapture out;
    EXPECT_FALSE(dm_->openDirectory(fileIndex));
    EXPECT_NE(out.str().find("not a folder"), std::string::npos);
}

TEST_F(DirectoryManagerTest, OpenDirectory_SymlinkOutsideRejected) {
    fs::path outside = tempRoot_ / "outside";
    fs::create_directories(outside);
    std::error_code ec;
    fs::create_directory_symlink(outside, workspace_ / "escape", ec);
    if (ec) GTEST_SKIP() << "symlinks unsupported: " << ec.message();

    dm_->refresh();
    int idx = -1;
    for (size_t i = 0; i < dm_->getEntries().size(); ++i)
        if (dm_->getEntries()[i].path().filename() == "escape") idx = i + 1;
    ASSERT_NE(idx, -1);
    CoutCapture out;
    EXPECT_FALSE(dm_->openDirectory(idx));
    EXPECT_NE(out.str().find("Access outside workspace"), std::string::npos);
}

TEST_F(DirectoryManagerTest, GoBack_AtRootRefused) {
    CoutCapture out;
    EXPECT_FALSE(dm_->goBack());
    EXPECT_EQ(dm_->getCurrentPath(), workspace_);
}

TEST_F(DirectoryManagerTest, GoBack_FromSubfolder) {
    mkdir(workspace_, "sub");
    dm_->refresh();
    dm_->openDirectory(1);
    CoutCapture out;
    EXPECT_TRUE(dm_->goBack());
    EXPECT_EQ(dm_->getCurrentPath(), workspace_);
}

TEST_F(DirectoryManagerTest, GoHome) {
    mkdir(workspace_, "sub");
    dm_->refresh();
    dm_->openDirectory(1);
    CoutCapture out;
    dm_->goHome();
    EXPECT_EQ(dm_->getCurrentPath(), workspace_);
}

// ---------------------------------------------------------------------------
// ClipboardManager
// ---------------------------------------------------------------------------

class ClipboardTest : public TempWorkspaceFixture {
protected:
    FileManager fm_{workspace_};
    ClipboardManager clip_;
};

TEST_F(ClipboardTest, InitiallyEmpty) {
    EXPECT_TRUE(clip_.isEmpty());
}

TEST_F(ClipboardTest, Copy_SetsState) {
    CoutCapture out;
    fs::path p = touchFile(workspace_, "a.txt");
    clip_.copy(p);
    EXPECT_FALSE(clip_.isEmpty());
    EXPECT_NE(out.str().find("copied"), std::string::npos);
}

TEST_F(ClipboardTest, Cut_SetsState) {
    CoutCapture out;
    fs::path p = touchFile(workspace_, "a.txt");
    clip_.cut(p);
    EXPECT_FALSE(clip_.isEmpty());
    EXPECT_NE(out.str().find("moving"), std::string::npos);
}

TEST_F(ClipboardTest, Paste_WhenEmpty) {
    CoutCapture out;
    EXPECT_FALSE(clip_.paste(workspace_, fm_));
    EXPECT_NE(out.str().find("empty"), std::string::npos);
}

TEST_F(ClipboardTest, Paste_Copy) {
    CoutCapture out;
    fs::path src = touchFile(workspace_, "a.txt");
    fs::path dst = mkdir(workspace_, "dest");
    clip_.copy(src);
    EXPECT_TRUE(clip_.paste(dst, fm_));
    EXPECT_TRUE(fs::exists(dst / "a.txt"));
    EXPECT_TRUE(fs::exists(src)); // copy preserves source
}

TEST_F(ClipboardTest, Paste_Cut) {
    CoutCapture out;
    fs::path src = touchFile(workspace_, "a.txt");
    fs::path dst = mkdir(workspace_, "dest");
    clip_.cut(src);
    EXPECT_TRUE(clip_.paste(dst, fm_));
    EXPECT_TRUE(fs::exists(dst / "a.txt"));
    EXPECT_FALSE(fs::exists(src)); // cut removes source
}

TEST_F(ClipboardTest, Clear) {
    fs::path p = touchFile(workspace_, "a.txt");
    clip_.copy(p);
    clip_.clear();
    EXPECT_TRUE(clip_.isEmpty());
}

// ---------------------------------------------------------------------------
// SearchEngine
// ---------------------------------------------------------------------------

class SearchTest : public TempWorkspaceFixture {
protected:
    std::unique_ptr<SearchEngine> se_;
    void SetUp() override {
        TempWorkspaceFixture::SetUp();
        se_ = std::make_unique<SearchEngine>(workspace_);
    }
};

TEST_F(SearchTest, EmptyKeyword) {
    CoutCapture out;
    EXPECT_TRUE(se_->search("").empty());
    EXPECT_NE(out.str().find("cannot be empty"), std::string::npos);
}

TEST_F(SearchTest, NoMatch) {
    touchFile(workspace_, "a.txt");
    CoutCapture out;
    EXPECT_TRUE(se_->search("zzz").empty());
}

TEST_F(SearchTest, CaseInsensitiveMatch) {
    touchFile(workspace_, "Notes.txt");
    CoutCapture out;
    auto r = se_->search("notes");
    ASSERT_EQ(r.size(), 1u);
    EXPECT_EQ(r[0].filename(), "Notes.txt");
}

TEST_F(SearchTest, RecursiveMatch) {
    fs::path sub = mkdir(workspace_, "sub");
    fs::path deep = mkdir(sub, "deep");
    touchFile(deep, "needle.txt");
    CoutCapture out;
    EXPECT_EQ(se_->search("needle").size(), 1u);
}

TEST_F(SearchTest, SubstringMatch) {
    touchFile(workspace_, "abcdef.txt");
    CoutCapture out;
    EXPECT_EQ(se_->search("cde").size(), 1u);
}

// ---------------------------------------------------------------------------
// StorageAnalyzer
// ---------------------------------------------------------------------------

class StorageTest : public TempWorkspaceFixture {
protected:
    std::unique_ptr<StorageAnalyzer> sa_;
    void SetUp() override {
        TempWorkspaceFixture::SetUp();
        sa_ = std::make_unique<StorageAnalyzer>(workspace_);
    }
};

TEST_F(StorageTest, EmptyWorkspace) {
    CoutCapture out;
    auto info = sa_->analyze();
    EXPECT_EQ(info.totalFiles, 0u);
    EXPECT_EQ(info.totalFolders, 0u);
    EXPECT_EQ(info.totalBytes, 0u);
}

TEST_F(StorageTest, WithFilesAndFolders) {
    touchFile(workspace_, "a.txt", "abc");      // 3 bytes
    touchFile(workspace_, "b.txt", "abcdef");   // 6 bytes
    fs::path sub = mkdir(workspace_, "sub");
    touchFile(sub, "c.txt", "12");              // 2 bytes

    CoutCapture out;
    auto info = sa_->analyze();
    EXPECT_EQ(info.totalFiles, 3u);
    EXPECT_EQ(info.totalFolders, 1u);
    EXPECT_EQ(info.totalBytes, 11u);
}

TEST_F(StorageTest, DisplayInfo) {
    touchFile(workspace_, "a.txt", "abc");
    CoutCapture out;
    sa_->displayStorageInfo();
    EXPECT_NE(out.str().find("STORAGE ANALYSIS"), std::string::npos);
}

TEST_F(StorageTest, WorkspaceIsFile) {
    fs::path f = touchFile(tempRoot_, "notadir");
    StorageAnalyzer sa(f);
    CoutCapture out;
    auto info = sa.analyze();
    EXPECT_EQ(info.totalBytes, 0u);
    EXPECT_NE(out.str().find("not a directory"), std::string::npos);
}

TEST_F(StorageTest, NonexistentWorkspace) {
    StorageAnalyzer sa(tempRoot_ / "does-not-exist");
    CoutCapture out;
    auto info = sa.analyze();
    EXPECT_EQ(info.totalBytes, 0u);
    EXPECT_NE(out.str().find("Storage analysis error"), std::string::npos);
}

// ---------------------------------------------------------------------------
// FileViewer
// ---------------------------------------------------------------------------

TEST_F(TempWorkspaceFixture, FileViewer_Valid) {
    fs::path p = touchFile(workspace_, "v.txt", "line1\nline2\n");
    CoutCapture out;
    FileViewer fv;
    fv.openFile(p);
    EXPECT_NE(out.str().find("line1"), std::string::npos);
    EXPECT_NE(out.str().find("line2"), std::string::npos);
}

TEST_F(TempWorkspaceFixture, FileViewer_Nonexistent) {
    CoutCapture out;
    FileViewer fv;
    fv.openFile(workspace_ / "ghost.txt");
    EXPECT_NE(out.str().find("does not exist"), std::string::npos);
}

TEST_F(TempWorkspaceFixture, FileViewer_NotAFile) {
    fs::path d = mkdir(workspace_, "sub");
    CoutCapture out;
    FileViewer fv;
    fv.openFile(d);
    EXPECT_NE(out.str().find("not a regular file"), std::string::npos);
}

// ---------------------------------------------------------------------------
// ContextMenu (parameterised: covers every switch case)
// ---------------------------------------------------------------------------

class ContextMenuParamTest
    : public ::testing::TestWithParam<std::tuple<bool, std::string, ContextMenu::Action>> {};

TEST_P(ContextMenuParamTest, ReturnsExpectedAction) {
    bool isDir = std::get<0>(GetParam());
    std::string choice = std::get<1>(GetParam());
    ContextMenu::Action expected = std::get<2>(GetParam());

    CinRedirect in(choice + "\n");
    CoutCapture out;
    ContextMenu menu;
    EXPECT_EQ(menu.show(isDir), expected);
}

INSTANTIATE_TEST_SUITE_P(FileMenu, ContextMenuParamTest, ::testing::Values(
    std::make_tuple(false, "1", ContextMenu::Action::Open),
    std::make_tuple(false, "2", ContextMenu::Action::Copy),
    std::make_tuple(false, "3", ContextMenu::Action::Cut),
    std::make_tuple(false, "4", ContextMenu::Action::Write),
    std::make_tuple(false, "5", ContextMenu::Action::Rename),
    std::make_tuple(false, "6", ContextMenu::Action::Delete),
    std::make_tuple(false, "7", ContextMenu::Action::Properties),
    std::make_tuple(false, "8", ContextMenu::Action::Cancel),
    std::make_tuple(false, "99", ContextMenu::Action::Cancel) // invalid default
));

INSTANTIATE_TEST_SUITE_P(FolderMenu, ContextMenuParamTest, ::testing::Values(
    std::make_tuple(true, "1", ContextMenu::Action::Open),
    std::make_tuple(true, "2", ContextMenu::Action::Copy),
    std::make_tuple(true, "3", ContextMenu::Action::Cut),
    std::make_tuple(true, "4", ContextMenu::Action::Rename),
    std::make_tuple(true, "5", ContextMenu::Action::Delete),
    std::make_tuple(true, "6", ContextMenu::Action::Properties),
    std::make_tuple(true, "7", ContextMenu::Action::Cancel),
    std::make_tuple(true, "99", ContextMenu::Action::Cancel)
));

TEST(ContextMenuTest, NonNumericInputCancel) {
    CinRedirect in("garbage\n");
    CoutCapture out;
    ContextMenu menu;
    EXPECT_EQ(menu.show(false), ContextMenu::Action::Cancel);
    EXPECT_NE(out.str().find("Invalid input"), std::string::npos);
}

// ---------------------------------------------------------------------------
// FileExplorer (uses the private-public seam)
// ---------------------------------------------------------------------------

class FileExplorerTest : public ChdirFixture {
protected:
    std::unique_ptr<FileExplorer> fe_;

    void SetUp() override {
        ChdirFixture::SetUp();
        fe_ = std::make_unique<FileExplorer>();
        CoutCapture out;
        fe_->initialize();
    }

    // Helper: locate the 1-based index of a name in the current listing.
    int indexOf(const std::string& name) {
        fe_->directoryManager.refresh();
        const auto& e = fe_->directoryManager.getEntries();
        for (size_t i = 0; i < e.size(); ++i)
            if (e[i].path().filename() == name) return int(i + 1);
        return -1;
    }
};

TEST_F(FileExplorerTest, Initialize_CreatesSubsystems) {
    EXPECT_NE(fe_->fileManager, nullptr);
    EXPECT_NE(fe_->searchEngine, nullptr);
    EXPECT_NE(fe_->storageAnalyzer, nullptr);
    EXPECT_TRUE(fe_->running);
}

TEST_F(FileExplorerTest, HandleCommand_Help) {
    CoutCapture out;
    fe_->handleCommand("help");
    EXPECT_NE(out.str().find("AVAILABLE COMMANDS"), std::string::npos);
}

TEST_F(FileExplorerTest, HandleCommand_Exit) {
    CoutCapture out;
    fe_->handleCommand("exit");
    EXPECT_FALSE(fe_->running);
}

TEST_F(FileExplorerTest, HandleCommand_Unknown) {
    CoutCapture out;
    fe_->handleCommand("frobnicate");
    EXPECT_NE(out.str().find("Unknown command"), std::string::npos);
}

TEST_F(FileExplorerTest, HandleCommand_Empty) {
    CoutCapture out;
    fe_->handleCommand("");
    EXPECT_TRUE(out.str().empty());
}

TEST_F(FileExplorerTest, HandleCommand_BackAndHome) {
    mkdir(workspace_, "sub");
    fe_->handleCommand("back"); // at root -> refused, but covered
    fe_->handleCommand("home");
    EXPECT_EQ(fe_->directoryManager.getCurrentPath(), workspace_);
}

TEST_F(FileExplorerTest, HandleCommand_Refresh) {
    CoutCapture out;
    fe_->handleCommand("refresh");
}

TEST_F(FileExplorerTest, HandleCommand_CreateFile) {
    CinRedirect in("new.txt\n");
    CoutCapture out;
    fe_->handleCommand("create-file");
    EXPECT_TRUE(fs::exists(workspace_ / "new.txt"));
}

TEST_F(FileExplorerTest, HandleCommand_CreateFolder) {
    CinRedirect in("newdir\n");
    CoutCapture out;
    fe_->handleCommand("create-folder");
    EXPECT_TRUE(fs::is_directory(workspace_ / "newdir"));
}

TEST_F(FileExplorerTest, HandleCommand_NumberSelectsAndOpensMenu) {
    touchFile(workspace_, "a.txt");
    CinRedirect in("8\n"); // menu choice 8 = Cancel for file
    CoutCapture out;
    fe_->handleCommand("1");
    EXPECT_NE(out.str().find("cancelled"), std::string::npos);
}

TEST_F(FileExplorerTest, HandleCommand_CopyViaCommand) {
    touchFile(workspace_, "a.txt");
    CoutCapture out;
    fe_->handleCommand("copy 1");
    EXPECT_FALSE(fe_->clipboardManager.isEmpty());
}

TEST_F(FileExplorerTest, HandleCommand_CutViaCommand) {
    touchFile(workspace_, "a.txt");
    CoutCapture out;
    fe_->handleCommand("cut 1");
    EXPECT_FALSE(fe_->clipboardManager.isEmpty());
}

TEST_F(FileExplorerTest, HandleCommand_PasteEmpty) {
    CoutCapture out;
    fe_->handleCommand("paste");
    EXPECT_NE(out.str().find("Clipboard is empty"), std::string::npos);
}

TEST_F(FileExplorerTest, HandleCommand_PasteCopy) {
    touchFile(workspace_, "a.txt");
    fe_->handleCommand("copy 1");
    CoutCapture out;
    fe_->handleCommand("paste");
    EXPECT_TRUE(fs::exists(workspace_ / "a.txt"));
}

TEST_F(FileExplorerTest, HandleCommand_CopyMissingIndex) {
    CoutCapture out;
    fe_->handleCommand("copy"); // no index provided
    EXPECT_NE(out.str().find("provide an item number"), std::string::npos);
}

TEST_F(FileExplorerTest, HandleCommand_ItemInvalidIndex) {
    CoutCapture out;
    fe_->handleCommand("delete 999");
    EXPECT_NE(out.str().find("Invalid selection"), std::string::npos);
}

TEST_F(FileExplorerTest, HandleCommand_Write) {
    touchFile(workspace_, "w.txt", "old");
    CinRedirect in("line1\nline2\nEND\n");
    CoutCapture out;
    fe_->handleCommand("write 1");
    std::ifstream f(workspace_ / "w.txt"); std::stringstream ss; ss << f.rdbuf();
    EXPECT_EQ(ss.str(), "line1\nline2\n");
}

TEST_F(FileExplorerTest, HandleCommand_Append) {
    touchFile(workspace_, "w.txt", "first\n");
    CinRedirect in("second\nEND\n");
    CoutCapture out;
    fe_->handleCommand("append 1");
    std::ifstream f(workspace_ / "w.txt"); std::stringstream ss; ss << f.rdbuf();
    EXPECT_EQ(ss.str(), "first\nsecond\n");
}

TEST_F(FileExplorerTest, HandleCommand_WriteOnFolderRefused) {
    mkdir(workspace_, "sub");
    CoutCapture out;
    fe_->handleCommand("write 1");
    EXPECT_NE(out.str().find("supported only for files"), std::string::npos);
}

TEST_F(FileExplorerTest, HandleCommand_Rename) {
    touchFile(workspace_, "a.txt");
    CinRedirect in("b.txt\n");
    CoutCapture out;
    fe_->handleCommand("rename 1");
    EXPECT_TRUE(fs::exists(workspace_ / "b.txt"));
}

TEST_F(FileExplorerTest, HandleCommand_DeleteConfirmed) {
    touchFile(workspace_, "a.txt");
    CinRedirect in("y\n");
    CoutCapture out;
    fe_->handleCommand("delete 1");
    EXPECT_FALSE(fs::exists(workspace_ / "a.txt"));
}

TEST_F(FileExplorerTest, HandleCommand_DeleteCancelled) {
    touchFile(workspace_, "a.txt");
    CinRedirect in("n\n");
    CoutCapture out;
    fe_->handleCommand("delete 1");
    EXPECT_TRUE(fs::exists(workspace_ / "a.txt"));
}

TEST_F(FileExplorerTest, HandleCommand_Properties) {
    touchFile(workspace_, "a.txt");
    CoutCapture out;
    fe_->handleCommand("properties 1");
    EXPECT_NE(out.str().find("PROPERTIES"), std::string::npos);
}

TEST_F(FileExplorerTest, HandleCommand_OpenFile) {
    touchFile(workspace_, "a.txt", "hello");
    CoutCapture out;
    fe_->handleCommand("open 1");
    EXPECT_NE(out.str().find("hello"), std::string::npos);
}

TEST_F(FileExplorerTest, HandleCommand_SearchWithKeyword) {
    touchFile(workspace_, "needle.txt");
    CoutCapture out;
    fe_->handleCommand("search needle");
    EXPECT_NE(out.str().find("SEARCH RESULTS"), std::string::npos);
    EXPECT_NE(out.str().find("needle.txt"), std::string::npos);
}

TEST_F(FileExplorerTest, HandleCommand_SearchMissingKeyword) {
    CoutCapture out;
    fe_->handleCommand("search");
    EXPECT_NE(out.str().find("Enter a search keyword"), std::string::npos);
}

TEST_F(FileExplorerTest, HandleCommand_Storage) {
    touchFile(workspace_, "a.txt", "abc");
    CoutCapture out;
    fe_->handleCommand("storage");
    EXPECT_NE(out.str().find("STORAGE ANALYSIS"), std::string::npos);
}

TEST_F(FileExplorerTest, HandleSelection_OpenViaMenu) {
    touchFile(workspace_, "a.txt", "data");
    CinRedirect in("1\n");
    CoutCapture out;
    fe_->handleSelection(1);
    EXPECT_NE(out.str().find("data"), std::string::npos);
}

TEST_F(FileExplorerTest, HandleSelection_PropertiesViaMenu) {
    touchFile(workspace_, "a.txt");
    CinRedirect in("7\n");
    CoutCapture out;
    fe_->handleSelection(1);
    EXPECT_NE(out.str().find("PROPERTIES"), std::string::npos);
}

TEST_F(FileExplorerTest, HandleSelection_InvalidIndex) {
    CoutCapture out;
    fe_->handleSelection(999);
    EXPECT_NE(out.str().find("Invalid selection"), std::string::npos);
}

TEST_F(FileExplorerTest, OpenItem_File) {
    touchFile(workspace_, "a.txt", "contents");
    CoutCapture out;
    fe_->openItem(1);
    EXPECT_NE(out.str().find("contents"), std::string::npos);
}

TEST_F(FileExplorerTest, OpenItem_Folder) {
    mkdir(workspace_, "sub");
    fe_->directoryManager.refresh();
    CoutCapture out;
    fe_->openItem(1);
    EXPECT_EQ(fe_->directoryManager.getCurrentPath().filename(), "sub");
}

TEST_F(FileExplorerTest, OpenItem_InvalidIndex) {
    CoutCapture out;
    fe_->openItem(999);
    EXPECT_NE(out.str().find("Invalid selection"), std::string::npos);
}

TEST_F(FileExplorerTest, DisplayHeaderAndDirectory) {
    touchFile(workspace_, "a.txt", "xyz");
    mkdir(workspace_, "sub");
    fe_->directoryManager.refresh();
    CoutCapture out;
    fe_->display();
    EXPECT_NE(out.str().find("SMART FILE EXPLORER"), std::string::npos);
    EXPECT_NE(out.str().find("Total:"), std::string::npos);
}

TEST_F(FileExplorerTest, DisplayEmptyDirectory) {
    CoutCapture out;
    fe_->display();
    EXPECT_NE(out.str().find("empty"), std::string::npos);
}

// Integration: exercise run() end-to-end. Feeding "help\nexit\n" hits run(),
// display(), handleCommand(), displayHelp(), exitApplication().
TEST_F(FileExplorerTest, Run_Integration) {
    CinRedirect in("help\nexit\n");
    CoutCapture out;
    fe_->run();
    EXPECT_FALSE(fe_->running);
    EXPECT_NE(out.str().find("AVAILABLE COMMANDS"), std::string::npos);
}

TEST_F(FileExplorerTest, ExitApplicationSetsFlag) {
    CoutCapture out;
    fe_->exitApplication();
    EXPECT_FALSE(fe_->running);
    EXPECT_NE(out.str().find("Exiting"), std::string::npos);
}