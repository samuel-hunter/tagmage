#include <err.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "database.h"
#include "util.h"

#define CHECK_STATUS(RC) if (RC != SQLITE_OK && RC != SQLITE_DONE) { \
    seterr(); \
    return -1; \
    } do {} while(0)
// add "do {} while(0)" at end for semicolons

#define PREPARE(STMT, QUERY) \
    sqlite3_prepare_v2(db, QUERY, -1, &(STMT), NULL)
#define BIND(TYPE, STMT, NAME, VAL) \
    sqlite3_bind_##TYPE \
    (STMT, sqlite3_bind_parameter_index(STMT, NAME), VAL)
#define BIND_TEXT(STMT, NAME, VAL) \
    sqlite3_bind_text \
    (STMT, sqlite3_bind_parameter_index(STMT, NAME), VAL, -1, NULL)

#define BUFFER_MAX 4096

static const char *db_setup_queries[] =
    {"CREATE TABLE image ("
     "  id INTEGER PRIMARY KEY,"
     "  title VARCHAR(" TITLE_MAX_STR ") NOT NULL,"
     "  ext VARCHAR(" EXT_MAX_STR ") );",

     "CREATE TABLE tag ("
     "  id INTEGER PRIMARY KEY,"
     "  name VARCHAR(" TAG_MAX_STR ") UNIQUE NOT NULL );",

     "CREATE TABLE image_tag ("
     "  image INTEGER NOT NULL,"
     "  tag INTEGER NOT NULL,"
     "  PRIMARY KEY (image, tag),"
     "  FOREIGN KEY (image) REFERENCES image(id) ON DELETE CASCADE,"
     "  FOREIGN KEY (tag) REFERENCES tag(id) ON DELETE CASCADE);",

     0};

static sqlite3 *db = NULL;
static char tagmage_err_buf[BUFFER_MAX + 1] = {0};

static void seterr()
{
    snprintf(tagmage_err_buf, BUFFER_MAX,
               "(%i) %s", sqlite3_errcode(db), sqlite3_errmsg(db));
}

static int iter_images(sqlite3_stmt *stmt, image_callback callback, void *arg)
{
    int rc;
    Image image;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        image.id = sqlite3_column_int(stmt, 0);
        strncpy((char*) &image.title,
                (char*) sqlite3_column_text(stmt, 1), TITLE_MAX);
        strncpy((char*) &image.ext,
                (char*) sqlite3_column_text(stmt, 2), EXT_MAX);

        // Exit early if the callback returns a nonzero status.
        if (callback(&image, arg)) break;
    }

    if (rc != SQLITE_DONE) {
        seterr();
        return -1;
    }

    return 0;
}

static int iter_tags(sqlite3_stmt *stmt, tag_callback callback)
{
    int rc;
    char tag[TAG_MAX + 1];

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        strncpy(tag, (char*) sqlite3_column_text(stmt, 1), TAG_MAX);

        // Exit early if the callback returns a nonzero status.
        if (callback(tag)) break;
    }

    if (rc != SQLITE_DONE) {
        seterr();
        return -1;
    }

    return 0;
}

static int cleanup_tags()
{
    int rc = sqlite3_exec(db,
                      "DELETE FROM tag WHERE id NOT IN"
                      " (SELECT tag FROM image_tag)",
                      NULL, NULL, NULL);
    CHECK_STATUS(rc);

    return 0;
}


void tagmage_warn()
{
    warnx("%s", tagmage_err_buf);
}

