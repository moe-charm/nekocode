//=============================================================================
// 🎯 Command Line Arguments実装 - コマンドライン引数解析
//=============================================================================

#include "nekocode/command_line_args.hpp"

namespace nekocode {

//=============================================================================
// 🎯 Command Line Arguments実装
//=============================================================================

CommandLineArgs CommandLineArgs::parse(int argc, char* argv[]) {
    CommandLineArgs args;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            args.show_help = true;
        // 🔇 削除予定の未使用引数（後方互換性のため一時的に残す）
        // } else if (arg == "--compact") {
        //     args.compact_mode = true;  // TODO: 実装するか削除
        // } else if (arg == "--format" && i + 1 < argc) {
        //     args.output_format = argv[++i];  // TODO: 実装するか削除
        // } else if (arg == "--lang" || arg == "--language") {
        //     if (i + 1 < argc) {
        //         args.language = argv[++i];  // TODO: 実装するか削除
        //     }
        // } else if (arg == "--list-languages") {
        //     args.list_languages = true;  // MCPでは使用、CLIでは未使用
        } else if (arg == "--stats-only") {
            args.stats_only = true;
        } else if (arg == "--no-parallel") {
            args.enable_parallel = false;
        } else if (arg == "--io-threads" && i + 1 < argc) {
            args.io_threads = std::stoul(argv[++i]);
        } else if (arg == "--cpu-threads" && i + 1 < argc) {
            args.cpu_threads = std::stoul(argv[++i]);
        } else if (arg == "--performance") {
            args.show_performance = true;
        } else if (arg == "--progress") {
            args.enable_progress = true;
        } else if (arg == "--debug") {
            args.debug_mode = true;
        } else if (arg == "--quiet" || arg == "-q") {
            args.quiet_mode = true;  // 🔇 簡略化後は使用されない（デフォルトでquiet）
        } else if (arg == "--no-check") {
            args.skip_precheck = true;
        } else if (arg == "--force") {
            args.force_execution = true;
        } else if (arg == "--check-only") {
            args.check_only = true;
        } else if (args.target_path.empty()) {
            args.target_path = arg;
        }
    }
    
    return args;
}

CommandLineArgs parse_args(int argc, char* argv[]) {
    return CommandLineArgs::parse(argc, argv);
}

} // namespace nekocode