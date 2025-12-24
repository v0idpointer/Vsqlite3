/*
	Vsqlite3
	Copyright (c) 2025 V0idPointer
	Licensed under the MIT License
*/

#ifndef __VSQLITE3_HPP__
#define __VSQLITE3_HPP__

#include <functional>
#include <type_traits>
#include <concepts>
#include <string>
#include <string_view>
#include <optional>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>
#include <algorithm>
#include <cstring>
#include <stdexcept>

#ifdef VSQLITE_USE_WINSQLITE
#include <winsqlite/winsqlite3.h>
#else
#include <sqlite3.h>
#endif

namespace Vsqlite3 {

	template <typename T, T InvalidHandleValue>
	class Handle {

	public:
		using Type = typename T;
		static constexpr T InvalidHandle = InvalidHandleValue;

		Handle(void);
		Handle(T handle, std::function<void(T)> releaseFn);
		Handle(const Handle&) = delete;
		Handle(Handle&& other) noexcept;
		~Handle(void);

		auto operator= (const Handle&) -> Handle & = delete;
		auto operator= (Handle&& other) noexcept -> Handle&;

		auto Get(void) const -> T;
		auto GetAddressOf(void) const -> std::add_const_t<T>*;
		auto GetAddressOf(void) -> T*;

		auto Release(void) -> bool;
		auto Reset(void) -> void;

	private:
		T m_handle;
		std::function<void(T)> m_releaseFn;

	};

	template <typename T, T InvalidHandleValue>
	inline Handle<T, InvalidHandleValue>::Handle()
		: Handle(InvalidHandleValue, nullptr) { }

	template <typename T, T InvalidHandleValue>
	inline Handle<T, InvalidHandleValue>::Handle(T handle, std::function<void(T)> releaseFn)
		: m_handle(handle), m_releaseFn(std::move(releaseFn)) { }

	template <typename T, T InvalidHandleValue>
	inline Handle<T, InvalidHandleValue>::Handle(Handle&& other) noexcept : Handle() {
		this->operator= (std::move(other));
	}

	template <typename T, T InvalidHandleValue>
	inline Handle<T, InvalidHandleValue>::~Handle() {
		if (!this->Release()) this->Reset();
	}

	template <typename T, T InvalidHandleValue>
	inline auto Handle<T, InvalidHandleValue>::operator= (Handle&& other) noexcept -> Handle& {

		if (this != &other) {

			this->Release();
			this->m_handle = std::move(other.m_handle);
			this->m_releaseFn = std::move(other.m_releaseFn);

			other.Reset();

		}

		return static_cast<Handle&>(*this);
	}

	template <typename T, T InvalidHandleValue>
	inline auto Handle<T, InvalidHandleValue>::Get() const -> T {
		return this->m_handle;
	}

	template <typename T, T InvalidHandleValue>
	inline auto Handle<T, InvalidHandleValue>::GetAddressOf(void) const -> std::add_const_t<T>* {
		return &this->m_handle;
	}

	template <typename T, T InvalidHandleValue>
	inline auto Handle<T, InvalidHandleValue>::GetAddressOf(void) -> T* {
		return const_cast<T*>(static_cast<const Handle&>(*this).GetAddressOf());
	}

	template <typename T, T InvalidHandleValue>
	inline auto Handle<T, InvalidHandleValue>::Release() -> bool {

		if ((this->m_handle != InvalidHandleValue) && this->m_releaseFn) {
			this->m_releaseFn(this->m_handle);
			this->Reset();
			return true;
		}

		return false;
	}

	template <typename T, T InvalidHandleValue>
	inline auto Handle<T, InvalidHandleValue>::Reset() -> void {
		this->m_handle = InvalidHandleValue;
		this->m_releaseFn = nullptr;
	}

	class SqliteException : public std::runtime_error {

	public:
		SqliteException(const std::string& message, const int extendedErrorCode);
		SqliteException(sqlite3* const pDb);
		SqliteException(sqlite3_stmt* const pStmt);

