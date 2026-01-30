-- =========================================================
-- SCRIPT SQL: Creazione database + Inserimento dati + Vincoli
-- Database: spese.db (SQLite)
-- =========================================================

-- Abilitazione vincoli di chiave esterna
PRAGMA foreign_keys = ON;

-- =========================================================
-- a) CREAZIONE DEL DATABASE
-- =========================================================

CREATE TABLE IF NOT EXISTS categories (
    id   INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS expenses (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    date        TEXT    NOT NULL,
    amount      REAL    NOT NULL CHECK (amount > 0),
    category_id INTEGER NOT NULL,
    description TEXT,
    FOREIGN KEY (category_id) REFERENCES categories(id)
);

CREATE TABLE IF NOT EXISTS budgets (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    month       TEXT    NOT NULL,
    category_id INTEGER NOT NULL,
    amount      REAL    NOT NULL CHECK (amount > 0),
    UNIQUE (month, category_id),
    FOREIGN KEY (category_id) REFERENCES categories(id)
);

-- =========================================================
-- b) INSERIMENTO DATI DI ESEMPIO
-- =========================================================

PRAGMA foreign_keys = ON;

-- Categorie
INSERT INTO categories (name) VALUES ('Alimenti');
INSERT INTO categories (name) VALUES ('Sport');
INSERT INTO categories (name) VALUES ('Affitto');
INSERT INTO categories (name) VALUES ('Extra');

-- Budget 2025-12
INSERT INTO budgets (month, category_id, amount)
VALUES ('2025-12', (SELECT id FROM categories WHERE name = 'Alimenti'), 400);

INSERT INTO budgets (month, category_id, amount)
VALUES ('2025-12', (SELECT id FROM categories WHERE name = 'Sport'), 200);

INSERT INTO budgets (month, category_id, amount)
VALUES ('2025-12', (SELECT id FROM categories WHERE name = 'Affitto'), 570);

INSERT INTO budgets (month, category_id, amount)
VALUES ('2025-12', (SELECT id FROM categories WHERE name = 'Extra'), 50);

-- Budget 2026-01
INSERT INTO budgets (month, category_id, amount)
VALUES ('2026-01', (SELECT id FROM categories WHERE name = 'Alimenti'), 400);

INSERT INTO budgets (month, category_id, amount)
VALUES ('2026-01', (SELECT id FROM categories WHERE name = 'Sport'), 200);

INSERT INTO budgets (month, category_id, amount)
VALUES ('2026-01', (SELECT id FROM categories WHERE name = 'Affitto'), 570);

INSERT INTO budgets (month, category_id, amount)
VALUES ('2026-01', (SELECT id FROM categories WHERE name = 'Extra'), 50);

-- Spese 2025-12
INSERT INTO expenses (date, amount, category_id, description)
VALUES ('2025-12-10', 455,
        (SELECT id FROM categories WHERE name = 'Alimenti'), NULL);

INSERT INTO expenses (date, amount, category_id, description)
VALUES ('2025-12-30', 200,
        (SELECT id FROM categories WHERE name = 'Sport'), NULL);

INSERT INTO expenses (date, amount, category_id, description)
VALUES ('2025-12-22', 570,
        (SELECT id FROM categories WHERE name = 'Affitto'), NULL);

INSERT INTO expenses (date, amount, category_id, description)
VALUES ('2025-12-25', 256,
        (SELECT id FROM categories WHERE name = 'Extra'), NULL);

-- Spese 2026-01
INSERT INTO expenses (date, amount, category_id, description)
VALUES ('2026-01-07', 390,
        (SELECT id FROM categories WHERE name = 'Alimenti'), NULL);

INSERT INTO expenses (date, amount, category_id, description)
VALUES ('2026-01-30', 200,
        (SELECT id FROM categories WHERE name = 'Sport'), NULL);

INSERT INTO expenses (date, amount, category_id, description)
VALUES ('2026-01-22', 570,
        (SELECT id FROM categories WHERE name = 'Affitto'), NULL);

INSERT INTO expenses (date, amount, category_id, description)
VALUES ('2026-01-23', 29,
        (SELECT id FROM categories WHERE name = 'Extra'), NULL);

-- =========================================================
-- c) VINCOLI DI INTEGRITÀ (VISIBILI NEL CODICE)
-- =========================================================
-- PRIMARY KEY:
--   id INTEGER PRIMARY KEY AUTOINCREMENT
--
-- FOREIGN KEY:
--   FOREIGN KEY(category_id) REFERENCES categories(id)
--
-- CHECK:
--   amount REAL NOT NULL CHECK (amount > 0)
--
-- UNIQUE:
--   name TEXT NOT NULL UNIQUE
--   UNIQUE (month, category_id)
--
-- NOT NULL:
--   name, date, amount, category_id, month
--
-- Nota: in SQLite i vincoli FOREIGN KEY sono attivi
-- solo se PRAGMA foreign_keys = ON;
