#include <iostream>
#include <string>
#include <limits>
#include <sqlite3.h>

using namespace std;

/*
    PROGETTO: Gestione spese personali (C++ + SQLite)
    - Interfaccia: console (testuale)
    - DB relazionale SQL: SQLite (file "spese.db")
    - Tabelle: 1) categories: categorie (PK id, UNIQUE name)
               2) expenses: spese (FK category_id -> categories.id, description facoltativa)
               3) budgets: budget mensile (UNIQUE month+category_id, FK category_id)
    - Moduli:MODULO 1: Gestione categorie (con SELECT per controllo esistenza)
             MODULO 2: Inserimento spesa (con SELECT categoria + description facoltativa)
             MODULO 3: Budget mensile (INSERT o UPDATE con UPSERT)
             MODULO 4: Report (con sottomenu richiesto + switch)
*/

// -------------------- FUNZIONI DI UTILITÀ --------------------

void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

bool execSQL(sqlite3* db, const string& sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        cerr << "Errore SQL: " << (err ? err : "sconosciuto") << endl;
        sqlite3_free(err);
        return false;
    }
    return true;
}

bool isDigits(const string& s, int start, int endInclusive) {
    for (int i = start; i <= endInclusive; i++) {
        if (i < 0 || i >= (int)s.size()) return false;
        if (s[i] < '0' || s[i] > '9') return false;
    }
    return true;
}

bool isValidDateSimple(const string& date) {
    if (date.size() != 10) return false;
    if (date[4] != '-' || date[7] != '-') return false;
    return isDigits(date, 0, 3) && isDigits(date, 5, 6) && isDigits(date, 8, 9);
}

bool isValidMonthSimple(const string& month) {
    if (month.size() != 7) return false;
    if (month[4] != '-') return false;
    return isDigits(month, 0, 3) && isDigits(month, 5, 6);
}

// -------------------- CREAZIONE DATABASE (TABELLE + VINCOLI) --------------------

bool initDB(sqlite3* db) {
    if (!execSQL(db, "PRAGMA foreign_keys = ON;")) return false;

    string sql = R"SQL(
        CREATE TABLE IF NOT EXISTS categories(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE
        );

        CREATE TABLE IF NOT EXISTS expenses(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            date TEXT NOT NULL,
            amount REAL NOT NULL CHECK(amount > 0),
            category_id INTEGER NOT NULL,
            description TEXT,
            FOREIGN KEY(category_id) REFERENCES categories(id)
        );

        CREATE TABLE IF NOT EXISTS budgets(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            month TEXT NOT NULL,
            category_id INTEGER NOT NULL,
            amount REAL NOT NULL CHECK(amount > 0),
            UNIQUE(month, category_id),
            FOREIGN KEY(category_id) REFERENCES categories(id)
        );
    )SQL";

    return execSQL(db, sql);
}

// -------------------- FUNZIONI DB (SELECT) --------------------

bool categoryExists(sqlite3* db, const string& name) {
    const char* sql = "SELECT COUNT(*) FROM categories WHERE name = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        exists = (count > 0);
    }

    sqlite3_finalize(stmt);
    return exists;
}

int categoryExistsId(sqlite3* db, const string& name) {
    const char* sql = "SELECT id FROM categories WHERE name = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) id = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);
    return id;
}

// -------------------- MODULO 1: GESTIONE CATEGORIE --------------------

void addCategory(sqlite3* db) {
    cout << "\n--- MODULO 1: Gestione Categorie ---\n";
    cout << "Nome categoria: ";
    string name;
    getline(cin, name);

    if (name.empty()) {
        cout << "Errore: il nome della categoria non puo' essere vuoto.\n";
        return;
    }

    // SELECT richiesto
    if (categoryExists(db, name)) {
        cout << "La categoria esiste gia'.\n";
        return;
    }

    const char* sql = "INSERT INTO categories(name) VALUES(?);";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Errore: impossibile preparare la query.\n";
        return;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) cout << "Categoria inserita correttamente.\n";
    else cout << "Errore: inserimento fallito.\n";
}

// -------------------- MODULO 2: INSERIMENTO SPESE --------------------

