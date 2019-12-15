#include <err.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "database.h"
#include "util.h"

#define CHECK_STATUS(RC) if (RC != SQLITE_OK && RC != SQLITE_DONE) {	\
		seterr();						\
		return -1;						\
		} do {} while(0)
// add "do {} while(0)" at end for semicolons

#define PREPARE(STMT, QUERY)					\
	sqlite3_prepare_v2(db, QUERY, -1, &(STMT), NULL)
#define BIND(TYPE, STMT, NAME, VAL)				\
	sqlite3_bind_##TYPE					\
	(STMT, sqlite3_bind_parameter_index(STMT, NAME), VAL)
#define BIND_TEXT(STMT, NAME, VAL)					\
	sqlite3_bind_text						\
	(STMT, sqlite3_bind_parameter_index(STMT, NAME), VAL, -1, NULL)

	static const char *db_setup_queries[] =
		{"CREATE TABLE image ("
		 "  id INTEGER PRIMARY KEY,"
		 "  title VARCHAR(" TITLE_MAX_STR ") NOT NULL); ",

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
static char err_buf[BUFF_MAX] = {0};

static void seterr()
{
	snprintf(err_buf, sizeof(err_buf),
                 "(%i) %s", sqlite3_errcode(db), sqlite3_errmsg(db));
}

static int iter_files(sqlite3_stmt *stmt, file_callback callback, void *arg)
{
	int rc;
	TMFile file;
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		file.id = sqlite3_column_int(stmt, 0);
		strncpy((char*) &file.title,
                        (char*) sqlite3_column_text(stmt, 1), TITLE_MAX);

		// Exit early if the callback returns a nonzero status.
		if (callback(&file, arg)) break;
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


const char *tmdb_get_error()
{
	return err_buf;
}

int tmdb_setup(const char *db_path)
{
	sqlite3_stmt *stmt = NULL;
	int rc = 0;
	int count = 0;

	// Use builtin memory by default
	if (db_path == NULL)
		db_path = ":memory:";

	if (db) {
		if (tmdb_cleanup() < 0)
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

int tmdb_cleanup()
{
	int rc = sqlite3_close(db);
	CHECK_STATUS(rc);

	if (rc == SQLITE_OK)
		db = NULL;

	return 0;
}


int tmdb_new_file(const char *title)
{
	sqlite3_stmt *stmt = NULL;
	int rc = 0;

	PREPARE(stmt,
                "INSERT INTO image (title) VALUES (:title)");
	BIND_TEXT(stmt, ":title", title);

	rc = sqlite3_step(stmt);
	CHECK_STATUS(rc);

	sqlite3_finalize(stmt);

	return sqlite3_last_insert_rowid(db);
}

int tmdb_edit_title(int file_id, const char *title)
{
	sqlite3_stmt *stmt = NULL;

	// Double-check it exists.
	if (tmdb_get_file(file_id, NULL) < 0)
		return -1;

	PREPARE(stmt,
                "UPDATE image SET title=:newtitle WHERE id=:fileid");

	BIND_TEXT(stmt, ":newtitle", title);
	BIND(int, stmt, ":fileid", file_id);

	int rc = sqlite3_step(stmt);
	CHECK_STATUS(rc);
	sqlite3_finalize(stmt);

	return 0;
}


int tmdb_add_tag(int file_id, const char *tag_name)
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

	BIND(int, stmt, ":img", file_id);
	BIND_TEXT(stmt, ":tag", tag_name);

	rc = sqlite3_step(stmt);
	CHECK_STATUS(rc);
	sqlite3_finalize(stmt);

	return 0;
}

int tmdb_remove_tag(int file_id, const char *tag_name)
{
	sqlite3_stmt *stmt;
	int rc;

	rc = PREPARE(stmt,
                     "DELETE FROM image_tag"
                     " WHERE image=:file"
                     " AND tag=(SELECT id FROM tag WHERE name=:tag)");
	CHECK_STATUS(rc);
	BIND(int, stmt, ":file", file_id);
	BIND_TEXT(stmt, ":tag", tag_name);

	rc = sqlite3_step(stmt);
	CHECK_STATUS(rc);
	sqlite3_finalize(stmt);

	return cleanup_tags();
}

int tmdb_delete_file(int file_id)
{
	sqlite3_stmt *stmt = NULL;
	int rc;

	PREPARE(stmt, "DELETE FROM image WHERE id=:fileid");
	BIND(int, stmt, ":fileid", file_id);

	rc = sqlite3_step(stmt);
	CHECK_STATUS(rc);
	sqlite3_finalize(stmt);

	return cleanup_tags();
}


int tmdb_get_file(int file_id, TMFile *file)
{
	sqlite3_stmt *stmt = NULL;
	int rc, status = 0;

	PREPARE(stmt, "SELECT title FROM image WHERE id=:file");
	BIND(int, stmt, ":file", file_id);

	rc = sqlite3_step(stmt);

	switch (rc) {
	case SQLITE_DONE:
		strncpy(err_buf, "File doesn't exist.", sizeof(err_buf));
		status = -1;
		break;
	case SQLITE_ROW:
		if (file) {
			file->id = file_id;
			strncpy((char*) &file->title,
                                (char*) sqlite3_column_text(stmt, 0), TITLE_MAX);
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

int tmdb_get_files(file_callback callback, void *arg)
{
	sqlite3_stmt *stmt = NULL;
	int rc;

	rc = PREPARE(stmt, "SELECT id,title FROM image");
	CHECK_STATUS(rc);

	rc = iter_files(stmt, callback, arg);
	sqlite3_finalize(stmt);

	return rc;
}


int tmdb_has_tag(int file_id, const char *tag_name) {
	sqlite3_stmt *stmt = NULL;
	int rc;

	rc = PREPARE(stmt,
                     "SELECT image FROM image_tag"
                     " WHERE image=:file"
                     " AND tag=(SELECT id FROM tag WHERE name=:tag)");
	BIND(int, stmt, ":file", file_id);
	BIND_TEXT(stmt, ":tag", tag_name);

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

int tmdb_get_tags(tag_callback callback)
{
	sqlite3_stmt *stmt = NULL;
	int status;

	PREPARE(stmt, "SELECT id, name FROM tag");

	status = iter_tags(stmt, callback);
	sqlite3_finalize(stmt);

	return status;
}

int tmdb_get_tags_by_file(int file_id, tag_callback callback)
{
	sqlite3_stmt *stmt = NULL;
	int status;

	PREPARE(stmt,
                "SELECT id, name FROM tag"
                " WHERE id IN (SELECT tag FROM image_tag"
                "                WHERE image=:file)");
	BIND(int, stmt, ":file", file_id);

	status = iter_tags(stmt, callback);
	sqlite3_finalize(stmt);

	return status;
}

int tmdb_has_tags(int file_id)
{
	sqlite3_stmt *stmt = NULL;
	int rc;

	PREPARE(stmt,
                "SELECT tag FROM image_tag "
                " WHERE image=:file");
	BIND(int, stmt, ":file", file_id);

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
