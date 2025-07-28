/**
 * @file IEditorCore.h
 * @brief エディターCore Interface - 完全JSON排除版
 * 
 * 革命的設計変更：
 * - nlohmann::json完全排除（Gemini先生指摘対応）
 * - 明示的構造体使用（ChatGPT先生提案）
 * - Qt依存を.cpp実装に隠蔽
 * - ZoomableTextEdit統合対応
 * - Phase C（C++20モジュール）完全対応
 * 
 * 対比：
 * 旧版: nlohmann::json getBuffer() ← JSON依存・Qt露出
 * 新版: EditorBuffer getBuffer() ← 型安全・Qt隠蔽
 * 
 * @version 2.0 - JSON Independence Revolution
 * @date 2025-07-25
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

// ✅ 軽量メッセージ構造体のみインクルード（JSON不要）
#include "../messages/CoreMessages.h"

namespace charmcode {

/**
 * @brief エディターバッファ構造体（完全型安全版）
 * 
 * 旧版: nlohmann::json（動的・Qt依存）
 * 新版: EditorBuffer（静的・Qt隠蔽）
 */
struct EditorBuffer {
    std::string bufferId;      // バッファ識別ID
    std::string filePath;      // ファイルパス（空文字列で無題）
    std::string content;       // テキスト内容
    std::string language;      // 言語種別（"cpp", "python", "txt"等）
    bool modified = false;     // 変更フラグ
    bool readOnly = false;     // 読み取り専用フラグ
    size_t cursorPosition = 0; // カーソル位置（文字オフセット）
    int cursorLine = 0;        // カーソル行（0ベース）
    int cursorColumn = 0;      // カーソル列（0ベース）
    
    EditorBuffer() = default;
    EditorBuffer(const std::string& id, const std::string& path = "")
        : bufferId(id), filePath(path) {}
    
    /**
     * @brief 有効性確認
     */
    bool isValid() const {
        return !bufferId.empty();
    }
    
    /**
     * @brief ファイル関連付け確認
     */
    bool hasFile() const {
        return !filePath.empty();
    }
    
    /**
     * @brief 行カウント取得
     */
    int getLineCount() const {
        if (content.empty()) return 1;
        return static_cast<int>(std::count(content.begin(), content.end(), '\n')) + 1;
    }
};

/**
 * @brief カーソル位置情報
 */
struct CursorPosition {
    std::string bufferId;   // バッファID
    int line = 0;          // 行番号（0ベース）
    int column = 0;        // 列番号（0ベース）
    size_t offset = 0;     // 文字オフセット
    
    CursorPosition() = default;
    CursorPosition(const std::string& id, int l, int c, size_t off = 0)
        : bufferId(id), line(l), column(c), offset(off) {}
    
    bool isValid() const {
        return !bufferId.empty() && line >= 0 && column >= 0;
    }
};

/**
 * @brief テキスト選択範囲
 */
struct TextSelection {
    std::string bufferId;     // バッファID
    CursorPosition start;     // 選択開始位置
    CursorPosition end;       // 選択終了位置
    std::string selectedText; // 選択されたテキスト
    
    TextSelection() = default;
    TextSelection(const std::string& id) : bufferId(id) {}
    
    bool hasSelection() const {
        return !selectedText.empty();
    }
    
    bool isValid() const {
        return !bufferId.empty() && start.isValid() && end.isValid();
    }
};

/**
 * @brief テキスト変更情報
 */
struct TextChange {
    std::string bufferId;     // バッファID
    std::string changeType;   // "insert", "delete", "replace"
    CursorPosition position; // 変更位置
    std::string oldText;     // 旧テキスト
    std::string newText;     // 新テキスト
    uint64_t timestamp = 0;  // 変更時刻
    
    TextChange() = default;
    TextChange(const std::string& id, const std::string& type)
        : bufferId(id), changeType(type) {}
};

/**
 * @brief エディター設定
 */
