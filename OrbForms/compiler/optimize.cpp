#include "compiler.h"
/* out of date
char* opNames[] = {
    "Add", "Sub", "Mult", "Div", "Mod",
    "Load", "Assign", "GetProp", "SetProp",
    "BOr", "BAnd", "XOr", "BNot", "SL", "SR",
    "Or", "And",
    "LT", "GT", "LTE", "GTE", "Eq", "NEq",
    "Not", "Neg", "IncB", "IncA", "DecB", "DecA",
    "Call", "FuncA", "Swap", "StackDup", "Discard", "Alloc",
    "CInt", "CChar", "CString", "CFloat", "CWord", "CWordPFP",
    "SCInt", "SCChar", "SCString", "SCFloat", "SCWord",
    "StackInit",
    "ItoC", "ItoF", "ItoS", "FtoI", "FtoC", "FtoS",
    "CtoI", "CtoF", "CtoS", "StoI", "StoF", "StoC",
    "StoB",
    "GetAt", "SetAt", "New", "Delete",
    "NoOp", "NoOp2",
    "LoadI", "StoreI", "LoadF0", "StoreF0", "LoadF1", "StoreF1",
    "AddF0", "AddF1",
};
*/

bool Compiler::Optimize() {
    optStmts(funcInitTree.tree);
    optStmts(funcTree.tree);
    optStmts(funcDestTree.tree);
    return true;
}

void Compiler::optStmts(StmtNode*& s) {
    if (!s) return;
    
    optStmt(s);
    StmtNode* iter = s;
    while (iter->next) {
        optStmt(iter->next);
        iter = iter->next;
    }
}

void Compiler::optStmt(StmtNode*& s) {
    if (!s) return;

    optStmts(s->kids[0]);
    optStmts(s->kids[1]);
    optExprs(s->expr[0]);
    optExprs(s->expr[1]);
    optExprs(s->expr[2]);

    // do stuff

}

void Compiler::optExprs(ExprNode*& e) {
    if (!e) return;

    optExpr(e);
    ExprNode* iter = e;
    while (iter->next) {
        optExpr(iter->next);
        iter = iter->next;
    }
}

void Compiler::optExpr(ExprNode*& e) {
    if (!e) return;

    // do stuff
    optMinimizeConsts(e);
    optConsts(e);
    optAddrModes(e);
}

void Compiler::optMinimizeConsts(ExprNode*& e) {
    if (!e) return;

    optMinimizeConst(e);
    ExprNode* iter = e;
    while (iter->next) {
        optMinimizeConst(iter->next);
        iter = iter->next;
    }
    return;
}

void Compiler::optMinimizeConst(ExprNode*& e) {
    // evaluate constants and conversions
    if (!e) return;

    optMinimizeConsts(e->kids[0]);
    optMinimizeConsts(e->kids[1]);
    optMinimizeConsts(e->kids[2]);

    // do opt
    ExprNode *l = e->kids[0], *r = e->kids[1], *r2 = e->kids[2], *n = NULL;

    // data minimization
    if (e->op == opCInt) {
        if (e->val.iVal >= 0 && e->val.iVal < 0x8000) // to avoid address confusion
            e->op = opCWord;
    }
    if (e->op == opSCInt) {
        if (e->val.iVal >= 0 && e->val.iVal < 0x8000) // to avoid address confusion
            e->op = opSCWord;
    }
}

void Compiler::optConsts(ExprNode*& e) {
    if (!e) return;

    optConst(e);
    ExprNode* iter = e;
    while (iter->next) {
        optConst(iter->next);
        iter = iter->next;
    }
}

char space_buff[256];
char* spaces(int c) {
    int i;
    for (i=0;i<c;i++)
        space_buff[i] = ' ';
    space_buff[i] = '\0';
    return space_buff;
}

static void removeParent(ExprNode*& parent, ExprNode*& child, bool copyType = true) {
    if (copyType) child->type = parent->type;
    parent->kids[0] = NULL;
    assert(child->next == NULL);
    child->next = parent->next;
    parent->next = NULL;
    delete parent;
    parent = child;
}

static bool isVariant(ExprNode* e) {
    if (e->type.indir) return false;
    return (e->type.vtype == vtVariant);
}

