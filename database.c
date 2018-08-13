#include <err.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "database.h"
#include "util.h"

#define ASSERT_SQL(EXPR) assert_sql(db, EXPR, __FILE__, __LINE__)

#define PREPARE(STMT, QUERY) ASSERT_SQL(sqlite3_prepare_v2(db, QUERY, -1, &(STMT), NULL))
#define BIND(TYPE, STMT, NAME, VAL) \
    ASSERT_SQL(sqlite3_bind_##TYPE (STMT,                            \
                                    sqlite3_bind_parameter_index(STMT, NAME), VAL))
#define BIND_TEXT(STMT, NAME, VAL) \
    ASSERT_SQL(sqlite3_bind_text(STMT,                               \
                                 sqlite3_bind_parameter_index(STMT, NAME), VAL, -1, NULL))

static const char *db_setup_queries[] =
    {"CREATE TABLE image ("
     "  id INTEGER PRIMARY KEY,"
     "  title VARCHAR(100) NOT NULL,"
     "  ext VARCHAR(10) NOT NULL );",

     "CREATE TABLE tag ("
     "  id INTEGER PRIMARY KEY,"
     "  name VARCHAR(100) UNIQUE NOT NULL );",

     "CREATE TABLE image_tag ("
     "  image INTEGER NOT NULL,"
     "  tag INTEGER NOT NULL,"
     "  PRIMARY KEY (image, tag),"
     "  FOREIGN KEY (image) REFERENCES image(id) ON DELETE CASCADE,"
     "  FOREIGN KEY (tag) REFERENCES tag(id) ON DELETE CASCADE);",

     0};

static sqlite3 *db = NULL;

static int assert_sql(sqlite3 *db, int status, const char *filename, int linenum)
{
    switch (status) {
    case SQLITE_OK:
    case SQLITE_ROW:
    case SQLITE_DONE:
        break;
    case SQLITE_WARNING:
        fprintf(stderr, "%s:%i: SQL Warning! (%i)\n", filename, linenum, status);
        break;
    default:
        err(SQLITE_ERROR, "%s:%i: SQL Error! (%i)\n%s\n", filename, linenum, status,
            sqlite3_errmsg(db));
    }

    return status;
}

static void iter_images(sqlite3_stmt *stmt, image_callback callback)
{
    int rc;
    Image image;
    while ((rc = ASSERT_SQL(sqlite3_step(stmt))) == SQLITE_ROW) {
        image.id = sqlite3_column_int(stmt, 0);
        strncpy((char*) &image.title, (char*) sqlite3_column_text(stmt, 1), UTF8_MAX);
        strncpy((char*) &image.ext, (char*) sqlite3_column_text(stmt, 2), UTF8_MAX);

        // Exit early if the callback returns a nonzero status.
        if (callback(&image)) break;
    }

    if (rc != SQLITE_DONE)
        err(rc, "Unexpected SQLite status: %i\n", rc);
}

static void iter_tags(sqlite3_stmt *stmt, tag_callback callback)
{
    int rc;
    Tag tag;

    while ((rc = ASSERT_SQL(sqlite3_step(stmt))) == SQLITE_ROW) {
        tag.id = sqlite3_column_int(stmt, 0);
        strncpy((char*) &tag.name, (char*) sqlite3_column_text(stmt, 1), UTF8_MAX);

        // Exit early if the callback returns a nonzero status.
        if (callback(&tag)) break;
    }

    if (rc != SQLITE_DONE)
        err(rc, "Unexpected SQLite status %i\n", rc);
}


void tagmage_setup(const char *db_path)
{
    sqlite3_stmt *stmt = NULL;
    int rc = 0;
    int count = 0;
    char *errmsg = NULL;

    // Use builtin memory by default
    if (db_path == NULL)
        db_path = ":memory:";

    if (db) tagmage_cleanup();

    rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        err(rc, "Can't open database: %s\n",
            sqlite3_errmsg(db));
    }

    // double-check if the table is set up
    PREPARE(stmt, "SELECT COUNT(*) FROM sqlite_master WHERE type='table'"
            "AND name IN ('image', 'tag', 'image_tag')");
    rc = ASSERT_SQL(sqlite3_step(stmt));
    count = sqlite3_column_int(stmt, 0);

    if (count != 3) {
        // Set up the new database
        for (int i = 0; db_setup_queries[i] != 0; i++) {
            ASSERT_SQL(sqlite3_exec(db, db_setup_queries[i], NULL, NULL, NULL));
        }
    }

    // Set up pragmas
    ASSERT_SQL(sqlite3_exec(db, "PRAGMA foreign_keys=TRUE", NULL, NULL, &errmsg));
    ASSERT_SQL(sqlite3_exec(db, "PRAGMA encoding='UTF-8'",  NULL, NULL, &errmsg));
}

void tagmage_cleanup()
{
    sqlite3_close(db);
    db = NULL;
}


int tagmage_new_image(const char *title, const char *ext)
{
    sqlite3_stmt *stmt = NULL;
    int rc = 0;

    PREPARE(stmt, "INSERT INTO image (title, ext) VALUES (:title, :ext)");
    BIND_TEXT(stmt, ":title", title);
    BIND_TEXT(stmt, ":ext", ext);

    rc = ASSERT_SQL(sqlite3_step(stmt));
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE)
        err(1, "new_image: Unexpected SQLite status code %i\n", rc);

    return sqlite3_last_insert_rowid(db);
}

