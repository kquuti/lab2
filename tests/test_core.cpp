#include <gtest/gtest.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/Citation.h"
#include "core/Collection.h"
#include "core/Reference.h"
#include "core/Storage.h"

namespace {

Reference makeBook() {
    Reference book(ReferenceType::Book, "The C Programming Language");
    book.setAuthors({"Kernighan Brian", "Ritchie Dennis"});
    book.setPublisher("Prentice Hall");
    book.setYear(1988);
    return book;
}

}  // namespace

// ---------- Reference ----------

TEST(Reference, MatchesIgnoringAsciiCase) {
    Reference r(ReferenceType::Book, "Clean Code");
    r.setAuthors({"Martin Robert"});
    r.setYear(2008);
    r.setComment("great book");
    EXPECT_TRUE(r.matches("clean", SearchField::All));
    EXPECT_TRUE(r.matches("MARTIN", SearchField::Author));
    EXPECT_TRUE(r.matches("2008", SearchField::Year));
    EXPECT_TRUE(r.matches("GREAT", SearchField::Comment));
    EXPECT_TRUE(r.matches("", SearchField::Title));
}

TEST(Reference, FieldLimitsTheSearch) {
    Reference r(ReferenceType::Book, "Clean Code");
    r.setAuthors({"Martin Robert"});
    EXPECT_FALSE(r.matches("martin", SearchField::Title));
    EXPECT_FALSE(r.matches("clean", SearchField::Author));
    EXPECT_FALSE(r.matches("2008", SearchField::Year));
}

TEST(Reference, MatchesCyrillicWithSameCase) {
    Reference r(ReferenceType::Book, "Основи програмування");
    EXPECT_TRUE(r.matches("Осн", SearchField::Title));
    EXPECT_TRUE(r.matches("програм", SearchField::Title));
    EXPECT_FALSE(r.matches("алгоритм", SearchField::Title));
}

TEST(Reference, TypeConversion) {
    EXPECT_EQ(typeFromInt(1), ReferenceType::Web);
    EXPECT_THROW(typeFromInt(-1), std::invalid_argument);
    EXPECT_THROW(typeFromInt(kReferenceTypeCount), std::invalid_argument);
    EXPECT_EQ(typeName(ReferenceType::Video), "Video");
}

// ---------- Collection: посилання ----------

TEST(Collection, AddsAndFindsReferences) {
    Collection collection;
    const int first = collection.addReference(makeBook());
    const int second = collection.addReference(makeBook());
    EXPECT_EQ(first, 1);
    EXPECT_EQ(second, 2);
    ASSERT_NE(collection.findReference(first), nullptr);
    EXPECT_EQ(collection.findReference(first)->title(), "The C Programming Language");
    EXPECT_EQ(collection.findReference(99), nullptr);
}

TEST(Collection, RejectsEmptyTitleAndUnknownContext) {
    Collection collection;
    EXPECT_THROW(collection.addReference(Reference(ReferenceType::Book, "")), std::invalid_argument);
    Reference r = makeBook();
    r.addContext(42);
    EXPECT_THROW(collection.addReference(r), std::invalid_argument);
    EXPECT_TRUE(collection.references().empty());
}

TEST(Collection, UpdatesReference) {
    Collection collection;
    const int id = collection.addReference(makeBook());
    Reference changed(ReferenceType::Web, "New title");
    collection.updateReference(id, changed);
    EXPECT_EQ(collection.findReference(id)->title(), "New title");
    EXPECT_EQ(collection.findReference(id)->id(), id);
    EXPECT_THROW(collection.updateReference(99, changed), std::invalid_argument);
}

TEST(Collection, RemovesReferenceAndDoesNotReuseIds) {
    Collection collection;
    const int first = collection.addReference(makeBook());
    collection.removeReference(first);
    EXPECT_EQ(collection.findReference(first), nullptr);
    EXPECT_THROW(collection.removeReference(first), std::invalid_argument);
    EXPECT_EQ(collection.addReference(makeBook()), first + 1);
}

// ---------- Collection: контексти ----------