struct EditorSettings {
    int fontSize = 12;           // フォントサイズ
    std::string fontFamily = "Consolas"; // フォント名
    bool lineNumbers = true;     // 行番号表示
    bool wordWrap = false;       // 行の折り返し
    int tabSize = 4;            // タブサイズ
    bool insertSpaces = true;    // スペース挿入
    bool autoIndent = true;      // 自動インデント
    bool syntaxHighlight = true; // シンタックスハイライト
    std::string theme = "default"; // テーマ名
    double zoomFactor = 1.0;     // ズーム倍率
    
    EditorSettings() = default;
};

/**
 * @brief 検索結果
 */
struct SearchResult {
    std::string bufferId;           // バッファID
    std::vector<CursorPosition> matches; // 検索結果位置一覧
    int totalMatches = 0;           // 総マッチ数
    int currentMatch = -1;          // 現在のマッチ位置
    std::string searchQuery;        // 検索クエリ
    bool caseSensitive = false;     // 大文字小文字区別
    bool useRegex = false;         // 正規表現使用
    
    SearchResult() = default;
    explicit SearchResult(const std::string& id) : bufferId(id) {}
    
    bool hasMatches() const {
        return totalMatches > 0;
    }
};

/**
 * @brief 純粋抽象エディターインターフェース
 * 
 * 革命的特徴：
 * - JSON完全排除・型安全保証
 * - Qt依存を.cpp実装に完全隠蔽
 * - ZoomableTextEdit統合対応
 * - 非同期・同期両対応
 * - Interface+Pimpl完全独立性
 * - C++20モジュール完全対応
 * 
 * 使用例：
 * ```cpp
 * auto editor = createEditorCore();
 * auto bufferId = editor->createBuffer("main.cpp");
 * editor->setBufferContent(bufferId, "#include <iostream>");
 * auto buffer = editor->getBuffer(bufferId);
 * // 型安全・IDE補完完璧
 * ```
 */
class IEditorCore {
public:
    virtual ~IEditorCore() = default;
    
    // ===============================================
    // 📝 バッファ管理
    // ===============================================
    
    /**
     * @brief バッファ作成
     * @param filePath ファイルパス（空文字列で無題）
     * @return バッファID
     */
    virtual std::string createBuffer(const std::string& filePath = "") = 0;
    
    /**
     * @brief バッファ削除
     * @param bufferId バッファID
     * @return 成功時true
     */
    virtual bool closeBuffer(const std::string& bufferId) = 0;
    
    /**
     * @brief バッファ取得
     * @param bufferId バッファID
     * @return バッファ情報
     */
    virtual EditorBuffer getBuffer(const std::string& bufferId) const = 0;
    
    /**
     * @brief 全バッファ取得
     * @return バッファ一覧
     */
    virtual std::vector<EditorBuffer> getAllBuffers() const = 0;
    
    /**
     * @brief アクティブバッファ設定
     * @param bufferId バッファID
     * @return 成功時true
     */
    virtual bool setActiveBuffer(const std::string& bufferId) = 0;
    
    /**
     * @brief アクティブバッファ取得
     * @return アクティブバッファID
     */
    virtual std::string getActiveBufferId() const = 0;
    
    /**
     * @brief バッファ存在確認
     * @param bufferId バッファID
     * @return 存在時true
     */
    virtual bool hasBuffer(const std::string& bufferId) const = 0;
    
    // ===============================================
    // ✏️ テキスト編集
    // ===============================================
    
    /**
     * @brief バッファ内容設定
     * @param bufferId バッファID
     * @param content テキスト内容
     * @return 成功時true
     */
    virtual bool setBufferContent(const std::string& bufferId, const std::string& content) = 0;
    
    /**
     * @brief バッファ内容取得
     * @param bufferId バッファID
     * @return テキスト内容
     */
    virtual std::string getBufferContent(const std::string& bufferId) const = 0;
    
    /**
     * @brief テキスト挿入
     * @param bufferId バッファID
     * @param position 挿入位置
     * @param text 挿入テキスト
     * @return 成功時true
     */
    virtual bool insertText(const std::string& bufferId, const CursorPosition& position, const std::string& text) = 0;
    
    /**
     * @brief テキスト削除
     * @param bufferId バッファID
     * @param start 削除開始位置
     * @param end 削除終了位置
     * @return 削除されたテキスト
     */
    virtual std::string deleteText(const std::string& bufferId, const CursorPosition& start, const CursorPosition& end) = 0;
    