void Compiler::optConst(ExprNode*& e) {
    // evaluate constants and conversions
    if (!e) return;
    static int s_depth = -2;
    s_depth += 2;

    //printf("%s%s type:%d\n", spaces(s_depth), opNames[e->op], e->type.vtype);

    optConsts(e->kids[0]);
    optConsts(e->kids[1]);
    optConsts(e->kids[2]);

    s_depth -= 2;

    ExprNode *l = e->kids[0], *r = e->kids[1], *r2 = e->kids[2], *n = NULL;

    if (e->op == opAdd || e->op == opSub) {
        if (r->op == opCWord && r->val.iVal == 0 && !isVariant(e)) { // variant string + 0 shouldn't be removed
            removeParent(e, l, false);
            return;
        }
    }
    if (e->op == opMult || e->op == opDiv) {
        if (r->op == opCWord && r->val.iVal == 1) {
            removeParent(e, l, false);
            return;
        }
    }
    
    // binary constant expressions
    if (l && r && l->op == opCWord && r->op == opCWord) {
        int val = 0;
        bool didSomething = true;
        switch (e->op) {
            case opAdd: val = l->val.iVal + r->val.iVal; break;
            case opSub: val = l->val.iVal - r->val.iVal; break;
            case opMult: val = l->val.iVal * r->val.iVal; break;
            default: didSomething = false;
        }
        if (didSomething && val >= 0 && val <= 0x8000) {
            n = new ExprNode(opCWord);
            n->type.vtype = vtInt;
            n->val.type = vtInt;
            n->val.iVal = val;
            n->next = e->next;
            e->next = NULL;
            delete e;
            e = n;
            return;
        }
    }

    // local structure access
    if (e->op == opAdd && l->op == opCWordPFP && r->op == opCWord) {
        int val = 0;
        val = l->val.iVal + r->val.iVal;
        if (val < 0x4000) {
            n = l;
            l->val.iVal = val;
            e->kids[0] = NULL;
            l->next = e->next;
            e->next = NULL;
            delete e;
            e = l;
            return;
        }
    }


    // unary constant expressions
    if (e->op == opNeg && l->op == opCWord) {
        n = l;
        l->val.iVal = -l->val.iVal;
        l->op = opCInt;
        e->kids[0] = NULL;
        l->next = e->next;
        e->next = NULL;
        delete e;
        e = l;
        return;
    }
}

void Compiler::optAddrModes(ExprNode*& e) {
    if (!e) return;

    optAddrMode(e);
    ExprNode* iter = e;
    while (iter->next) {
        optAddrMode(iter->next);
        iter = iter->next;
    }
}

void Compiler::optAddrMode(ExprNode*& e) {
    // convert Loads and Stores into more efficient forms
    if (!e) return;

    optAddrModes(e->kids[0]);
    optAddrModes(e->kids[1]);
    optAddrModes(e->kids[2]);

doitagain:
    // do opt
    ExprNode *l = e->kids[0], *r = e->kids[1], *r2 = e->kids[2], *n = NULL;

    if (e->op == opLoad) {
        if (l->op == opCWord || l->op == opCWordPFP) {
            l->val.iVal = l->op == opCWord ? l->val.iVal : l->val.iVal + 0xc000;
            removeParent(e, l);
            e->op = opLoadI;
        } else if (l->op == opLoadI && l->val.iVal == 0xc000) {
            removeParent(e, l);
            e->op = opLoadF0;
            e->val.iVal = 0;
        } else if (l->op == opLoadI && l->val.iVal == 0xc001) {
            removeParent(e, l);
            e->op = opLoadF1;
            e->val.iVal = 0;
        } else if (l->op == opAddF0) {
            removeParent(e, l);
            e->op = opLoadF0;
        } else if (l->op == opAddF1) {
            removeParent(e, l);
            e->op = opLoadF1;
        }
    } else if (e->op == opAdd) {
        if (l->op == opLoadI && (l->val.iVal == 0xc000 || l->val.iVal == 0xc001) && r->op == opCWord) {
            e->op = l->val.iVal == 0xc000 ? opAddF0 : opAddF1;
            e->val.iVal = r->val.iVal;
            e->kids[0] = NULL;
            e->kids[1] = NULL;
            delete l;
            delete r;
        }
    } else if (e->op == opAssign && !(e->type.vtype == vtStruct && e->type.indir == 0)) {
        if (l->op == opCWord || l->op == opCWordPFP) {
            assert(!isVariant(e));
            e->op = opStoreI;
            e->val.iVal = l->op == opCWord ? l->val.iVal : l->val.iVal + 0xc000;
            delete l;
            e->kids[0] = r;
            e->kids[1] = NULL;
            goto doitagain;
        } else if ((l->op == opAddF0 || l->op == opAddF1) && !isVariant(e)) {
            e->op = l->op == opAddF0 ? opStoreF0 : opStoreF1;
            e->val.iVal = l->val.iVal;
            delete l;
            e->kids[0] = r;
            e->kids[1] = NULL;
            goto doitagain;
        } else if (l->op == opLoadI && (l->val.iVal == 0xc000 || l->val.iVal == 0xc001) && !isVariant(e)) {
            e->op = l->val.iVal == 0xc000 ? opStoreF0 : opStoreF1;
            e->val.iVal = 0;
            delete l;
            e->kids[0] = r;
            e->kids[1] = NULL;
            goto doitagain;
        }
    } else if (e->op == opDiscard && l->op == opStoreI && l->kids[0]->op >= opCInt && l->kids[0]->op <= opCWord) {
        // a = 5
        removeParent(e, l);
        l = e->kids[0];
        e->data = e->val.iVal;
        e->val.iVal = l->val.iVal;
        e->op = (OpType)(l->op + opSCInt - opCInt);
        e->kids[0] = NULL;
        delete l;
    } else if (e->op == opDiscard && l->op == opStoreI && l->kids[0]->op == opLoadI) {
        // a = b
        removeParent(e, l);
        l = e->kids[0];
        e->data = e->val.iVal;
        e->val.iVal = l->val.iVal;
        e->op = opMove;
        e->kids[0] = NULL;
        delete l;
    } else if (e->op == opSStore && l->kids[0] && l->kids[0]->op == opLoad &&
            l->kids[0]->kids[0]->op == opStackDup && l->kids[0]->kids[0]->kids[0]) {

        ExprNode* addr = l->kids[0]->kids[0]->kids[0];
        if (addr->op == opCWord || addr->op == opCWordPFP) {
            // a += 2
            ExprNode* store = new ExprNode(opStoreI);
            store->val.iVal = addr->op == opCWord ? addr->val.iVal : addr->val.iVal + 0xc000;
            store->kids[0] = l;
            store->next = e->next;
            store->type = e->type;
            ExprNode* load = new ExprNode(opLoadI);
            load->val.iVal = store->val.iVal;
            delete l->kids[0]; // old opLoad
            l->kids[0] = load;
            e->kids[0] = NULL;
            e->next = NULL;
            delete e;
            e = store;

        } else if (addr->op == opAddF0 || addr->op == opAddF0) {
            // this.b += 2
            ExprNode* store = new ExprNode(addr->op == opAddF0 ? opStoreF0 : opStoreF1);
            store->val.iVal = addr->val.iVal;
            store->kids[0] = l;
            store->next = e->next;
            ExprNode* load = new ExprNode(addr->op == opAddF0 ? opLoadF0 : opLoadF1);
            load->val.iVal = addr->val.iVal;
            delete l->kids[0]; // old opLoad
            l->kids[0] = load;
            e->kids[0] = NULL;
            e->next = NULL;
            delete e;
            e = store;

        } else if (addr->op == opLoadI && (addr->val.iVal == 0xc000 || addr->val.iVal == 0xc001)) {
            // this.a += 2
            ExprNode* store = new ExprNode(addr->val.iVal == 0xc000 ? opStoreF0 : opStoreF1);
            store->val.iVal = 0;
            store->kids[0] = l;
            store->next = e->next;
            ExprNode* load = new ExprNode(addr->val.iVal == 0xc000 ? opLoadF0 : opLoadF1);
            load->val.iVal = 0;
            delete l->kids[0]; // old opLoad
            l->kids[0] = load;
            e->kids[0] = NULL;
            e->next = NULL;
            delete e;
            e = store;
        }
        // otherwise just generate the address and stackdup it
    }
}

