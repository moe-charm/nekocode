/**
 * @file IntentDefinitions_v22.h
 * @brief P2P Intent定義 - 全Core共通Intent名定義
 * 
 * Intent命名規則: <domain>.<entity>.<action>
 * 例: settings.get.request, editor.buffer.create
 * 
 * @version 2.0
 * @date 2025-07-25
 */

#pragma once

#include <string>

namespace charmcode {

// ===============================================
// System Intents - 全Core共通
// ===============================================

namespace SystemIntents {
    // Core lifecycle
    constexpr const char* CORE_INITIALIZED = "core.initialized";
    constexpr const char* CORE_SHUTDOWN = "core.shutdown";
    constexpr const char* SYSTEM_ERROR = "system.error";
    constexpr const char* SYSTEM_STATISTICS = "system.statistics";
}

// ===============================================
// Settings Management Intents
// ===============================================

namespace SettingsIntents {
    // Request/Response pairs
    constexpr const char* GET_REQUEST = "settings.get.request";
    constexpr const char* GET_RESPONSE = "settings.get.response";
    
    constexpr const char* SET_REQUEST = "settings.set.request";
    constexpr const char* SET_RESPONSE = "settings.set.response";
    
    constexpr const char* SAVE_REQUEST = "settings.save.request";
    constexpr const char* SAVE_RESPONSE = "settings.save.response";
    
    constexpr const char* RELOAD_REQUEST = "settings.reload.request";
    constexpr const char* RELOAD_RESPONSE = "settings.reload.response";
    
    // Notifications (broadcast)
    constexpr const char* CHANGED = "settings.changed";
}

// ===============================================
// Editor Operation Intents
// ===============================================

namespace EditorIntents {
    // Buffer management
    constexpr const char* BUFFER_CREATE = "editor.buffer.create";
    constexpr const char* BUFFER_CREATED = "editor.buffer.created";
    
    constexpr const char* BUFFER_CLOSE = "editor.buffer.close";
    constexpr const char* BUFFER_CLOSED = "editor.buffer.closed";
    
    constexpr const char* BUFFER_SET_ACTIVE = "editor.buffer.setActive";
    constexpr const char* BUFFER_ACTIVATED = "editor.buffer.activated";
    
    // Content operations
    constexpr const char* CONTENT_SET = "editor.content.set";
    constexpr const char* CONTENT_UPDATED = "editor.content.updated";
    
    constexpr const char* CONTENT_GET = "editor.content.get";
    constexpr const char* CONTENT_RESPONSE = "editor.content.response";
    
    // Text operations
    constexpr const char* TEXT_INSERT = "editor.text.insert";
    constexpr const char* TEXT_INSERTED = "editor.text.inserted";
    
    constexpr const char* TEXT_DELETE = "editor.text.delete";
    constexpr const char* TEXT_DELETED = "editor.text.deleted";
    
    constexpr const char* TEXT_CHANGED = "editor.text.changed";
    
    // Zoom operations
    constexpr const char* ZOOM_SET = "editor.zoom.set";
    constexpr const char* ZOOM_UPDATED = "editor.zoom.updated";
    
    // Settings
    constexpr const char* SETTINGS_APPLY = "editor.settings.apply";
    constexpr const char* SETTINGS_APPLIED = "editor.settings.applied";
    
    // Cursor and selection
    constexpr const char* CURSOR_MOVED = "editor.cursor.moved";
    
    // Statistics
    constexpr const char* STATISTICS_UPDATE = "editor.statistics.update";
}

// ===============================================
// FileSystem Operation Intents
// ===============================================

namespace FileSystemIntents {
    // Drive operations
    constexpr const char* DRIVES_REQUEST = "filesystem.drives.request";
    constexpr const char* DRIVES_RESPONSE = "filesystem.drives.response";
    
    // Directory operations
    constexpr const char* DIRECTORY_LIST = "filesystem.directory.list";
    constexpr const char* DIRECTORY_RESPONSE = "filesystem.directory.response";
    
