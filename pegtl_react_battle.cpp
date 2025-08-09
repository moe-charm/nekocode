// 🔥 PEGTL React.Component Battle Code
// 目的: クラスのstart_lineを正確に取得する

#include <tao/pegtl.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace tao::pegtl;

namespace battle {

// 基本要素
struct ws : star<space> {};
struct newline : sor<one<'\n'>, one<'\r'>> {};
struct comment_line : seq<TAO_PEGTL_STRING("//"), until<newline>> {};
struct comment_block : seq<TAO_PEGTL_STRING("/*"), until<TAO_PEGTL_STRING("*/")>> {};
struct comment : sor<comment_line, comment_block> {};
struct ignore : star<sor<space, comment>> {};

// 識別子（ドット対応版！）
struct simple_identifier : seq<
    sor<alpha, one<'_', '$'>>, 
    star<sor<alnum, one<'_', '$'>>>
> {};

// ✨ ドット記法対応識別子（React.Component対応）
struct dotted_identifier : seq<
    simple_identifier,
    star<seq<one<'.'>, simple_identifier>>
> {};

// キーワード
struct export_keyword : seq<TAO_PEGTL_STRING("export"), not_at<alnum>> {};
struct class_keyword : seq<TAO_PEGTL_STRING("class"), not_at<alnum>> {};
struct extends_keyword : seq<TAO_PEGTL_STRING("extends"), not_at<alnum>> {};

// 🎯 方法1: クラスヘッダーのみマッチ（ブロック含まない）
struct class_header_only : seq<
    opt<seq<export_keyword, plus<space>>>,
    class_keyword,
    plus<space>,
    simple_identifier,  // クラス名
    opt<seq<
        plus<space>,
        extends_keyword,
        plus<space>,
        dotted_identifier  // React.Component対応！
    >>,
    star<space>,
    one<'{'>  // 開き括弧で終了
> {};

// 🎯 方法2: クラス開始位置を別途記録
struct class_start_marker : seq<
    opt<seq<export_keyword, plus<space>>>,
    class_keyword
> {};

// 🎯 方法3: クラス名の位置で記録
struct class_name_position : seq<
    opt<seq<export_keyword, plus<space>>>,
    class_keyword,
    plus<space>,
    simple_identifier  // ここの位置を記録
> {};

// ブロックスキップ（既存と同じ）
struct block;
struct block : seq<one<'{'>, star<sor<block, not_one<'}'>>>, one<'}'>> {};

// 完全なクラス（比較用）
struct full_class : seq<
    opt<seq<export_keyword, plus<space>>>,
    class_keyword,
    plus<space>,
    simple_identifier,
    opt<seq<
        plus<space>,
        extends_keyword,
        plus<space>,
        dotted_identifier
    >>,
    star<space>,
    block
> {};

// ルート
struct root : star<sor<
    class_header_only,
    full_class,
    any
>> {};

// アクション定義
struct ClassInfo {
    std::string name;
    size_t start_line;
    size_t header_end_line;
    size_t body_end_line;
    std::string extends_class;
    bool has_export = false;
};

std::vector<ClassInfo> detected_classes;

template<typename Rule>
struct action {};

template<>
struct action<class_header_only> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, std::vector<ClassInfo>& classes) {
        auto pos = in.position();
        
        std::cout << "🎯 class_header_only matched!\n";
        std::cout << "   Position: line " << pos.line << ", column " << pos.column << "\n";
        std::cout << "   Matched text: " << in.string() << "\n";
        
        // クラス名とextends抽出
        std::string matched = in.string();
        size_t class_pos = matched.find("class");
        size_t name_start = matched.find_first_not_of(" \t", class_pos + 5);
        size_t name_end = matched.find_first_of(" \t{", name_start);
        std::string class_name = matched.substr(name_start, name_end - name_start);
        
