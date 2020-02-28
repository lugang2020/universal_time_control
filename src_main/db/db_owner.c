#include "db_owner.h"
#include "../../third_party/sqlite/sqlite3.h"

#include <stdlib.h>
#include <stdio.h>

#include "../../third_party/tinycthread/tinycthread.h"

struct _db_owner {
	sqlite3* sql;
	mtx_t    mutex;
};

db_owner_t* db_owner_init() {
	MALLOC_TYPE(db_owner_t, d)

	if (!sqlite3_threadsafe()) {
		printf("sqlite has no threading\n");
		abort();
	}

	if (sqlite3_open("config.db", &d->sql) != SQLITE_OK) {
		free(d);
		printf("DB error\n");
		abort();
	}

	sqlite3_exec(d->sql, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);

	mtx_init(&d->mutex, mtx_plain);

	return d;
}

void db_owner_destroy(db_owner_t* d) {
	mtx_lock(&d->mutex);
	sqlite3_free(d->sql);
	mtx_unlock(&d->mutex);
	mtx_destroy(&d->mutex);
	free(d);
}

struct sqlite3* db_owner_lock_and_get_db(db_owner_t* d) {
	mtx_lock(&d->mutex);
	return d->sql;
}

void db_owner_surrender_db_and_unlock(db_owner_t* d) {
	mtx_unlock(&d->mutex);
}