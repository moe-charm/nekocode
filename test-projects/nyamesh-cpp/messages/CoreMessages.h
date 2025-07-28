/**
 * @file CoreMessages.h
 * @brief nyamesh_v22 最小限メッセージ構造定義
 * 
 * Phase B-3: コンパイル成功優先 - 必要最小限のみ実装
 * 「一個一個結合していく」理念実装
 * 
 * 段階的実装方針：
 * - 基本システムメッセージのみ（エラー・警告・情報・初期化）
 * - 複雑な構造体は後で段階的追加
 * - まずコンパイル成功、後で機能追加
 * 
 * @version 2.0 - Minimal First
 * @date 2025-07-25
 */

#pragma once

#include <string>
#include <chrono>

namespace nyamesh2::messages {

// ===============================================
// 基本システムメッセージ群（最小限）
// ===============================================

/**
 * @brief システムエラー通知（最小版）
 */
struct SystemError {
    std::string error;
    std::string core;
    
    SystemError() = default;
    SystemError(const std::string& err, const std::string& c)
        : error(err), core(c) {}
};

/**
 * @brief システム警告通知（最小版）
 */
struct SystemWarning {
    std::string warning;
    std::string core;
    
    SystemWarning() = default;
    SystemWarning(const std::string& warn, const std::string& c)
        : warning(warn), core(c) {}
};

/**
 * @brief システム情報通知（最小版）
 */
struct SystemInfo {
    std::string info;
    std::string core;
    
    SystemInfo() = default;
    SystemInfo(const std::string& i, const std::string& c)
        : info(i), core(c) {}
};

/**
 * @brief Core初期化完了通知（最小版）
 */
struct CoreInitialized {
    std::string coreName;
    std::string version;
    
    CoreInitialized() = default;
    CoreInitialized(const std::string& name, const std::string& ver)
        : coreName(name), version(ver) {}
};

// ===============================================
// 設定メッセージ群（最小版）
// ===============================================

/**
 * @brief 設定値構造体（最小版）
 */
struct SettingsValue {
    std::string path;
    std::string value;
    std::string type;
    bool exists = false;
    
    SettingsValue() = default;
    SettingsValue(const std::string& p, const std::string& v, const std::string& t)
        : path(p), value(v), type(t), exists(true) {}
        
    bool isValid() const { return exists && !path.empty(); }
};

/**
 * @brief 設定取得要求（最小版）
 */
struct SettingsGetRequest {
    std::string path;
    std::string requestId;
    
    SettingsGetRequest() = default;
    SettingsGetRequest(const std::string& p, const std::string& id)
        : path(p), requestId(id) {}
};

/**
 * @brief 設定取得応答（最小版）
 */
struct SettingsGetResponse {
    std::string requestId;
    SettingsValue value;
    bool success = false;
    std::string errorMessage;
    
    SettingsGetResponse() = default;
};

/**
 * @brief 設定変更要求（最小版）
 */
struct SettingsSetRequest {
    SettingsValue newValue;
    std::string requestId;
    
    SettingsSetRequest() = default;
    SettingsSetRequest(const SettingsValue& val, const std::string& id)
        : newValue(val), requestId(id) {}
};

/**
 * @brief 設定変更応答（最小版）
 */
struct SettingsSetResponse {
    std::string requestId;
    bool success = false;
    std::string errorMessage;
    
    SettingsSetResponse() = default;
};

/**
 * @brief 設定変更通知（最小版）
 */
struct SettingsChanged {
    std::string path;
    std::string oldValue;
    std::string newValue;
    std::string type;
    
    SettingsChanged() = default;
    SettingsChanged(const std::string& p, const std::string& ov, const std::string& nv, const std::string& t)
        : path(p), oldValue(ov), newValue(nv), type(t) {}
};

// ===============================================
// ファイルシステムメッセージ群（最小版）
// ===============================================

/**
 * @brief ドライブ一覧要求（最小版）
 */
struct DrivesRequest {
    std::string requestId;
    
    DrivesRequest() = default;
    DrivesRequest(const std::string& id) : requestId(id) {}
};

/**
 * @brief ドライブ一覧応答（最小版）
 */
struct DrivesResponse {
    std::string requestId;
    std::string drivesData;  // JSON文字列形式
    bool success = false;
    std::string errorMessage;
    
    DrivesResponse() = default;
};

/**
 * @brief ディレクトリ内容要求（最小版）
 */
struct DirectoryRequest {
    std::string path;
    std::string requestId;
    