        ClassInfo info;
        info.name = class_name;
        info.start_line = pos.line;
        info.header_end_line = pos.line;  // ヘッダーは同じ行
        info.has_export = matched.find("export") != std::string::npos;
        
        // extends解析
        size_t extends_pos = matched.find("extends");
        if (extends_pos != std::string::npos) {
            size_t extends_start = matched.find_first_not_of(" \t", extends_pos + 7);
            size_t extends_end = matched.find_first_of(" \t{", extends_start);
            info.extends_class = matched.substr(extends_start, extends_end - extends_start);
        }
        
        classes.push_back(info);
    }
};

template<>
struct action<full_class> {
    template<typename ActionInput>
    static void apply(const ActionInput& in, std::vector<ClassInfo>& classes) {
        auto pos = in.position();
        
        std::cout << "📦 full_class matched!\n";
        std::cout << "   Start position: line " << pos.line << ", column " << pos.column << "\n";
        std::cout << "   Length: " << in.size() << " bytes\n";
        
        // マッチ全体から最初の行だけ表示
        std::string matched = in.string();
        size_t first_newline = matched.find('\n');
        std::string first_line = matched.substr(0, first_newline);
        std::cout << "   First line: " << first_line << "\n";
    }
};

} // namespace battle

void test_pegtl_battle() {
    std::string test_code = R"(
export class MyClass {
    constructor() {
        this.value = 42;
    }
}

export class SecondClass extends React.Component {
    method() {
        return "hello";
    }
}

class ThirdClass extends React.PureComponent {
    render() {
        return <div>Test</div>;
    }
}
)";

    std::cout << "🔥 PEGTL React.Component Battle Start!\n";
    std::cout << "=====================================\n\n";
    
    std::cout << "📝 Test Code:\n";
    std::cout << test_code << "\n";
    std::cout << "=====================================\n\n";
    
    // Test 1: Header only matching
    {
        std::cout << "🎯 Test 1: Class Header Only Matching\n";
        std::cout << "-------------------------------------\n";
        
        std::vector<battle::ClassInfo> classes;
        memory_input input(test_code, "test");
        
        try {
            parse<battle::root, battle::action>(input, classes);
            
            std::cout << "\n📊 Results:\n";
            for (const auto& cls : classes) {
                std::cout << "  Class: " << cls.name << "\n";
                std::cout << "    Start line: " << cls.start_line << "\n";
                std::cout << "    Export: " << (cls.has_export ? "yes" : "no") << "\n";
                if (!cls.extends_class.empty()) {
                    std::cout << "    Extends: " << cls.extends_class << "\n";
                }
                std::cout << "\n";
            }
        } catch (const parse_error& e) {
            std::cerr << "Parse error: " << e.what() << "\n";
        }
    }
    
    // Test 2: Position tracking
    {
        std::cout << "\n🎯 Test 2: Position Tracking\n";
        std::cout << "-------------------------------------\n";
        
        memory_input input(test_code, "test");
        
        // マニュアルで位置を追跡
        size_t line = 1;
        size_t col = 1;
        
        for (size_t i = 0; i < test_code.size(); ++i) {
            if (test_code.substr(i, 5) == "class") {
                std::cout << "Found 'class' at line " << line << ", column " << col << "\n";
                
                // クラス名を探す
                size_t name_start = i + 5;
                while (name_start < test_code.size() && std::isspace(test_code[name_start])) {
                    name_start++;
                }
                size_t name_end = name_start;
                while (name_end < test_code.size() && 
                       (std::isalnum(test_code[name_end]) || test_code[name_end] == '_')) {
                    name_end++;
                }
                std::string class_name = test_code.substr(name_start, name_end - name_start);
                std::cout << "  Class name: " << class_name << "\n";
            }
            
            if (test_code[i] == '\n') {
                line++;
                col = 1;
            } else {
                col++;
            }
        }
    }
    
    std::cout << "\n🏆 Battle Complete!\n";
}

int main() {
    test_pegtl_battle();
    return 0;
}