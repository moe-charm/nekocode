/**
 * @file IFileSystemCore.h
 * @brief FileSystemCore Interface - ファイルシステム操作API定義
 * 
 * nyamesh_v22革命的特徴:
 * - Qt完全隠蔽: ヘッダーにQt依存なし
 * - 純粋C++型: std::string, int64_t使用
 * - Interface+Pimpl: 実装完全分離
 * - JSON-free: バイナリメッセージング準拠
 * 
 * @version 2.0 - Pure Implementation
 * @date 2025-07-25
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace charmcode {

/**
 * @brief ドライブ情報構造体（純粋C++版）
 */
struct DriveInfo_v22 {
    std::string drivePath;       // C:/, D:/, etc.
    std::string displayName;     // Local Disk (C:)
    std::string volumeName;      // Volume label
    std::string driveIcon;       // 💾, 🔌, 🌐, 💽
    std::string devicePath;      // Device path
    std::string fileSystemType;  // NTFS, FAT32, etc.
    int64_t totalBytes = 0;
    int64_t availableBytes = 0;
    std::string formattedSize;   // "500 GB", "1.2 TB"
    bool isValid = false;
    bool isReady = false;
    bool isRemovable = false;
    std::string driveType;       // Fixed, Removable, Network, CD-ROM
    
    DriveInfo_v22() = default;
};

/**
 * @brief ディレクトリエントリ情報（純粋C++版）
 */
struct DirectoryEntry_v22 {
    std::string name;
    std::string path;
    std::string absolutePath;
    bool isDirectory = false;
    int64_t size = 0;
    std::string formattedSize;    // "1.2 MB", "500 KB"
    std::string lastModified;     // ISO format
    std::string fileType;         // extension or "folder"
    std::string displayIcon;      // 📁, 📄, etc.
    bool isHidden = false;
    bool isReadOnly = false;
    bool isSymLink = false;
    
    DirectoryEntry_v22() = default;
};

/**
 * @brief ファイル統計情報（純粋C++版）
 */
struct FileSystemStatistics_v22 {
    int totalDrives = 0;
    int totalDirectoriesScanned = 0;
    int totalFilesRead = 0;
    int totalFilesWritten = 0;
    int64_t totalBytesRead = 0;
    int64_t totalBytesWritten = 0;
    std::string lastAccessedPath;
    
    FileSystemStatistics_v22() = default;
};

/**
 * @brief ファイルシステムCore インターフェース
 * 
 * 実装詳細を完全隠蔽:
 * - Qt依存処理はすべて.cppに隠蔽
 * - Win32 API直接呼び出しも隠蔽
 * - 複雑なファイルI/O処理隠蔽
 */
class IFileSystemCore {
public:
    virtual ~IFileSystemCore() = default;
    
    // === Core管理 ===
    
    /**
     * @brief Core開始
     */
    virtual void startProcessing() = 0;
    
    /**
     * @brief Core停止
     */
    virtual void stopProcessing() = 0;
    
    /**
     * @brief 統計情報取得
     */
    virtual FileSystemStatistics_v22 getStatistics() const = 0;
    
    // === ドライブ操作 ===
    
    /**
     * @brief システムドライブ一覧取得
     */
    virtual std::vector<DriveInfo_v22> getDrives() = 0;
    
    /**
     * @brief ドライブ情報更新
     */
    virtual void refreshDrives() = 0;
    
    // === ディレクトリ操作 ===
    
    /**
     * @brief ディレクトリ内容取得
     */
    virtual std::vector<DirectoryEntry_v22> getDirectoryContents(const std::string& path) = 0;
    
    /**
     * @brief ディレクトリ作成
     */
    virtual bool createDirectory(const std::string& path) = 0;
    
    /**
     * @brief ディレクトリ削除
     */
    virtual bool removeDirectory(const std::string& path) = 0;
    
    // === ファイル操作 ===
    
    /**
     * @brief ファイル読み込み
     */
    virtual std::string readFile(const std::string& path) = 0;
    
    /**
     * @brief ファイル書き込み
     */
    virtual bool writeFile(const std::string& path, const std::string& content) = 0;
    
    /**
     * @brief ファイル削除
     */
    virtual bool removeFile(const std::string& path) = 0;
    
    /**
     * @brief ファイル情報取得
     */
    virtual DirectoryEntry_v22 getFileInfo(const std::string& path) = 0;
    
    /**
     * @brief ファイル存在確認
     */
    virtual bool fileExists(const std::string& path) = 0;
    
    // === パス操作 ===
    
    /**
     * @brief パス正規化
     */
    virtual std::string normalizePath(const std::string& path) = 0;
    
    /**
     * @brief 親ディレクトリ取得
     */
    virtual std::string getParentDirectory(const std::string& path) = 0;
    
    /**
     * @brief ファイル名取得
     */
    virtual std::string getFileName(const std::string& path) = 0;
    
    /**
     * @brief 拡張子取得
     */
    virtual std::string getFileExtension(const std::string& path) = 0;
};

/**
 * @brief FileSystemCore_v22 Factory関数
 */
class FileSystemCoreFactory_v22 {
public:
    /**
     * @brief 標準FileSystemCore作成
     */
    static std::unique_ptr<IFileSystemCore> create();
    
    /**
     * @brief 軽量FileSystemCore作成（メモリ使用量最小化）
     */
    static std::unique_ptr<IFileSystemCore> createLight();
    
    /**
     * @brief カスタム設定FileSystemCore作成
     */
    static std::unique_ptr<IFileSystemCore> createCustom(
        bool enableCache = true,
        int cacheTimeoutMs = 5000,
        bool enableHiddenFiles = false
    );
};

} // namespace charmcode