void addExpense(sqlite3* db) {
    cout << "\n--- MODULO 2: Inserimento di una Spesa ---\n";

    string date, categoryName, description;
    double amount;

    cout << "Data (formato YYYY-MM-DD): ";
    getline(cin, date);

    if (date.empty() || !isValidDateSimple(date)) {
        cout << "Errore: formato data non valido. Usa YYYY-MM-DD.\n";
        return;
    }

    cout << "Importo: ";
    if (!(cin >> amount)) {
        clearInput();
        cout << "Errore: importo non valido.\n";
        return;
    }
    clearInput();

    cout << "Nome della categoria: ";
    getline(cin, categoryName);

    cout << "Descrizione (facoltativa): ";
    getline(cin, description);

    if (amount <= 0) {
        cout << "Errore: l’importo deve essere maggiore di zero.\n";
        return;
    }

    // SELECT richiesto
    int catId = categoryExistsId(db, categoryName);
    if (catId == -1) {
        cout << "Errore: la categoria non esiste.\n";
        return;
    }

    const char* sql =
        "INSERT INTO expenses(date, amount, category_id, description) VALUES(?,?,?,?);";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Errore: inserimento spesa fallito.\n";
        return;
    }

    sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, amount);
    sqlite3_bind_int(stmt, 3, catId);

    if (description.empty()) sqlite3_bind_null(stmt, 4);
    else sqlite3_bind_text(stmt, 4, description.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) cout << "Spesa inserita correttamente.\n";
    else cout << "Errore: inserimento spesa fallito.\n";
}

// -------------------- MODULO 3: BUDGET MENSILE --------------------

void setBudget(sqlite3* db) {
    cout << "\n--- MODULO 3: Definizione del Budget Mensile ---\n";

    string month, cat;
    double amount;

    cout << "Mese (YYYY-MM): ";
    getline(cin, month);

    if (month.empty() || !isValidMonthSimple(month)) {
        cout << "Errore: formato mese non valido. Usa YYYY-MM.\n";
        return;
    }

    cout << "Nome della categoria: ";
    getline(cin, cat);

    cout << "Importo del budget: ";
    if (!(cin >> amount)) {
        cout << "Errore: budget non valido.\n";
        clearInput();
        return;
    }
    clearInput();

    if (amount <= 0) {
        cout << "Errore: il budget deve essere maggiore di zero.\n";
        return;
    }

    // SELECT richiesto
    int catId = categoryExistsId(db, cat);
    if (catId == -1) {
        cout << "Errore: la categoria non esiste.\n";
        return;
    }

    // UPSERT richiesto (inserimento o aggiornamento)
    const char* sql =
        "INSERT INTO budgets(month, category_id, amount) VALUES(?,?,?) "
        "ON CONFLICT(month, category_id) DO UPDATE SET amount=excluded.amount;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Errore: salvataggio budget fallito.\n";
        return;
    }

    sqlite3_bind_text(stmt, 1, month.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, catId);
    sqlite3_bind_double(stmt, 3, amount);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) cout << "Budget mensile salvato correttamente.\n";
    else cout << "Errore: salvataggio budget fallito.\n";
}

// -------------------- REPORT --------------------

void reportTotalByCategory(sqlite3* db) {
    cout << "\n--- Report 1: Totale spese per categoria ---\n";

    const char* sql =
        "SELECT c.name, IFNULL(SUM(e.amount), 0) "
        "FROM categories c "
        "LEFT JOIN expenses e ON e.category_id = c.id "
        "GROUP BY c.id, c.name "
        "ORDER BY c.name;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Errore report.\n";
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* cname = sqlite3_column_text(stmt, 0);
        double total = sqlite3_column_double(stmt, 1);

        cout << "Categoria: " << (cname ? (const char*)cname : "")
             << " | Totale speso: " << total << "\n";
    }

    sqlite3_finalize(stmt);
}

