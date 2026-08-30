-- Compiled from EnLang Database DSL (tournament_db.enlgdb)
-- Target Dialect: SQLITE

CREATE TABLE IF NOT EXISTS "players" (
    "id" INTEGER PRIMARY KEY AUTOINCREMENT,
    "username" TEXT NOT NULL UNIQUE,
    "team" TEXT NOT NULL,
    "rating" REAL DEFAULT 1000.0,
    "kills" INTEGER DEFAULT 0,
    "is_mvp" INTEGER DEFAULT False
);

INSERT INTO "players" ("username", "team", "rating", "kills", "is_mvp") VALUES ('ShadowNinja', 'Vortex Gaming', 1540.5, 42, 1);

INSERT INTO "players" ("username", "team", "rating", "kills", "is_mvp") VALUES ('Specter', 'Phoenix Squad', 1420.0, 36, 0);

INSERT INTO "players" ("username", "team", "rating", "kills", "is_mvp") VALUES ('CyberPulse', 'Vortex Gaming', 1610.2, 55, 1);

SELECT * FROM "players" WHERE ("is_mvp" = 1) ORDER BY "rating" DESC;

SELECT "username", "team", "kills", "rating" FROM "players" WHERE ("kills" >= 40) ORDER BY "kills" DESC LIMIT 5;

UPDATE "players" SET "rating" = ("rating" + 25.0) WHERE ("username" = 'Specter');

CREATE TABLE IF NOT EXISTS "temp_logs" (
    "id" INTEGER PRIMARY KEY AUTOINCREMENT,
    "message" TEXT NOT NULL
);

DROP TABLE IF EXISTS "temp_logs";