int tagmage_setup(const char *db_path)
{
    sqlite3_stmt *stmt = NULL;
    int rc = 0;
    int count = 0;

    // Use builtin memory by default
    if (db_path == NULL)
        db_path = ":memory:";

    if (db) {
        if (tagmage_cleanup() < 0)
            return -1;
    }

    rc = sqlite3_open(db_path, &db);
    CHECK_STATUS(rc);

    // double-check if the table is set up
    rc = PREPARE(stmt,
            "SELECT COUNT(*) FROM sqlite_master WHERE type='table'"
            "AND name IN ('image', 'tag', 'image_tag')");
    CHECK_STATUS(rc);

    rc = sqlite3_step(stmt);

    count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (count != 3) {
        // Set up the new database
        for (int i = 0; db_setup_queries[i] != 0; i++) {
            rc = sqlite3_exec(db, db_setup_queries[i], NULL,
                              NULL, NULL);
            CHECK_STATUS(rc);
        }
    }

    // Set up pragmas
    rc = sqlite3_exec(db, "PRAGMA foreign_keys=TRUE",
                      NULL, NULL, NULL);
    CHECK_STATUS(rc);

    rc = sqlite3_exec(db, "PRAGMA encoding='UTF-8'",
                      NULL, NULL, NULL);
    CHECK_STATUS(rc);

    return 0;
}

int tagmage_cleanup()
{
    int rc = sqlite3_close(db);
    CHECK_STATUS(rc);

    if (rc == SQLITE_OK)
        db = NULL;

    return 0;
}


int tagmage_new_image(const char *title, const char *ext)
{
    sqlite3_stmt *stmt = NULL;
    int rc = 0;

    PREPARE(stmt,
            "INSERT INTO image (title, ext) VALUES (:title, :ext)");
    BIND_TEXT(stmt, ":title", title);
    BIND_TEXT(stmt, ":ext", ext);

    rc = sqlite3_step(stmt);
    CHECK_STATUS(rc);

    sqlite3_finalize(stmt);

    return sqlite3_last_insert_rowid(db);
}

int tagmage_edit_title(int image_id, char *title)
{
    sqlite3_stmt *stmt = NULL;

    // Double-check it exists.
    if (tagmage_get_image(image_id, NULL) < 0)
        return -1;

    PREPARE(stmt,
            "UPDATE image SET title=:newtitle WHERE id=:imageid");

    BIND_TEXT(stmt, ":newtitle", title);
    BIND(int, stmt, ":imageid", image_id);

    int rc = sqlite3_step(stmt);
    CHECK_STATUS(rc);
    sqlite3_finalize(stmt);

    return 0;
}


int tagmage_add_tag(int image_id, char *tag_name)
{
    sqlite3_stmt *stmt;
    int rc;

    // Add tag if it doesn't exist
    PREPARE(stmt, "INSERT OR IGNORE INTO tag (name) VALUES (:tag)");
    BIND_TEXT(stmt, ":tag", tag_name);
    rc = sqlite3_step(stmt);
    CHECK_STATUS(rc);
    sqlite3_finalize(stmt);
    stmt = NULL;

    PREPARE(stmt,
            "INSERT INTO image_tag (image, tag) VALUES (:img, "
            "  (SELECT id FROM tag WHERE name=:tag))");

    BIND(int, stmt, ":img", image_id);
    BIND_TEXT(stmt, ":tag", tag_name);

    rc = sqlite3_step(stmt);
    CHECK_STATUS(rc);
    sqlite3_finalize(stmt);

    return 0;
}

int tagmage_remove_tag(int image_id, char *tag_name)
{
    sqlite3_stmt *stmt;
    int rc;

    rc = PREPARE(stmt,
                 "DELETE FROM image_tag"
                 " WHERE image=:img"
                 " AND tag=(SELECT id FROM tag WHERE name=:tag)");
    CHECK_STATUS(rc);
    BIND(int, stmt, ":img", image_id);
    BIND_TEXT(stmt, ":tag", tag_name);

    rc = sqlite3_step(stmt);
    CHECK_STATUS(rc);
    sqlite3_finalize(stmt);

    return cleanup_tags();
}

