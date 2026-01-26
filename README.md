
# TetoDB

**TetoDB** is a lightweight, disk-based relational database management system built from scratch in C++. It implements core database engine concepts including a custom B+ Tree indexing engine, a paged memory buffer pool (Pager), and a SQL-like command interface.

Designed for portability and performance, TetoDB relies on fixed-width binary serialization to ensure database files are consistent across Windows and Linux environments.

> **⚠️ IMPORTANT: DATA PERSISTENCE**
> TetoDB uses a **manual commit** system. Changes are held in the memory buffer pool and are NOT written to disk automatically. 
>
> * You **MUST** run the `.commit` command to save your changes.
> * Exiting the program (`.exit` or Ctrl+C) **WITHOUT** committing will result in the loss of all unsaved data.

## 🚀 Features

* **Persistent Storage:** Data is stored in binary files using fixed 4KB pages, mimicking real-world database page sizes.
* **B+ Tree Indexing:** Supports fast lookups, range scans, and range deletions on integer columns.
* **Buffer Pool (Pager):** Manages file I/O with an in-memory cache, supporting lazy writes and manual commits.
* **Cross-Platform:** Compiles and runs natively on both **Windows** (using `_commit`, `<io.h>`) and **Linux** (using `fsync`, `<unistd.h>`).
* **SQL-Like Interface:** Supports `CREATE`, `INSERT`, `SELECT`, and `DELETE` commands.
* **Performance Profiling:** Built-in execution timer measures the processing time of every command in milliseconds.
* **Script Execution:** Capable of running bulk commands from a text file for testing and seeding data.

## 🛠 Architecture

TetoDB is composed of several modular components:

1.  **Pager (`Pager.cpp`):** Handles low-level file I/O. It reads/writes 4KB blocks and manages the "Flush" strategy to persist data to disk.
2.  **B-Tree (`Btree.cpp`):** Implements a B+ Tree data structure for indexing. It supports splitting (for inserts) and merging (concepts for delete), ensuring the tree remains balanced.
3.  **Schema (`Schema.cpp`):** Defines the structure of tables (`Table`, `Column`, `Row`) and handles serialization/deserialization of row data into raw bytes.
4.  **Database Engine (`Database.cpp`):** Orchestrates the table metadata, manages the active tables, and executes high-level logic (e.g., deciding whether to use a full table scan or an index scan).
5.  **Command Parser (`CommandParser.cpp`):** Tokenizes and validates user input into structured command objects.

## 📦 Installation & Build

No external dependencies are required. You only need a C++ compiler (g++, clang, or MSVC).

### Compile on Windows or Linux
```bash
g++ *.cpp -o TetoDB

```

*Note for Windows users:* If you use PowerShell, you might need to run it as `./TetoDB.exe`.

## 📖 Usage

### Starting the Database

To open (or create) a database environment, run the executable with a name for your database files:

```bash
# Interactive Mode
./TetoDB my_db

```

### Running a Script

You can also pass a text file containing commands as a second argument:

```bash
# Execute commands from script.txt and then exit
./TetoDB my_db script

```

### Supported Commands

#### 1. Create Table

Create a new table. For integer columns, the third argument specifies if it is indexed (`1` = Index, `0` = No Index). For char columns, it specifies the length.

```sql
-- Create table 'users' with an index on 'id' and a 32-byte string for 'name'
CREATE TABLE users id int 1 name char 32 age int 0

```

#### 2. Insert Data

Insert a row into the table. String values **must** be quoted.

```sql
INSERT INTO users 42 "Blue Teto" 16
INSERT INTO users 10 "Teto" 31

```

#### 3. Select Data

Perform a full table scan or a range query.

```sql
-- Select all rows
SELECT FROM users

-- Range Scan: Select rows where 'id' is between 10 and 50 (inclusive)
-- Syntax: SELECT FROM <table> WHERE <col> <min> <max>
SELECT FROM users WHERE id 10 50

```

#### 4. Delete Data

Delete all rows or specific rows using a range.

```sql
-- Delete rows where 'id' is exactly 42
DELETE FROM users WHERE id 42 42

-- Delete all rows
DELETE FROM users

```

#### 5. System Commands

* `.commit`: **REQUIRED** to save changes. Flushes all dirty pages from memory to disk.
* `.tables`: Lists all tables in the database.
* `.schema <table>`: Shows the schema definition for a specific table.
* `.exit`: Closes the database and exits. **WARNING: Does not autosave.**

## 📂 File Format

TetoDB uses three types of binary files to store data:

* **`*.teto`**: The **Metadata/Catalog** file. Stores definitions of all tables, columns, and free lists (recycled row IDs).
* **`*_<table>.db`**: The **Heap File**. Stores the actual row data for a specific table.
* **`*_<table>_<col>.btree`**: The **Index File**. Stores the B+ Tree nodes (Internal and Leaf pages) for an indexed column.

## ⚠️ Limitations

* **No Comments Supported:** The parser does not handle comments (e.g., `#` or `--`) in script files or iterative mode. Each command must be in one **single line**. Lines must contain only **ONE** valid commands or be empty.
* **Strict Syntax:** Syntax validation is minimal. Commands must strictly follow the format shown above (e.g., correct number of arguments, proper quoting). Malformed commands may cause undefined behavior or crashes.
* **Fixed String Length:** Strings are strictly fixed-width (`char N`). If you insert a longer string, it is truncated.
* **Integer Keys Only:** B-Tree indexing is currently supported only for `int` columns.
* **Single User:** The database is not thread-safe and is designed for a single connection.

---

**Author:** duy-dustin-tong

**License:** MIT