TEST(Collection, BuildsContextHierarchy) {
    Collection collection;
    const int study = collection.addContext("Study");
    const int cpp = collection.addContext("C++", study);
    const int qt = collection.addContext("Qt", cpp);
    EXPECT_EQ(collection.contextPath(qt), "Study / C++ / Qt");
    EXPECT_EQ(collection.childrenOf(0).size(), 1u);
    EXPECT_EQ(collection.childrenOf(study).size(), 1u);
    EXPECT_EQ(collection.contextWithDescendants(study).size(), 3u);
    EXPECT_TRUE(collection.contextWithDescendants(99).empty());
}

TEST(Collection, ContextValidation) {
    Collection collection;
    EXPECT_THROW(collection.addContext(""), std::invalid_argument);
    EXPECT_THROW(collection.addContext("A", 5), std::invalid_argument);
    const int id = collection.addContext("A");
    collection.renameContext(id, "B");
    EXPECT_EQ(collection.findContext(id)->name, "B");
    EXPECT_THROW(collection.renameContext(id, ""), std::invalid_argument);
    EXPECT_THROW(collection.renameContext(99, "X"), std::invalid_argument);
}

TEST(Collection, RemovingContextRemovesSubcontextsAndDetachesReferences) {
    Collection collection;
    const int parent = collection.addContext("Parent");
    const int child = collection.addContext("Child", parent);
    const int other = collection.addContext("Other");

    Reference r = makeBook();
    r.addContext(child);
    r.addContext(other);
    const int id = collection.addReference(r);

    collection.removeContext(parent);
    EXPECT_EQ(collection.findContext(parent), nullptr);
    EXPECT_EQ(collection.findContext(child), nullptr);
    ASSERT_NE(collection.findContext(other), nullptr);
    EXPECT_FALSE(collection.findReference(id)->inContext(child));
    EXPECT_TRUE(collection.findReference(id)->inContext(other));
    EXPECT_THROW(collection.removeContext(parent), std::invalid_argument);
}

// ---------- Collection: пошук ----------

class SearchFixture : public ::testing::Test {
protected:
    void SetUp() override {
        programming = collection.addContext("Programming");
        cpp = collection.addContext("C++", programming);
        music = collection.addContext("Music");

        Reference book(ReferenceType::Book, "Clean Code");
        book.setAuthors({"Martin Robert"});
        book.addContext(cpp);
        bookId = collection.addReference(book);

        Reference web(ReferenceType::Web, "Qt Documentation");
        web.addContext(programming);
        webId = collection.addReference(web);

        Reference video(ReferenceType::Video, "Jazz history");
        video.addContext(music);
        video.addContext(programming);  // одне посилання в кількох контекстах
        videoId = collection.addReference(video);
    }

    Collection collection;
    int programming = 0;
    int cpp = 0;
    int music = 0;
    int bookId = 0;
    int webId = 0;
    int videoId = 0;
};

TEST_F(SearchFixture, EmptyQueryFindsEverything) {
    EXPECT_EQ(collection.search(SearchQuery()).size(), 3u);
}

TEST_F(SearchFixture, SearchByTextAndField) {
    SearchQuery query;
    query.text = "martin";
    query.field = SearchField::Author;
    EXPECT_EQ(collection.search(query), (std::vector<int>{bookId}));
    query.field = SearchField::Title;
    EXPECT_TRUE(collection.search(query).empty());
}

TEST_F(SearchFixture, SearchByType) {
    SearchQuery query;
    query.type = ReferenceType::Web;
    EXPECT_EQ(collection.search(query), (std::vector<int>{webId}));
}

TEST_F(SearchFixture, SearchByContextWithAndWithoutSubcontexts) {
    SearchQuery query;
    query.contextId = programming;
    query.includeSubcontexts = true;
    EXPECT_EQ(collection.search(query), (std::vector<int>{bookId, webId, videoId}));

    query.includeSubcontexts = false;
    EXPECT_EQ(collection.search(query), (std::vector<int>{webId, videoId}));

    query.contextId = music;
    EXPECT_EQ(collection.search(query), (std::vector<int>{videoId}));
}

TEST_F(SearchFixture, CriteriaAreCombined) {
    SearchQuery query;
    query.contextId = programming;
    query.type = ReferenceType::Book;
    query.text = "clean";
    EXPECT_EQ(collection.search(query), (std::vector<int>{bookId}));
    query.text = "jazz";
    EXPECT_TRUE(collection.search(query).empty());
}