		auto GetExtendedErrorCode(void) const -> int;
		auto GetPrimaryErrorCode(void) const -> int;

	private:
		int m_extendedErrorCode;

	};

	inline SqliteException::SqliteException(const std::string& message, const int extendedErrorCode)
		: std::runtime_error(message), m_extendedErrorCode(extendedErrorCode) { }

	inline SqliteException::SqliteException(sqlite3* const pDb)
		: SqliteException(sqlite3_errmsg(pDb), sqlite3_extended_errcode(pDb)) { }

	inline SqliteException::SqliteException(sqlite3_stmt* const pStmt)
		: SqliteException(sqlite3_db_handle(pStmt)) { }

	inline auto SqliteException::GetExtendedErrorCode() const -> int {
		return this->m_extendedErrorCode;
	}

	inline auto SqliteException::GetPrimaryErrorCode() const -> int {
		return (this->m_extendedErrorCode & 0xFF);
	}

	enum class DatabaseOpenFlags : int {

		None = 0,

		ReadOnly = SQLITE_OPEN_READONLY,
		ReadWrite = SQLITE_OPEN_READWRITE,
		Create = SQLITE_OPEN_CREATE,

		Uri = SQLITE_OPEN_URI,
		Memory = SQLITE_OPEN_MEMORY,
		NoMutex = SQLITE_OPEN_NOMUTEX,
		FullMutex = SQLITE_OPEN_FULLMUTEX,
		SharedCache = SQLITE_OPEN_SHAREDCACHE,
		PrivateCache = SQLITE_OPEN_PRIVATECACHE,
		NoFollow = SQLITE_OPEN_NOFOLLOW

	};

	inline constexpr auto operator| (const DatabaseOpenFlags lhs, const DatabaseOpenFlags rhs) -> DatabaseOpenFlags {
		using T = std::underlying_type_t<DatabaseOpenFlags>;
		return static_cast<DatabaseOpenFlags>(static_cast<T>(lhs) | static_cast<T>(rhs));
	}

	inline constexpr auto operator|= (DatabaseOpenFlags& lhs, const DatabaseOpenFlags rhs) -> DatabaseOpenFlags& {
		lhs = (lhs | rhs);
		return lhs;
	}

	inline constexpr auto operator& (const DatabaseOpenFlags lhs, const DatabaseOpenFlags rhs) -> DatabaseOpenFlags {
		using T = std::underlying_type_t<DatabaseOpenFlags>;
		return static_cast<DatabaseOpenFlags>(static_cast<T>(lhs) & static_cast<T>(rhs));
	}

	inline constexpr auto operator&= (DatabaseOpenFlags& lhs, const DatabaseOpenFlags rhs) -> DatabaseOpenFlags& {
		lhs = (lhs & rhs);
		return lhs;
	}

	inline constexpr auto operator^ (const DatabaseOpenFlags lhs, const DatabaseOpenFlags rhs) -> DatabaseOpenFlags {
		using T = std::underlying_type_t<DatabaseOpenFlags>;
		return static_cast<DatabaseOpenFlags>(static_cast<T>(lhs) ^ static_cast<T>(rhs));
	}

	inline constexpr auto operator^= (DatabaseOpenFlags& lhs, const DatabaseOpenFlags rhs) -> DatabaseOpenFlags& {
		lhs = (lhs ^ rhs);
		return lhs;
	}

	inline constexpr auto operator~ (const DatabaseOpenFlags rhs) -> DatabaseOpenFlags {
		using T = std::underlying_type_t<DatabaseOpenFlags>;
		return static_cast<DatabaseOpenFlags>(~static_cast<T>(rhs));
	}

	class Statement;

	class Database {

	public:
		Database(const std::optional<std::string_view> filename, const DatabaseOpenFlags flags);

		auto ConnectionHandle(void) const -> sqlite3*;

		auto PrepareStatement(const std::string_view sql) -> Statement;

		template <typename... Args>
		auto Execute(const std::string_view sql, const Args&... args) -> void;

	private:
		Handle<sqlite3*, nullptr> m_db;

	};