    /**
     * @brief テキスト置換
     * @param bufferId バッファID
     * @param start 置換開始位置
     * @param end 置換終了位置
     * @param newText 新しいテキスト
     * @return 成功時true
     */
    virtual bool replaceText(const std::string& bufferId, const CursorPosition& start, const CursorPosition& end, const std::string& newText) = 0;
    
    // ===============================================
    // 🎯 カーソル・選択操作
    // ===============================================
    
    /**
     * @brief カーソル位置設定
     * @param bufferId バッファID
     * @param position カーソル位置
     * @return 成功時true
     */
    virtual bool setCursorPosition(const std::string& bufferId, const CursorPosition& position) = 0;
    
    /**
     * @brief カーソル位置取得
     * @param bufferId バッファID
     * @return カーソル位置
     */
    virtual CursorPosition getCursorPosition(const std::string& bufferId) const = 0;
    
    /**
     * @brief テキスト選択設定
     * @param bufferId バッファID
     * @param selection 選択範囲
     * @return 成功時true
     */
    virtual bool setSelection(const std::string& bufferId, const TextSelection& selection) = 0;
    
    /**
     * @brief テキスト選択取得
     * @param bufferId バッファID
     * @return 選択範囲
     */
    virtual TextSelection getSelection(const std::string& bufferId) const = 0;
    
    /**
     * @brief 選択クリア
     * @param bufferId バッファID
     * @return 成功時true
     */
    virtual bool clearSelection(const std::string& bufferId) = 0;
    
    // ===============================================
    // 🔍 検索・置換
    // ===============================================
    
    /**
     * @brief テキスト検索
     * @param bufferId バッファID
     * @param query 検索クエリ
     * @param caseSensitive 大文字小文字区別
     * @param useRegex 正規表現使用
     * @return 検索結果
     */
    virtual SearchResult searchText(const std::string& bufferId, const std::string& query, bool caseSensitive = false, bool useRegex = false) const = 0;
    
    /**
     * @brief 次の検索結果に移動
     * @param bufferId バッファID
     * @return 移動後の位置
     */
    virtual CursorPosition findNext(const std::string& bufferId) = 0;
    
    /**
     * @brief 前の検索結果に移動
     * @param bufferId バッファID
     * @return 移動後の位置
     */
    virtual CursorPosition findPrevious(const std::string& bufferId) = 0;
    
    /**
     * @brief 全置換
     * @param bufferId バッファID
     * @param searchQuery 検索クエリ
     * @param replaceText 置換テキスト
     * @param caseSensitive 大文字小文字区別
     * @param useRegex 正規表現使用
     * @return 置換回数
     */
    virtual int replaceAll(const std::string& bufferId, const std::string& searchQuery, const std::string& replaceText, bool caseSensitive = false, bool useRegex = false) = 0;
    
    // ===============================================
    // 📁 ファイル操作
    // ===============================================
    
    /**
     * @brief ファイル読み込み
     * @param filePath ファイルパス
     * @return バッファID（失敗時は空文字列）
     */
    virtual std::string loadFile(const std::string& filePath) = 0;
    
    /**
     * @brief ファイル保存
     * @param bufferId バッファID
     * @param filePath ファイルパス（空文字列で既存パス使用）
     * @return 成功時true
     */
    virtual bool saveFile(const std::string& bufferId, const std::string& filePath = "") = 0;
    
    /**
     * @brief 全ファイル保存
     * @return 保存成功したファイル数
     */
    virtual int saveAllFiles() = 0;
    
    /**
     * @brief 変更確認
     * @param bufferId バッファID
     * @return 変更ありの場合true
     */
    virtual bool isModified(const std::string& bufferId) const = 0;
    
    /**
     * @brief 変更フラグクリア
     * @param bufferId バッファID
     * @return 成功時true
     */
    virtual bool clearModified(const std::string& bufferId) = 0;
    
    // ===============================================
    // ⚙️ エディター設定
    // ===============================================
    
    /**
     * @brief エディター設定取得
     * @return 現在の設定
     */
    virtual EditorSettings getSettings() const = 0;
    
