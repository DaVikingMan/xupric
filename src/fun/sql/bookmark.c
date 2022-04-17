#include <sqlite3.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "util/util.h"
#include "bookmark.h"

static sqlite3 *db;
static struct bookmark *books;
static int books_len;

void bookmark_init(char *file)
{
	char *sql;

	if (sqlite3_open(file, &db))
		die(1, "[ERROR] Unable to open the bookmarks database\n");

	sql = "CREATE TABLE books(id INTEGER NOT NULL UNIQUE" \
		", uri TEXT NOT NULL, PRIMARY KEY (id AUTOINCREMENT));";

	/* ignore already exists errors */
	sqlite3_exec(db, sql, NULL, NULL, NULL);
}

void bookmark_add(char *text)
{
	char *sql;
	char *err;

	sql = ecalloc(1500, sizeof(char));

	strcpy(sql, "INSERT INTO books(uri) VALUES(\'");
	strcat(sql, text);
	strcat(sql, "\');");

	sqlite3_exec(db, sql, NULL, NULL, &err);

	if (err)
		printf("Failed to add bookmark: %s\n", err);

	efree(sql);
}

void bookmark_remove(struct bookmark *b)
{
	char *sql;
	char *err;
	char id[11];

	sql = ecalloc(1500, sizeof(char));

	snprintf(id, 11, "%i", b->id);
	strcpy(sql, "DELETE FROM books WHERE id = \'");
	strcat(sql, id);
	strcat(sql, "\';");

	sqlite3_exec(db, sql, NULL, NULL, &err);

	if (err)
		printf("Failed to remove bookmark: %s\n", err);

	efree(sql);
}

void bookmark_remove_by_uri(char *uri)
{
	char *sql;
	char *err;

	sql = ecalloc(1500, sizeof(char));

	strcpy(sql, "DELETE FROM books WHERE uri = \'");
	strcat(sql, uri);
	strcat(sql, "\';");

	sqlite3_exec(db, sql, NULL, NULL, &err);

	if (err)
		printf("Failed to remove bookmark: %s\n", err);

	efree(sql);
}

struct bookmark *bookmark_get(void)
{
	char *sql;
	sqlite3_stmt *stmt;
	int i;

	sql = "SELECT COUNT(*) AS count FROM books;";
	if (sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL))
		die(1, "[ERROR] Unable to get bookmarks: %s\n", sqlite3_errmsg(db));

	sqlite3_step(stmt);
	/* +1 so it won't try to alloc a 0 size buffer (error) */
	books = erealloc(books, sqlite3_column_int(stmt, 0) * sizeof(struct bookmark) + 1);
	sqlite3_finalize(stmt);

	sql = "SELECT * FROM books;";
	if (sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL))
		die(1, "[ERROR] Unable to get bookmarks: %s\n", sqlite3_errmsg(db));

	for (i = 0; sqlite3_step(stmt) == SQLITE_ROW; i++) {
		books[i].id = sqlite3_column_int(stmt, 0);
		books[i].uri = strdup(sqlite3_column_text(stmt, 1));
	}

	books_len = i;
	sqlite3_finalize(stmt);
	return books;
}

struct bookmark *bookmark_p_get(void)
{
	return books;
}

int books_len_get(void)
{
	return books_len;
}

int bookmark_exists(char *book)
{
	char *sql;
	sqlite3_stmt *stmt;
	int exists = 0;

	sql = ecalloc(1500, sizeof(char));

	strcpy(sql, "SELECT uri FROM books WHERE uri = \'");
	strcat(sql, book);
	strcat(sql, "\';");

	if (sqlite3_prepare_v2(db, sql, strlen(sql), &stmt, NULL))
		die(1, "[ERROR] Unable to get bookmarks: %s\n", sqlite3_errmsg(db));

	if (sqlite3_step(stmt) == SQLITE_ROW)
		exists = 1;

	sqlite3_finalize(stmt);
	efree(sql);
	return exists;
}

void bookmark_cleanup(void)
{
	int i;

	for (i = 0; i < books_len; i++)
		efree(books[i].uri);
	efree(books);

	sqlite3_close(db);
}