	inline Database::Database(const std::optional<std::string_view> filename, const DatabaseOpenFlags flags) {

		this->m_db = { nullptr, &sqlite3_close_v2 };

		const int res = sqlite3_open_v2(
			filename.value_or(":memory:").data(),
			this->m_db.GetAddressOf(),
			static_cast<int>(flags),
			nullptr
		);

		if (res != SQLITE_OK)
			throw SqliteException { this->m_db.Get() };

	}

	inline auto Database::ConnectionHandle() const -> sqlite3* {
		return this->m_db.Get();
	}

	template <typename T>
	struct Binding {

		static inline auto Bind(sqlite3_stmt* const pStmt, const int index, const T& arg) -> int {
			static_assert(false, "Binding specialization does not exist.");
		}

		static inline auto Column(sqlite3_stmt* const pStmt, const int column, T& arg) -> void {
			static_assert(false, "Binding specialization does not exist.");
		}

	};

#ifndef VSQLITE_NO_DEFAULT_BINDING_SPECIALIZATIONS

	template <>
	struct Binding<std::nullptr_t> {

		static inline auto Bind(sqlite3_stmt* const pStmt, const int index, const std::nullptr_t) -> int {
			return sqlite3_bind_null(pStmt, index);
		}

	};

	template <typename T>
	struct Binding<std::optional<T>> {

		static inline auto Bind(sqlite3_stmt* const pStmt, const int index, const std::optional<T>& arg) -> int {
			if (arg.has_value()) return Binding<T>::Bind(pStmt, index, arg.value());
			else return Binding<std::nullptr_t>::Bind(pStmt, index, nullptr);
		}

		static inline auto Column(sqlite3_stmt* const pStmt, const int column, std::optional<T>& arg) -> void {
			if (sqlite3_column_type(pStmt, column) == SQLITE_NULL) arg = std::nullopt;
			else Binding<T>::Column(pStmt, column, arg.emplace());
		}

	};

	template <>
	struct Binding<std::nullopt_t> {

		static inline auto Bind(sqlite3_stmt* const pStmt, const int index, const std::nullopt_t) -> int {
			return Binding<std::nullptr_t>::Bind(pStmt, index, nullptr);
		}

	};

	template <>
	struct Binding<const char*> {

		static inline auto Bind(sqlite3_stmt* const pStmt, const int index, const char* arg) -> int {
			return sqlite3_bind_text(pStmt, index, arg, -1, SQLITE_TRANSIENT);
		}

	};

	template <std::size_t Len>
	struct Binding<char[Len]> {

		static inline auto Bind(sqlite3_stmt* const pStmt, const int index, const char (&arg)[Len]) -> int {
			return sqlite3_bind_text(pStmt, index, arg, static_cast<int>(Len - 1), SQLITE_TRANSIENT);
		}

	};

	template <>
	struct Binding<std::string_view> {

		static inline auto Bind(sqlite3_stmt* const pStmt, const int index, const std::string_view arg) -> int {
			return Binding<const char*>::Bind(pStmt, index, arg.data());
		}

	};

	template <>
	struct Binding<std::string> {

		static inline auto Bind(sqlite3_stmt* const pStmt, const int index, const std::string& arg) -> int {
			return Binding<const char*>::Bind(pStmt, index, arg.c_str());
		}

		static inline auto Column(sqlite3_stmt* const pStmt, const int column, std::string& arg) -> void {
			const int len = sqlite3_column_bytes(pStmt, column);
			const unsigned char* pText = sqlite3_column_text(pStmt, column);
			arg = { reinterpret_cast<const char*>(pText), static_cast<std::size_t>(len) };
		}

	};

	template <std::integral T>
	struct Binding<T> {

		static inline auto Bind(sqlite3_stmt* const pStmt, const int index, const T arg) -> int {
			if constexpr (sizeof(T) < sizeof(std::int64_t)) return sqlite3_bind_int(pStmt, index, static_cast<int>(arg));
			else return sqlite3_bind_int64(pStmt, index, static_cast<sqlite3_int64>(arg));
		}