void reportMonthVsBudget(sqlite3* db) {
    cout << "\n--- Report 2: Spese mensili vs budget ---\n";
    cout << "Mese (YYYY-MM): ";
    string month;
    getline(cin, month);

    if (month.empty() || !isValidMonthSimple(month)) {
        cout << "Errore: formato mese non valido. Usa YYYY-MM.\n";
        return;
    }

    const char* sql =
        "SELECT c.name, b.amount, IFNULL(SUM(e.amount),0) "
        "FROM categories c "
        "LEFT JOIN budgets b ON b.category_id=c.id AND b.month=? "
        "LEFT JOIN expenses e ON e.category_id=c.id AND substr(e.date,1,7)=? "
        "GROUP BY c.id, c.name, b.amount "
        "ORDER BY c.name;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Errore report.\n";
        return;
    }

    sqlite3_bind_text(stmt, 1, month.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, month.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* cname = sqlite3_column_text(stmt, 0);

        bool hasBudget = sqlite3_column_type(stmt, 1) != SQLITE_NULL;
        double budget = hasBudget ? sqlite3_column_double(stmt, 1) : 0.0;

        double spent = sqlite3_column_double(stmt, 2);

        cout << "\nCategoria: " << (cname ? (const char*)cname : "") << "\n";
        cout << "Speso: " << spent << "\n";

        if (!hasBudget) {
            cout << "Budget: (non definito)\n";
            cout << "Stato: NESSUN BUDGET\n";
        } else {
            cout << "Budget: " << budget << "\n";
            if (spent > budget) {
                cout << "Stato: SUPERAMENTO\n";
                cout << "Superamento di: " << (spent - budget) << "\n";
            } else {
                cout << "Stato: OK\n";
            }
        }
    }

    sqlite3_finalize(stmt);
}

void reportAllExpenses(sqlite3* db) {
    cout << "\n--- Report 3: Elenco completo delle spese (ordinate per data) ---\n";

    const char* sql =
        "SELECT e.date, c.name, e.amount, IFNULL(e.description,'') "
        "FROM expenses e JOIN categories c ON c.id=e.category_id "
        "ORDER BY e.date;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        cout << "Errore report.\n";
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* d = sqlite3_column_text(stmt, 0);
        const unsigned char* c = sqlite3_column_text(stmt, 1);
        double a = sqlite3_column_double(stmt, 2);
        const unsigned char* s = sqlite3_column_text(stmt, 3);

        cout << "Data: " << (d ? (const char*)d : "")
             << " | Categoria: " << (c ? (const char*)c : "")
             << " | Importo: " << a
             << " | Desc: " << (s ? (const char*)s : "") << "\n";
    }

    sqlite3_finalize(stmt);
}

void reportsMenu(sqlite3* db) {
    int ch = 0;
    do {
        cout << "\n--- MENU DEI REPORT ---\n";
        cout << "1) Totale spese per categoria\n";
        cout << "2) Spese mensili vs budget\n";
        cout << "3) Elenco completo delle spese ordinate per data\n";
        cout << "4) Ritorna al menu principale\n";
        cout << "Scelta: ";

        if (!(cin >> ch)) {
            clearInput();
            cout << "Scelta non valida.\nRiprovare.\n";
            continue;
        }
        clearInput();

        switch (ch) {
            case 1: reportTotalByCategory(db); break;
            case 2: reportMonthVsBudget(db); break;
            case 3: reportAllExpenses(db); break;
            case 4: break;
            default:
                cout << "Scelta non valida.\nRiprovare.\n";
                break;
        }
    } while (ch != 4);
}

// -------------------- SEED DATA (DATI DEMO) --------------------
// l'utente sceglie se caricare i dati demo, prima del seed svuotiamo expenses e budgets (così non si duplicano).