TEST_F(SearchFixture, UnknownContextFindsNothing) {
    SearchQuery query;
    query.contextId = 999;
    EXPECT_TRUE(collection.search(query).empty());
}

// ---------- Цитування ----------

TEST(Citation, AuthorNameParts) {
    EXPECT_EQ(authorSurname("Kernighan Brian"), "Kernighan");
    EXPECT_EQ(authorSurname("Kernighan, Brian"), "Kernighan");
    EXPECT_EQ(authorInitials("Kernighan Brian", " "), "B.");
    EXPECT_EQ(authorInitials("Іваненко Іван Петрович", " "), "І. П.");
    EXPECT_EQ(authorInitials("Brian Wilson Kernighan", ""), "W.K.");
    EXPECT_EQ(authorSurname(""), "");
    EXPECT_EQ(authorInitials("Ritchie", " "), "");
}

TEST(Citation, GostBook) {
    Reference r(ReferenceType::Book, "Основи програмування");
    r.setAuthors({"Іваненко Іван Петрович"});
    r.setPublisher("Наука");
    r.setYear(2020);
    EXPECT_EQ(GostFormatter().format(r), "Іваненко І. П. Основи програмування. – Наука, 2020.");
}

TEST(Citation, GostWebPage) {
    Reference r(ReferenceType::Web, "Qt Documentation");
    r.setUrl("https://doc.qt.io");
    EXPECT_EQ(GostFormatter().format(r),
              "Qt Documentation [Електронний ресурс]. – Режим доступу: https://doc.qt.io");
}

TEST(Citation, Dstu8302Book) {
    EXPECT_EQ(Dstu8302Formatter().format(makeBook()),
              "Kernighan B., Ritchie D. The C Programming Language. Prentice Hall, 1988.");
}

TEST(Citation, Dstu8302WebPage) {
    Reference r(ReferenceType::Web, "Qt Documentation");
    r.setUrl("https://doc.qt.io");
    EXPECT_EQ(Dstu8302Formatter().format(r), "Qt Documentation. URL: https://doc.qt.io");
}

TEST(Citation, HarvardTwoAuthors) {
    EXPECT_EQ(HarvardFormatter().format(makeBook()),
              "Kernighan, B. and Ritchie, D. (1988) The C Programming Language. Prentice Hall.");
}

TEST(Citation, HarvardNoYear) {
    Reference r(ReferenceType::Book, "X");
    r.setAuthors({"Kernighan Brian"});
    EXPECT_EQ(HarvardFormatter().format(r), "Kernighan, B. (n.d.) X.");
}

TEST(Citation, HarvardNoAuthors) {
    EXPECT_EQ(HarvardFormatter().format(Reference(ReferenceType::Web, "Title")), "Title (n.d.).");
}

TEST(Citation, BibTexBook) {
    Reference r = makeBook();
    r.setAuthors({"Kernighan Brian"});
    EXPECT_EQ(BibTexFormatter().format(r),
              "@book{Kernighan1988,\n"
              "  author = {Kernighan, B.},\n"
              "  title = {The C Programming Language},\n"
              "  year = {1988},\n"
              "  publisher = {Prentice Hall}\n"
              "}");
}

TEST(Citation, BibTexKeyWithoutAuthor) {
    Reference r(ReferenceType::Web, "T");
    r.setId(7);
    EXPECT_EQ(BibTexFormatter().format(r), "@misc{ref7,\n  title = {T}\n}");
}

TEST(Citation, FormatListNumbersEntries) {
    const Reference a(ReferenceType::Book, "A");
    const Reference b(ReferenceType::Book, "B");
    EXPECT_EQ(formatList(HarvardFormatter(), {&a, &b}), "1. A (n.d.).\n2. B (n.d.).\n");
}

TEST(Citation, FormatListForBibTexIsNotNumbered) {
    Reference a(ReferenceType::Web, "A");
    a.setId(1);
    EXPECT_EQ(formatList(BibTexFormatter(), {&a}), "@misc{ref1,\n  title = {A}\n}\n\n");
    EXPECT_TRUE(formatList(HarvardFormatter(), {}).empty());
}

