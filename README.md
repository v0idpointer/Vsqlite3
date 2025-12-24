# Vsqlite3

**Vsqlite3** is a lightweight C++20 single-header library wrapper for the SQLite3 C API.

## Features

- **Modern C++20** - Built from the ground up using C++20 features like concepts and `std::span`.
- **Header-only** - No need to compile anything, just include the `Vsqlite3.hpp` header file.
- **RAII guaranteed** - The library manages `sqlite3*` and `sqlite3_stmt*` handles.
- **Variadic binding and column extraction** - Bind multiple values or fetch multiple result columns in a single, type-safe function call.
- **Supports `std::optional`** - `std::nullopt` is mapped to `SQLITE_NULL` in both directions.
- **Custom type support** - Easily add support for custom types by specializing the `Binding<T>` template.
- **Exception-based** - `SqliteException` is thrown if an error occurs.

## Examples

- Connection and query execution

```cpp
#include <Vsqlite3/Vsqlite3.hpp>

using namespace Vsqlite3;

Database db = { "database.db", (DatabaseOpenFlags::ReadWrite | DatabaseOpenFlags::Create) };

db.Execute(R"(
	CREATE TABLE profiles (
		id INTEGER PRIMARY KEY,
		username TEXT NOT NULL,
		bio TEXT
	);
)");
```

- Data binding and fetching rows

```cpp
Statement stmt = db.PrepareStatement("INSERT INTO profiles (username, bio) VALUES (?, ?);");
stmt.Execute("alice", "Hello World!");
stmt.Execute("bob", std::nullopt);

stmt = db.PrepareStatement("SELECT id, username FROM profiles;");

std::int32_t id;
std::string username;
while (stmt.Fetch(id, username)) {
	std::cout << id << " " << username << std::endl;
}
```

- Handling `NULL`s with `std::optional`

```cpp
Statement stmt = db.PrepareStatement("SELECT username, bio FROM profiles;");

std::string username;
std::optional<std::string> bio;

while (stmt.Fetch(username, bio)) {
	
	if (bio.has_value()) {
		std::cout << username << "'s bio: " << bio.value() << std::endl;
	} else {
		std::cout << username << " has no bio." << std::endl;
    }

}
```

- Exception handling

```cpp
std::string query;
std::getline(std::cin, query);

try { db.Execute(query); }
catch (const SqliteException& ex) {
	std::cout << ex.what() << std::endl;
}
```

- Custom `Binding<T>` specialization

```cpp
namespace Vsqlite3 {

	template <>
	struct Binding<std::chrono::system_clock::time_point> {

		static inline auto Bind(sqlite3_stmt* const pStmt, const int index, const std::chrono::system_clock::time_point& arg) -> int {
			return sqlite3_bind_int64(pStmt, index, std::chrono::system_clock::to_time_t(arg));
		}

		static inline auto Column(sqlite3_stmt* const pStmt, const int column, std::chrono::system_clock::time_point& arg) -> void {
			arg = std::chrono::system_clock::from_time_t(sqlite3_column_int64(pStmt, column));
		}

	};

}

std::chrono::system_clock::time_point now;
db.PrepareStatement("SELECT unixepoch();").Fetch(now);

std::cout << now << std::endl;
```

## Configuration

You can customize the library's behavior using preprocessor definitions:

| Macro | Description  |
| ----- | ------------ |
| `VSQLITE_USE_WINSQLITE` | Includes `winsqlite/winsqlite3.h` instead of `sqlite3.h`. This feature is intended for applications running on Windows 10 and later. |
| `VSQLITE_NO_DEFAULT_BINDING_SPECIALIZATIONS` | Disables the default `Binding<T>` specializations. |

## Contributing

Contributions are welcome!

Open a pull request or an issue.

## License

[MIT License](https://opensource.org/license/MIT)

Copyright (c) 2025 [V0idPointer](https://v0idpointer.net)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.