		static inline auto Column(sqlite3_stmt* const pStmt, const int column, T& arg) -> void {
			if constexpr (sizeof(T) < sizeof(std::int64_t)) arg = static_cast<T>(sqlite3_column_int(pStmt, column));
			else arg = static_cast<T>(sqlite3_column_int64(pStmt, column));
		}

	};

	template <std::floating_point T>
	struct Binding<T> {

		static inline auto Bind(sqlite3_stmt* const pStmt, const int index, const T arg) -> int {
			return sqlite3_bind_double(pStmt, index, static_cast<double>(arg));
		}

		static inline auto Column(sqlite3_stmt* const pStmt, const int column, T& arg) -> void {
			arg = static_cast<T>(sqlite3_column_double(pStmt, column));
		}

	};

	template <>
	struct Binding<bool> {

		static inline auto Bind(sqlite3_stmt* const pStmt, const int index, const bool arg) -> int {
			return Binding<std::int32_t>::Bind(pStmt, index, (arg ? 1 : 0));
		}

		static inline auto Column(sqlite3_stmt* const pStmt, const int column, bool& arg) -> void {
			std::int32_t val = 0;
			Binding<std::int32_t>::Column(pStmt, column, val);
			arg = (val != 0);
		}

	};

	template <>
	struct Binding<std::span<const std::uint8_t>> {

		static inline auto Bind(sqlite3_stmt* const pStmt, const int index, const std::span<const std::uint8_t> arg) -> int {
			return sqlite3_bind_blob64(pStmt, index, arg.data(), static_cast<sqlite3_uint64>(arg.size()), SQLITE_TRANSIENT);
		}

	};

	template <>
	struct Binding<std::span<std::uint8_t>> {

		static inline auto Bind(sqlite3_stmt* const pStmt, const int index, const std::span<std::uint8_t> arg) -> int {
			return Binding<std::span<const std::uint8_t>>::Bind(pStmt, index, arg);
		}

		static inline auto Column(sqlite3_stmt* const pStmt, const int column, std::span<std::uint8_t> arg) -> void {
			const int len = sqlite3_column_bytes(pStmt, column);
			const void* pData = sqlite3_column_blob(pStmt, column);
			std::memcpy(arg.data(), pData, std::min(static_cast<std::size_t>(len), arg.size()));
		}

	};

	template <>
	struct Binding<std::vector<std::uint8_t>> {

		static inline auto Bind(sqlite3_stmt* const pStmt, const int index, const std::vector<std::uint8_t>& arg) -> int {
			return Binding<std::span<const std::uint8_t>>::Bind(pStmt, index, arg);
		}

		static inline auto Column(sqlite3_stmt* const pStmt, const int column, std::vector<std::uint8_t>& arg) -> void {

			const int len = sqlite3_column_bytes(pStmt, column);
			arg.resize(len);

			Binding<std::span<std::uint8_t>>::Column(pStmt, column, arg);

		}

	};

#endif // VSQLITE_NO_DEFAULT_BINDING_SPECIALIZATIONS

	class Statement {

	public:
		Statement(const Database& db, const std::string_view sql);

		auto StatementHandle(void) const -> sqlite3_stmt*;

		auto Reset(void) -> void;
		auto Step(void) -> void;
		auto Unbind(void) -> void;
		
		template <int Index, typename T, typename... Args>
		auto Bind(const T& arg, const Args&... args) -> void;

		template <typename T, typename... Args>
		auto Bind(const T& arg, const Args&... args) -> void;

		template <int Index, typename T>
		auto Bind(const T& arg) -> void;

		template <typename T>
		auto Bind(const T& arg) -> void;

		template <int Column, typename T, typename... Args>
		auto Column(T& arg, Args&... args) -> void;

		template <typename T, typename... Args>
		auto Column(T& arg, Args&... args) -> void;

		template <int Column, typename T>
		auto Column(T& arg) -> void;

		template <typename T>
		auto Column(T& arg) -> void;

		auto Column(void) -> void;

		auto Execute(void) -> void;

		template <typename... Args>
		auto Execute(const Args&... args) -> void;