void ensureCategory(sqlite3* db, const string& name) {
    if (categoryExists(db, name)) return;

    const char* sql = "INSERT INTO categories(name) VALUES(?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void upsertBudget(sqlite3* db, const string& month, const string& categoryName, double amount) {
    int catId = categoryExistsId(db, categoryName);
    if (catId == -1) return;

    const char* sql =
        "INSERT INTO budgets(month, category_id, amount) VALUES(?,?,?) "
        "ON CONFLICT(month, category_id) DO UPDATE SET amount=excluded.amount;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

    sqlite3_bind_text(stmt, 1, month.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, catId);
    sqlite3_bind_double(stmt, 3, amount);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void insertExpense(sqlite3* db, const string& date, const string& categoryName, double amount, const string& desc = "") {
    int catId = categoryExistsId(db, categoryName);
    if (catId == -1) return;

    const char* sql =
        "INSERT INTO expenses(date, amount, category_id, description) VALUES(?,?,?,?);";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

    sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, amount);
    sqlite3_bind_int(stmt, 3, catId);

    if (desc.empty()) sqlite3_bind_null(stmt, 4);
    else sqlite3_bind_text(stmt, 4, desc.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void seedData(sqlite3* db) {
    // Svuoto solo le tabelle "variabili" per evitare duplicazioni nelle demo
    // (NON cancello categories così restano eventuali categorie aggiunte dall'utente)
    execSQL(db, "DELETE FROM expenses;");
    execSQL(db, "DELETE FROM budgets;");

    // Categorie 
    ensureCategory(db, "Alimenti");
    ensureCategory(db, "Sport");
    ensureCategory(db, "Affitto");
    ensureCategory(db, "Extra");

    // Budget 2025-12
    upsertBudget(db, "2025-12", "Alimenti", 300);
    upsertBudget(db, "2025-12", "Sport", 200);
    upsertBudget(db, "2025-12", "Affitto", 570);
    upsertBudget(db, "2025-12", "Extra", 50);

    // Budget 2026-01
    upsertBudget(db, "2026-01", "Alimenti", 400);
    upsertBudget(db, "2026-01", "Sport", 200);
    upsertBudget(db, "2026-01", "Affitto", 570);
    upsertBudget(db, "2026-01", "Extra", 30);

    // Spese 2025-12
    insertExpense(db, "2025-12-10", "Alimenti", 455);
    insertExpense(db, "2025-12-30", "Sport", 200);
    insertExpense(db, "2025-12-22", "Affitto", 570);
    insertExpense(db, "2025-12-25", "Extra", 256);

    // Spese 2026-01
    insertExpense(db, "2026-01-07", "Alimenti", 390);
    insertExpense(db, "2026-01-30", "Sport", 200);
    insertExpense(db, "2026-01-22", "Affitto", 570);
    insertExpense(db, "2026-01-23", "Extra", 29);
}

// -------------------- MAIN --------------------

int main() {
    sqlite3* db = nullptr;

    if (sqlite3_open("spese.db", &db) != SQLITE_OK) {
        cerr << "Impossibile aprire DB.\n";
        return 1;
    }

    if (!initDB(db)) {
        cerr << "Errore init DB.\n";
        sqlite3_close(db);
        return 1;
    }

    // Saluto 
    char tasto;
    cout << "Benvenuti nel sistema di gestione delle spese personali." << endl;
    cout << "Premi un tasto e INVIO per continuare." << endl;
    cin >> tasto;
    clearInput();

    // Scelta semplice: caricare o no i dati demo (evita duplicazioni)
    char sceltaDemo;
    cout << "\nVuoi caricare i dati di esempio (demo)? (S/N): ";
    cin >> sceltaDemo;
    clearInput();

    if (sceltaDemo == 'S' || sceltaDemo == 's') {
        seedData(db);
        cout << "Dati demo caricati.\n";
    } else {
        cout << "Dati demo NON caricati.\n";
    }

    int choice = 0;
    do {
        cout << "\n=== MENU PRINCIPALE ===\n";
        cout << "1) Gestione Categorie\n";
        cout << "2) Inserimento Spesa\n";
        cout << "3) Definizione Budget Mensile\n";
        cout << "4) Visualizzazione Report\n";
        cout << "5) Esci\n";
        cout << "Scelta: ";

        if (!(cin >> choice)) {
            clearInput();
            cout << "Scelta non valida.\nRiprovare.\n";
            continue;
        }
        clearInput();

        switch (choice) {
            case 1: addCategory(db); break;
            case 2: addExpense(db); break;
            case 3: setBudget(db); break;
            case 4: reportsMenu(db); break;
            case 5: cout << "Uscita...\n"; break;
            default:
                cout << "Scelta non valida.\nRiprovare.\n";
                break;
        }
    } while (choice != 5);

    sqlite3_close(db);
    return 0;
}
