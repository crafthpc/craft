#include <iostream>
#include <vector>
#include <string.h>

#include "Symtab.h"
#include "Variable.h"
#include "Type.h"

using namespace Dyninst;
using namespace SymtabAPI;

Symtab *st;

extern void print_symbols( std::vector< Symbol *>& allsymbols );

int main(int argc, char *argv[]) {

	if (argc != 2) {
		printf("usage:  dumpsyms <binary>\n");
		exit(-1);
	}

	char pathname[512];
    strcpy(pathname, argv[argc-1]);

	Symtab::openFile(st, pathname);

    std::vector<Symbol *> syms;
    std::vector<Symbol *>::iterator it;
    size_t static_syms = 0;
    size_t dynamic_syms = 0;
    size_t total_syms = 0;

    st->getAllSymbols(syms);
    total_syms += syms.size();
    print_symbols(syms);
    for (it = syms.begin(); it != syms.end(); it++) {
        if ((*it)->isInDynSymtab())
            dynamic_syms++;
        if ((*it)->isInSymtab())
            static_syms++;
    }

    std::vector<Variable *> vars;
    std::vector<Variable *>::iterator iv;
    std::vector<std::string>::iterator is;
    st->getAllVariables(vars);
    for (iv = vars.begin(); iv != vars.end(); iv++) {
        Variable *v = *iv;
        std::vector<std::string> names = v->getAllPrettyNames();
        for (is = names.begin(); is != names.end(); is++) {
            printf("%s ", (*is).c_str());
        }
        printf("size=%d ", v->getSize());
        printf("offset=%p ", (void*)v->getOffset());
        Type *t = v->getType();
        if (t) {
            printf("%s ", t->getName().c_str());
        } else {
            printf("(no type info) ");
        }
        printf("\n");
    }

    printf("%lu total symbol(s) - %lu static and %lu dynamic  (%lu variables)\n", 
            total_syms, static_syms, dynamic_syms, vars.size());

	return(0);
}