		template <typename... Args>
		auto Fetch(Args&... args) -> bool;

	private:
		Handle<sqlite3_stmt*, nullptr> m_stmt;
		bool m_canFetch;

	};

	inline auto Database::PrepareStatement(const std::string_view sql) -> Statement {
		return { static_cast<Database&>(*this), sql };
	}

	template <typename... Args>
	inline auto Database::Execute(const std::string_view sql, const Args&... args) -> void {
		this->PrepareStatement(sql).Execute(args...);
	}

	inline Statement::Statement(const Database& db, const std::string_view sql) {

		if (sql.empty())
			throw std::invalid_argument("'sql': Empty string.");

		this->m_stmt = { nullptr, &sqlite3_finalize };
		this->m_canFetch = false;

		const int res = sqlite3_prepare_v2(
			db.ConnectionHandle(),
			sql.data(),
			static_cast<int>(sql.size()),
			this->m_stmt.GetAddressOf(),
			nullptr
		);

		if (res != SQLITE_OK)
			throw SqliteException { db.ConnectionHandle() };

	}

	inline auto Statement::StatementHandle() const -> sqlite3_stmt* {
		return this->m_stmt.Get();
	}

	inline auto Statement::Reset() -> void {

		const int res = sqlite3_reset(this->StatementHandle());
		if (res != SQLITE_OK)
			throw SqliteException { this->StatementHandle() };

		this->m_canFetch = false;

	}

	inline auto Statement::Step() -> void {

		const int res = sqlite3_step(this->StatementHandle());
		if ((res != SQLITE_ROW) && (res != SQLITE_DONE))
			throw SqliteException { this->StatementHandle() };

		this->m_canFetch = (res == SQLITE_ROW);

	}

	inline auto Statement::Unbind() -> void {

		const int res = sqlite3_clear_bindings(this->StatementHandle());
		if (res != SQLITE_OK)
			throw SqliteException { this->StatementHandle() };

	}

	template <int Index, typename T, typename... Args>
	inline auto Statement::Bind(const T& arg, const Args&... args) -> void {
		this->Bind<Index>(arg);
		this->Bind<Index + 1>(args...);
	}

	template <typename T, typename... Args>
	inline auto Statement::Bind(const T& arg, const Args&... args) -> void {
		this->Bind<1>(arg, args...);
	}

	template <int Index, typename T>
	inline auto Statement::Bind(const T& arg) -> void {

		const int res = Binding<T>::Bind(this->StatementHandle(), Index, arg);
		if (res != SQLITE_OK)
			throw SqliteException { this->StatementHandle() };

	}

	template <typename T>
	inline auto Statement::Bind(const T& arg) -> void {
		this->Bind<1>(arg);
	}

	template <int Column, typename T, typename... Args>
	inline auto Statement::Column(T& arg, Args&... args) -> void {
		this->Column<Column>(arg);
		this->Column<Column + 1>(args...);
	}

	template <typename T, typename... Args>
	inline auto Statement::Column(T& arg, Args&... args) -> void {
		this->Column<0>(arg, args...);
	}

	template <int Column, typename T>
	inline auto Statement::Column(T& arg) -> void {
		Binding<T>::Column(this->StatementHandle(), Column, arg);
	}

	template <typename T>
	inline auto Statement::Column(T& arg) -> void {
		this->Column<0>(arg);
	}

	inline auto Statement::Column() -> void { }

	inline auto Statement::Execute() -> void {
		
		this->Reset();
		this->Unbind();
		this->Step();

	}

	template <typename... Args>
	inline auto Statement::Execute(const Args&... args) -> void {

		this->Reset();
		this->Unbind();
		this->Bind(args...);
		this->Step();

	}

	template <typename... Args>
	inline auto Statement::Fetch(Args&... args) -> bool {

		if (!this->m_canFetch)
			this->Step();

		if (this->m_canFetch) {
			this->Column(args...);
			this->m_canFetch = false;
			return true;
		}

		return false;
	}

}

#endif // __VSQLITE3_HPP__