    DirectoryRequest() = default;
    DirectoryRequest(const std::string& p, const std::string& id)
        : path(p), requestId(id) {}
};

/**
 * @brief ディレクトリ内容応答（最小版）
 */
struct DirectoryResponse {
    std::string requestId;
    std::string path;
    std::string entriesData;  // JSON文字列形式
    int count = 0;
    bool success = false;
    std::string errorMessage;
    
    DirectoryResponse() = default;
};

/**
 * @brief ファイル読み込み要求（最小版）
 */
struct FileReadRequest {
    std::string path;
    std::string requestId;
    
    FileReadRequest() = default;
    FileReadRequest(const std::string& p, const std::string& id)
        : path(p), requestId(id) {}
};

/**
 * @brief ファイル読み込み応答（最小版）
 */
struct FileReadResponse {
    std::string requestId;
    std::string path;
    std::string content;
    int64_t size = 0;
    bool success = false;
    std::string errorMessage;
    
    FileReadResponse() = default;
};

/**
 * @brief ファイル書き込み要求（最小版）
 */
struct FileWriteRequest {
    std::string path;
    std::string content;
    std::string requestId;
    
    FileWriteRequest() = default;
    FileWriteRequest(const std::string& p, const std::string& c, const std::string& id)
        : path(p), content(c), requestId(id) {}
};

/**
 * @brief ファイル書き込み応答（最小版）
 */
struct FileWriteResponse {
    std::string requestId;
    std::string path;
    int64_t size = 0;
    bool success = false;
    std::string errorMessage;
    
    FileWriteResponse() = default;
};

// ===============================================
// エディターメッセージ群（最小版）
// ===============================================

/**
 * @brief エディター要求（最小版）
 */
struct EditorRequest {
    std::string requestType;
    std::string bufferId;
    
    EditorRequest() = default;
    EditorRequest(const std::string& type, const std::string& id)
        : requestType(type), bufferId(id) {}
};

/**
 * @brief バッファー作成要求（最小版）
 */
struct BufferCreateRequest {
    std::string filePath;
    std::string bufferId;
    
    BufferCreateRequest() = default;
    BufferCreateRequest(const std::string& path, const std::string& id)
        : filePath(path), bufferId(id) {}
};

/**
 * @brief テキスト変更要求（最小版）
 */
struct TextChangeRequest {
    std::string bufferId;
    std::string change;
    int position = 0;
    
    TextChangeRequest() = default;
    TextChangeRequest(const std::string& id, const std::string& ch, int pos)
        : bufferId(id), change(ch), position(pos) {}
};

} // namespace nyamesh2::messages

// ===============================================
// メッセージタイプ定数（最小版）
// ===============================================

namespace nyamesh2::message_types {
    constexpr const char* SYSTEM_ERROR = "system.error";
    constexpr const char* SYSTEM_WARNING = "system.warning";  
    constexpr const char* SYSTEM_INFO = "system.info";
    constexpr const char* CORE_INITIALIZED = "core.initialized";
    
    constexpr const char* SETTINGS_GET_REQUEST = "settings.get.request";
    constexpr const char* SETTINGS_GET_RESPONSE = "settings.get.response";
    constexpr const char* SETTINGS_SET_REQUEST = "settings.set.request";
    constexpr const char* SETTINGS_SET_RESPONSE = "settings.set.response";
    constexpr const char* SETTINGS_CHANGED = "settings.changed";
    
    constexpr const char* FILESYSTEM_DRIVES_REQUEST = "filesystem.drives.request";
    constexpr const char* FILESYSTEM_DRIVES_RESPONSE = "filesystem.drives.response";
    constexpr const char* FILESYSTEM_DIRECTORY_REQUEST = "filesystem.directory.request";
    constexpr const char* FILESYSTEM_DIRECTORY_RESPONSE = "filesystem.directory.response";
    constexpr const char* FILESYSTEM_FILE_READ_REQUEST = "filesystem.file.read.request";
    constexpr const char* FILESYSTEM_FILE_READ_RESPONSE = "filesystem.file.read.response";
    constexpr const char* FILESYSTEM_FILE_WRITE_REQUEST = "filesystem.file.write.request";
    constexpr const char* FILESYSTEM_FILE_WRITE_RESPONSE = "filesystem.file.write.response";
    
    constexpr const char* EDITOR_REQUEST = "editor.request";
    constexpr const char* BUFFER_CREATE_REQUEST = "buffer.create.request";
    constexpr const char* TEXT_CHANGE_REQUEST = "text.change.request";
}