void tagmage_edit_title(int image_id, char *title)
{
    sqlite3_stmt *stmt = NULL;
    PREPARE(stmt,
            "UPDATE image SET title=:newtitle WHERE id=:imageid");

    BIND_TEXT(stmt, ":newtitle", title);
    BIND(int, stmt, ":imageid", image_id);

    int rc = ASSERT_SQL(sqlite3_step(stmt));
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
        err(1, "edit_image: Unexpected SQLite status code %i\n", rc);
}


void tagmage_add_tag(int image_id, char *tag_name)
{
    sqlite3_stmt *stmt;

    // Add tag if it doesn't exist
    PREPARE(stmt, "INSERT OR IGNORE INTO tag (name) VALUES (:tag)");
    BIND_TEXT(stmt, ":tag", tag_name);
    ASSERT_SQL(sqlite3_step(stmt));
    sqlite3_finalize(stmt);
    stmt = NULL;

    PREPARE(stmt,
            "INSERT INTO image_tag (image, tag) VALUES (:img, "
            "  (SELECT id FROM tag WHERE name=:tag))");

    BIND(int, stmt, ":img", image_id);
    BIND_TEXT(stmt, ":tag", tag_name);

    int rc = ASSERT_SQL(sqlite3_step(stmt));
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE)
        err(1, "add_tag: Unexpected SQLite status code %i\n", rc);
}

void tagmage_delete_image(int image_id)
{
    sqlite3_stmt *stmt = NULL;
    PREPARE(stmt, "DELETE FROM image WHERE id=:imageid");
    BIND(int, stmt, ":imageid", image_id);

    int rc = ASSERT_SQL(sqlite3_step(stmt));
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
        err(1, "delete_image: Unexpected SQLite status code %i\n", rc);
}


int tagmage_get_image(int image_id, Image *image)
{
    sqlite3_stmt *stmt = NULL;
    int rc;

    PREPARE(stmt, "SELECT title,ext FROM image WHERE id=:imageid");
    BIND(int, stmt, ":imageid", image_id);

    rc = ASSERT_SQL(sqlite3_step(stmt));
    // Image doesn't exist; return error
    if (rc == SQLITE_DONE)
        return -1;

    image->id = image_id;
    strncpy((char*) &image->title,
            (char*) sqlite3_column_text(stmt, 0), UTF8_MAX);
    strncpy((char*) &image->ext,
            (char*) sqlite3_column_text(stmt, 1), UTF8_MAX);

    sqlite3_finalize(stmt);

    return 0;
}

void tagmage_get_images(image_callback callback)
{
    sqlite3_stmt *stmt = NULL;

    PREPARE(stmt, "SELECT id,title,ext FROM image");

    iter_images(stmt, callback);
    sqlite3_finalize(stmt);
}

void tagmage_get_untagged_images(image_callback callback)
{
    sqlite3_stmt *stmt = NULL;
    PREPARE(stmt,
            "SELECT id, title, ext,  FROM image "
            "  WHERE id NOT IN (SELECT image FROM image_tag)");

    iter_images(stmt, callback);
    sqlite3_finalize(stmt);
}

void tagmage_get_images_by_tag(char *tag, image_callback callback)
{
    sqlite3_stmt *stmt = NULL;
    PREPARE(stmt,
            "SELECT id, title, ext FROM image"
            " WHERE id IN (SELECT image FROM image_tag"
            "   WHERE tag = (SELECT id FROM tag"
            "                 WHERE name=:tag))");

    BIND_TEXT(stmt, ":tag", tag);

    iter_images(stmt, callback);
    sqlite3_finalize(stmt);
}

void tagmage_search_images(int *tag_ids, image_callback callback) {
    err(1, "search_images: Unsupported operation.\n");
}


int tagmage_get_tag(int tag_id, Tag *tag)
{
    sqlite3_stmt *stmt = NULL;
    int rc;

    PREPARE(stmt, "SELECT name FROM tag WHERE id=:tagid");

    rc = ASSERT_SQL(sqlite3_step(stmt));
    // Tag doesn't exist; return error
    if (rc == SQLITE_DONE)
        return -1;

    tag->id = tag_id;
    strncpy((char*) &tag->name, (char*) sqlite3_column_text(stmt, 0),
            UTF8_MAX);

    return 0;
}

void tagmage_get_tags(tag_callback callback)
{
    sqlite3_stmt *stmt = NULL;
    PREPARE(stmt, "SELECT id, name FROM tag");

    iter_tags(stmt, callback);
    sqlite3_finalize(stmt);
}

void tagmage_get_tags_by_image(int image_id, tag_callback callback)
{
    sqlite3_stmt *stmt = NULL;
    PREPARE(stmt,
            "SELECT id, name FROM tag"
            " WHERE id IN (SELECT tag FROM image_tag"
            "                WHERE image = :imageid)");
    BIND(int, stmt, ":imageid", image_id);
    iter_tags(stmt, callback);
    sqlite3_finalize(stmt);
}
