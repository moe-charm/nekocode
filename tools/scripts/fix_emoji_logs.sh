#!/bin/bash

# TypeScript・JavaScriptアナライザーの絵文字ログを一括でg_quiet_mode条件付きに修正

echo "🔧 TypeScript・JavaScriptアナライザーの絵文字ログ修正開始..."

# TypeScriptアナライザー修正
echo "📝 TypeScriptアナライザー修正中..."
sed -i 's/std::cerr << "\([🧹📊🚀⚡🎯🔍💡⭐🎉💬🔥📝⚙️🛠️📂🏆⚠️❌✨🎨🔄🔒🎲🌀🎊🌟💫⭐🌈🚀✈️🛸🎪🎭🎨🎬🎤🎧🎵🎶🎸🥇🏅🏆📈📊📉📋📌📍📎🔖🏷️📬📭📮📧📨📩📤📥📦📫📪🗳️✉️📄📃📑📊📈📉🗒️🗓️📅📆📇📋📌📍📎🔗📎🖇️📐📏📌📍📊📈📉🏁🚩🎌🏴🏳️][^"]*\)".*$/        if (!g_quiet_mode) {\n            std::cerr << "\1"/' src/analyzers/typescript/typescript_pegtl_analyzer.hpp

# JavaScriptアナライザー修正
echo "📝 JavaScriptアナライザー修正中..."
sed -i 's/std::cerr << "\([🧹📊🚀⚡🎯🔍💡⭐🎉💬🔥📝⚙️🛠️📂🏆⚠️❌✨🎨🔄🔒🎲🌀🎊🌟💫⭐🌈🚀✈️🛸🎪🎭🎨🎬🎤🎧🎵🎶🎸🥇🏅🏆📈📊📉📋📌📍📎🔖🏷️📬📭📮📧📨📩📤📥📦📫📪🗳️✉️📄📃📑📊📈📉🗒️🗓️📅📆📇📋📌📍📎🔗📎🖇️📐📏📌📍📊📈📉🏁🚩🎌🏴🏳️][^"]*\)".*$/        if (!g_quiet_mode) {\n            std::cerr << "\1"/' src/analyzers/javascript/javascript_pegtl_analyzer.hpp

echo "✅ 修正完了！"