    constexpr const char* DIRECTORY_CREATE = "filesystem.directory.create";
    constexpr const char* DIRECTORY_CREATED = "filesystem.directory.created";
    
    constexpr const char* DIRECTORY_DELETE = "filesystem.directory.delete";
    constexpr const char* DIRECTORY_DELETED = "filesystem.directory.deleted";
    
    // File operations
    constexpr const char* FILE_READ = "filesystem.file.read";
    constexpr const char* FILE_CONTENT = "filesystem.file.content";
    
    constexpr const char* FILE_WRITE = "filesystem.file.write";
    constexpr const char* FILE_WRITTEN = "filesystem.file.written";
    
    constexpr const char* FILE_DELETE = "filesystem.file.delete";
    constexpr const char* FILE_DELETED = "filesystem.file.deleted";
    
    // Path operations
    constexpr const char* PATH_INFO = "filesystem.path.info";
    constexpr const char* PATH_RESPONSE = "filesystem.path.response";
    
    // Watch operations
    constexpr const char* WATCH_START = "filesystem.watch.start";
    constexpr const char* WATCH_STARTED = "filesystem.watch.started";
    
    constexpr const char* WATCH_STOP = "filesystem.watch.stop";
    constexpr const char* WATCH_STOPPED = "filesystem.watch.stopped";
    
    constexpr const char* WATCH_EVENT = "filesystem.watch.event";
    
    // Error notification
    constexpr const char* ERROR = "filesystem.error";
}

// ===============================================
// QuickAccess Intents (Future - Phase 3)
// ===============================================

namespace QuickAccessIntents {
    constexpr const char* BOOKMARKS_LIST = "quickaccess.bookmarks.list";
    constexpr const char* BOOKMARKS_RESPONSE = "quickaccess.bookmarks.response";
    
    constexpr const char* BOOKMARK_ADD = "quickaccess.bookmark.add";
    constexpr const char* BOOKMARK_REMOVE = "quickaccess.bookmark.remove";
    
    constexpr const char* RECENT_LIST = "quickaccess.recent.list";
    constexpr const char* RECENT_RESPONSE = "quickaccess.recent.response";
    
    constexpr const char* SHORTCUT_CREATE = "quickaccess.shortcut.create";
    constexpr const char* CHANGED = "quickaccess.changed";
}

// ===============================================
// UI Coordination Intents (Future - Phase 4)
// ===============================================

namespace UIIntents {
    constexpr const char* LAYOUT_UPDATE = "ui.layout.update";
    constexpr const char* PANEL_SHOW = "ui.panel.show";
    constexpr const char* DIALOG_OPEN = "ui.dialog.open";
    constexpr const char* MENU_UPDATE = "ui.menu.update";
    constexpr const char* STATUSBAR_UPDATE = "ui.statusbar.update";
    
    constexpr const char* ACTION_PREFIX = "ui.action.";
    constexpr const char* STATE_CHANGED = "ui.state.changed";
}

// ===============================================
// Network Operation Intents (Future - Phase 5)
// ===============================================

namespace NetworkIntents {
    constexpr const char* SERVER_CONNECT = "network.server.connect";
    constexpr const char* CONNECTED = "network.connected";
    
    constexpr const char* SERVER_DISCONNECT = "network.server.disconnect";
    constexpr const char* DISCONNECTED = "network.disconnected";
    
    constexpr const char* DIRECTORY_LIST = "network.directory.list";
    constexpr const char* DIRECTORY_RESPONSE = "network.directory.response";
    
    constexpr const char* FILE_DOWNLOAD = "network.file.download";
    constexpr const char* FILE_UPLOAD = "network.file.upload";
    
    constexpr const char* TRANSFER_PROGRESS = "network.transfer.progress";
    constexpr const char* ERROR = "network.error";
}

} // namespace charmcode