#ifdef OLD_OPTIMIZER

bool Compiler::optConst(ExprNode*& e) {
    // evaluate constants and conversions
    if (!e) return;

    optConst(e->kids[0]);
    optConst(e->kids[1]);
    optConst(e->kids[2]);

    ExprNode *l = e->kids[0], *r = e->kids[1], *r2 = e->kids[2], *n = NULL;

    // data minimization
    if (e->op == opCInt) {
        if (e->val.iVal >= 0 && e->val.iVal < 0x8000) // to avoid address confusion
            e->op = opCWord;
    }
    if (e->op == opCSInt) {
        if (e->val.iVal >= 0 && e->val.iVal < 0x8000) // to avoid address confusion
            e->op = opCSWord;
    }
    
    // identity ops
    if (e->op == opAdd || e->op == opSub) {
        if (r->op == opCWord && r->val.iVal == 0) {
            e->kids[0] = NULL;
            l->next = e->next;
            e->next = NULL;
            delete e;
            e = l;
        }
    }
    if (e->op == opMult || e->op == opDiv) {
        if (r->op == opCWord && r->val.iVal == 1) {
            e->kids[0] = NULL;
            l->next = e->next;
            e->next = NULL;
            delete e;
            e = l;
        }
    }
    
    // binary constant expressions
    if (l && r && l->op == opCWord && r->op == opCWord) {
        int val = 0;
        bool didSomething = true;
        switch (e->op) {
            case opAdd: val = l->val.iVal + r->val.iVal; break;
            case opSub: val = l->val.iVal - r->val.iVal; break;
            case opMult: val = l->val.iVal * r->val.iVal; break;
            default: didSomething = false;
        }
        if (didSomething && val >= 0 && val <= 0x8000) {
            n = new ExprNode(opCWord);
            n->val.type = vtInt;
            n->type.vtype = vtInt;
            n->val.iVal = val;
            n->next = e->next;
            e->next = NULL;
            delete e;
            e = n;
        }
    }

    // local structure access
    if (e->op == opAdd && l->op == opCWordPFP && r->op == opCWord) {
        int val = 0;
        val = l->val.iVal + r->val.iVal;
        if (val < 0x4000) {
            n = l;
            l->val.iVal = val;
            e->kids[0] = NULL;
            l->next = e->next;
            e->next = NULL;
            delete e;
            e = l;
        }
    }

    // unary constant expressions

    // constant conversions (e.g. const int to char)

}
#endif