TEST(Citation, AllFormattersAreAvailableThroughInterface) {
    const auto formatters = createAllFormatters();
    ASSERT_EQ(formatters.size(), 4u);
    const Reference book = makeBook();
    for (const auto& formatter : formatters) {
        EXPECT_FALSE(formatter->name().empty());
        EXPECT_FALSE(formatter->format(book).empty());
    }
    EXPECT_EQ(formatters[3]->fileExtension(), "bib");
}

// ---------- Збереження ----------

TEST(Storage, RoundTripKeepsEverything) {
    Collection original;
    const int parent = original.addContext("Work");
    const int child = original.addContext("Sub\tcontext", parent);

    Reference r(ReferenceType::Video, "Title\twith tab\nand newline \\ backslash");
    r.setAuthors({"A B", "C D"});
    r.setYear(1999);
    r.setPublisher("Publisher");
    r.setUrl("http://example.com");
    r.setComment("multi\nline");
    r.addContext(child);
    const int id = original.addReference(r);
    original.addReference(Reference(ReferenceType::Audio, "Plain"));

    std::ostringstream out;
    saveCollection(original, out);
    std::istringstream in(out.str());
    const Collection loaded = loadCollection(in);

    ASSERT_EQ(loaded.references().size(), 2u);
    const Reference* copy = loaded.findReference(id);
    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->type(), ReferenceType::Video);
    EXPECT_EQ(copy->title(), r.title());
    EXPECT_EQ(copy->authors(), r.authors());
    EXPECT_EQ(copy->year(), 1999);
    EXPECT_EQ(copy->publisher(), "Publisher");
    EXPECT_EQ(copy->url(), "http://example.com");
    EXPECT_EQ(copy->comment(), "multi\nline");
    EXPECT_TRUE(copy->inContext(child));
    EXPECT_EQ(loaded.contextPath(child), "Work / Sub\tcontext");
}

TEST(Storage, NewIdsContinueAfterLoading) {
    Collection original;
    original.addReference(Reference(ReferenceType::Book, "A"));
    original.addReference(Reference(ReferenceType::Book, "B"));
    std::ostringstream out;
    saveCollection(original, out);
    std::istringstream in(out.str());
    Collection loaded = loadCollection(in);
    EXPECT_EQ(loaded.addReference(Reference(ReferenceType::Book, "C")), 3);
}

TEST(Storage, EmptyCollectionRoundTrip) {
    std::ostringstream out;
    saveCollection(Collection(), out);
    std::istringstream in(out.str());
    EXPECT_TRUE(loadCollection(in).references().empty());
}

TEST(Storage, RejectsUnknownFormat) {
    std::istringstream empty("");
    EXPECT_THROW(loadCollection(empty), std::runtime_error);
    std::istringstream garbage("garbage\n");
    EXPECT_THROW(loadCollection(garbage), std::runtime_error);
}

TEST(Storage, RejectsBrokenRecords) {
    std::istringstream unknownRecord("REFS\t1\nX\t1\n");
    EXPECT_THROW(loadCollection(unknownRecord), std::runtime_error);
    std::istringstream shortRecord("REFS\t1\nR\t0\t1\n");
    EXPECT_THROW(loadCollection(shortRecord), std::runtime_error);
    std::istringstream badNumber("REFS\t1\nC\tabc\tName\t0\n");
    EXPECT_THROW(loadCollection(badNumber), std::runtime_error);
}

TEST(Storage, DropsUnknownContextsOfReferences) {
    std::istringstream in("REFS\t1\nR\t0\t1\tTitle\t2000\t\t\t\t0\t1\t99\n");
    const Collection loaded = loadCollection(in);
    ASSERT_EQ(loaded.references().size(), 1u);
    EXPECT_TRUE(loaded.references()[0].contextIds().empty());
}

TEST(Storage, ErrorMessageContainsLineNumber) {
    std::istringstream in("REFS\t1\nC\t1\tOk\t0\nX\n");
    try {
        loadCollection(in);
        FAIL() << "expected an exception";
    } catch (const std::runtime_error& error) {
        EXPECT_NE(std::string(error.what()).find("Line 3"), std::string::npos);
    }
}