int tagmage_delete_image(int image_id)
{
    sqlite3_stmt *stmt = NULL;
    int rc;

    PREPARE(stmt, "DELETE FROM image WHERE id=:imageid");
    BIND(int, stmt, ":imageid", image_id);

    rc = sqlite3_step(stmt);
    CHECK_STATUS(rc);
    sqlite3_finalize(stmt);

    return cleanup_tags();
}


int tagmage_get_image(int image_id, Image *image)
{
    sqlite3_stmt *stmt = NULL;
    int rc, status = 0;

    PREPARE(stmt, "SELECT title,ext FROM image WHERE id=:imageid");
    BIND(int, stmt, ":imageid", image_id);

    rc = sqlite3_step(stmt);

    switch (rc) {
    case SQLITE_DONE:
        strncpy(tagmage_err_buf, "Image doesn't exist.", BUFFER_MAX);
        status = -1;
        break;
    case SQLITE_ROW:
        if (image) {
            image->id = image_id;
            strncpy((char*) &image->title,
                    (char*) sqlite3_column_text(stmt, 0), TITLE_MAX);
            strncpy((char*) &image->ext,
                    (char*) sqlite3_column_text(stmt, 1), EXT_MAX);
        }
        break;
    default:
        seterr();
        status = -1;
        break;
    }

    sqlite3_finalize(stmt);
    return status;
}

int tagmage_get_images(image_callback callback, void *arg)
{
    sqlite3_stmt *stmt = NULL;
    int rc;

    rc = PREPARE(stmt, "SELECT id,title,ext FROM image");
    CHECK_STATUS(rc);

    rc = iter_images(stmt, callback, arg);
    sqlite3_finalize(stmt);

    return rc;
}

int tagmage_get_untagged_images(image_callback callback, void *arg)
{
    sqlite3_stmt *stmt = NULL;
    int status;

    PREPARE(stmt,
            "SELECT id, title, ext FROM image "
            "  WHERE id NOT IN (SELECT image FROM image_tag)");

    status = iter_images(stmt, callback, arg);
    sqlite3_finalize(stmt);

    return status;
}

int tagmage_get_images_by_tag(char *tag, image_callback callback, void *arg)
{
    sqlite3_stmt *stmt = NULL;
    int status;

    PREPARE(stmt,
            "SELECT id, title, ext FROM image"
            " WHERE id IN (SELECT image FROM image_tag"
            "   WHERE tag = (SELECT id FROM tag"
            "                 WHERE name=:tag))");

    BIND_TEXT(stmt, ":tag", tag);

    status = iter_images(stmt, callback, arg);
    sqlite3_finalize(stmt);

    return status;
}


int tagmage_has_tag(int image_id, char *tag_name) {
    sqlite3_stmt *stmt = NULL;
    int rc;

    rc = PREPARE(stmt,
            "SELECT image FROM image_tag"
            " WHERE image=:image"
            " AND tag=(SELECT id FROM tag WHERE name=:tag)");
    BIND(int, stmt, ":image", image_id);
    rc = BIND_TEXT(stmt, ":tag", tag_name);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    switch (rc) {
    case SQLITE_DONE:
        return 0;
    case SQLITE_ROW:
        return 1;
    default:
        seterr();
        return -1;
    }
}

int tagmage_get_tags(tag_callback callback)
{
    sqlite3_stmt *stmt = NULL;
    int status;

    PREPARE(stmt, "SELECT id, name FROM tag");

    status = iter_tags(stmt, callback);
    sqlite3_finalize(stmt);

    return status;
}

int tagmage_get_tags_by_image(int image_id, tag_callback callback)
{
    sqlite3_stmt *stmt = NULL;
    int status;

    PREPARE(stmt,
            "SELECT id, name FROM tag"
            " WHERE id IN (SELECT tag FROM image_tag"
            "                WHERE image = :imageid)");
    BIND(int, stmt, ":imageid", image_id);

    status = iter_tags(stmt, callback);
    sqlite3_finalize(stmt);

    return status;
}