    /**
     * @brief エディター設定更新
     * @param settings 新しい設定
     * @return 成功時true
     */
    virtual bool updateSettings(const EditorSettings& settings) = 0;
    
    /**
     * @brief ズーム倍率設定
     * @param factor ズーム倍率（1.0が100%）
     * @return 成功時true
     */
    virtual bool setZoomFactor(double factor) = 0;
    
    /**
     * @brief ズーム倍率取得
     * @return 現在のズーム倍率
     */
    virtual double getZoomFactor() const = 0;
    
    // ===============================================
    // 📡 変更監視・通知システム
    // ===============================================
    
    /**
     * @brief テキスト変更監視
     * @param bufferId バッファID
     * @param callback 変更通知コールバック
     * @return 監視ID
     */
    virtual std::string watchTextChanges(const std::string& bufferId,
                                        std::function<void(const TextChange&)> callback) = 0;
    
    /**
     * @brief カーソル変更監視
     * @param bufferId バッファID
     * @param callback 変更通知コールバック
     * @return 監視ID
     */
    virtual std::string watchCursorChanges(const std::string& bufferId,
                                          std::function<void(const CursorPosition&)> callback) = 0;
    
    /**
     * @brief 監視解除
     * @param watchId 監視ID
     * @return 成功時true
     */
    virtual bool unwatchChanges(const std::string& watchId) = 0;
    
    // ===============================================
    // 📊 統計・メタ情報
    // ===============================================
    
    /**
     * @brief エディター統計情報
     */
    struct EditorStatistics {
        int totalBuffers = 0;
        int modifiedBuffers = 0;
        size_t totalCharacters = 0;
        size_t totalLines = 0;
        std::string activeBuffer;
        double currentZoom = 1.0;
    };
    
    /**
     * @brief 統計情報取得
     * @return 統計情報
     */
    virtual EditorStatistics getStatistics() const = 0;
    
    /**
     * @brief Core情報取得
     * @return Core情報文字列
     */
    virtual std::string getCoreInfo() const = 0;
    
    // ===============================================
    // ⚡ 非同期API
    // ===============================================
    
    /**
     * @brief 非同期ファイル読み込み
     * @param filePath ファイルパス
     * @param callback 完了コールバック
     */
    virtual void loadFileAsync(const std::string& filePath,
                              std::function<void(const std::string& bufferId, bool success)> callback) = 0;
    
    /**
     * @brief 非同期ファイル保存
     * @param bufferId バッファID
     * @param filePath ファイルパス
     * @param callback 完了コールバック
     */
    virtual void saveFileAsync(const std::string& bufferId, const std::string& filePath,
                              std::function<void(bool success)> callback) = 0;
};

// ===============================================
// 🏭 Factory関数（Phase C準備）
// ===============================================

/**
 * @brief EditorCore作成
 * 
 * 実装詳細を隠蔽したFactory関数
 * Phase C（C++20モジュール）移行時も同じインターフェース
 * 
 * @return EditorCoreインスタンス
 */
std::unique_ptr<IEditorCore> createEditorCore();

/**
 * @brief 設定付きEditorCore作成
 * @param settings 初期設定
 * @return EditorCoreインスタンス
 */
std::unique_ptr<IEditorCore> createEditorCoreWithSettings(const EditorSettings& settings);

/**
 * @brief EditorCore設定
 */
struct EditorCoreConfig {
    EditorSettings defaultSettings;
    bool enableZoom = true;
    bool enableSyntaxHighlight = true;
    bool enableAutoSave = true;
    int autoSaveIntervalMs = 30000;
    size_t maxBuffers = 50;
    size_t maxBufferSize = 10 * 1024 * 1024; // 10MB
};

/**
 * @brief 設定付きEditorCore作成
 * @param config Core設定
 * @return EditorCoreインスタンス
 */
std::unique_ptr<IEditorCore> createEditorCoreWithConfig(const EditorCoreConfig& config);

} // namespace charmcode

/**
 * @brief Phase C（C++20モジュール）対応準備
 */
#ifdef __cpp_modules
// export module nyamesh.interfaces.editor;
// export import nyamesh.messages;
// export namespace charmcode { class IEditorCore; }
#endif