#include <iostream>
#include <filesystem>
#include <iomanip>
#include <memory>
#include "Parser.h"
#include "GrammarUtils.h"
#include "PyMacro.h"
using namespace std;


using namespace filesystem;
int testDir(GrammarState &g, const string& dir, const string &logfile, const string &failed = "failed.txt") {
	ofstream log(logfile);
	ofstream fail(failed);
	int total = 0, success = 0, skip = 0;
	//setDebug(1);
	cout << "\n=========================================================================\n";
	for (auto it = directory_iterator(path(dir)); it != directory_iterator(); ++it) {
		const directory_entry& e = *it;
		if (!e.is_regular_file())continue;
		try {
			string text = read_whole_file(e.path().string());
			/*bool unicode = false;
			for (char c : text)if (c <= 0)unicode = true;
			if (0&&unicode) {
				skip++;
				continue;
			} else*/ total++;
			Timer tm;
			tm.start();
			ParseContext pt(&g);
			auto N = parse(pt, text);
			double t = tm.stop();
			cout << setprecision(3);
			log << e.path().filename() << ":\t Success :\t " << N.root->size << " nodes\t time = " << t << "\t(" << text.size() / t * 1e-6 << " MB/s)\n";
			cout << e.path().filename() << ":\t Success :\t " << N.root->size << " nodes\t time = " << t << "\t(" << text.size() / t * 1e-6 << " MB/s)\n";
			success++;
		} catch (SyntaxError & ex) {
			log << e.path().filename() << ":\t Failed  :\t" << ex.what()<<"\n";
			cout << e.path().filename() << ":\t Failed  :\t" << ex.what() << "\n";
			fail << e.path().filename() << ":\t" << ex.what() << "\n" << ex.stack_info << "\n";
			fail.flush();
		} catch (Exception & ex) {
			log << e.path().filename() << ":\t Failed  :\t" << ex.what() << "\n";
			cout << e.path().filename() << ":\t Failed  :\t" << ex.what() << "\n";
			fail << e.path().filename() << ":\t" << ex.what() << "\n";
			fail.flush();
		}
		log.flush();
	}
	cout << "=========================================================================\n\n";
	if (skip) cout << skip << " contain unicode or zero symbols and skipped\n";
	cout << success << " / " << total << " (" << success * 100. / total << "%) passed\n" << (total - success) << " failed\n\n";
	return 0;
}


int main(int argc, char*argv[]) {
    string res;

    /*Timer tm("Creating context");
    tm.start();
    for(int i=0; i<100; i++){
        auto p = new_python_context(1);
        del_python_context(p);
    }
    tm.stop_pr();
    return 0;*/

    try {
        Timer tm("Parsing + printing");
        tm.start();
        string text = loadfile("../test/macros/match.pyg");
        auto p = create_python_context(1, "");
        ParserState* pst = (ParserState*)new_parser_state(p, text.c_str(), "");
		//setDebug(DBG_SHIFT | DBG_REDUCE | DBG_RULES | DBG_TOKEN | DBG_LOOKAHEAD | DBG_QQ);
        while(auto n = pst->parse_next().root.get()){
            res += ast_to_text(p, n);
        }
        del_parser_state(pst);
        del_python_context(p);
        tm.stop_pr();
    } catch (SyntaxError &ex) {
        cout << "Error: " << ex.what() << "\n" << ex.stack_info << "\n";
    } catch (Exception & e) {
        cout << "Exception: " << e.what() << "\n";
    }
    //cout<<"=================== RESULT ============================\n";
    cout<<res.size()<<endl;
    //cout<<"=======================================================\n";
    //
    //cout << sizeof(string) << endl;
    return 0;
#ifndef _DEBUG
	try {
#endif
		string text, dir;
		//setDebug(true);
		if (argc>1 && (argv[1] == "-d"s || argv[1] == "--dir"s)) {
			if (argc < 3)throw Exception("dir argument expected");
			dir = argv[2];
			argv += 2;
			argc -= 2;
		}
		for (int i = 1; i < argc; i++) {
			text += loadfile(argv[i]);
		}
		shared_ptr<GrammarState> tg;
		if (!dir.empty()) {
			tg = std::make_shared<GrammarState>();
			//addRule(st, "text -> new_syntax");
		}
		GrammarState st;
		init_base_grammar(st, tg.get());
		cout << "Const rules:\n";
		st.print_rules(cout);
		cout << "\n";
		if (!text.empty()) {
			Timer tm("Parsing");
			tm.start();
			ParseContext pt(&st);
			ParseTree res = parse(pt, text);
			tm.stop_pr();
			cout << "Parser finished successfully\n";
			Timer tm1("Printing");
			tm1.start();
			tree2file("parse_out.txt", res, &st);
			tm1.stop_pr();
		}
		if (!dir.empty()) {
			return testDir(*tg, dir, "log.txt");
		}
		//st.print_rules(cout);
#ifndef _DEBUG
	} catch (SyntaxError &ex) {
		cout << "Error: " << ex.what() << "\n" << ex.stack_info << "\n";
	} catch (Exception & e) {
		cout << "Exception: " << e.what() << "\n";
	}
#endif
	//system("pause");
	return 0;
}
