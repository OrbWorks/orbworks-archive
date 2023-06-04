#include "compiler.h"

ExprNode::ExprNode() {
    kids[0] = kids[1] = kids[2] = next = NULL;
    data = 0;
    isKnownType = false;
}

ExprNode::ExprNode(OpType e) {
    //this->ExprNode::ExprNode();
    new (this) ExprNode();
    op = e;
}

ExprNode::~ExprNode() {
    delete kids[0];
    delete kids[1];
    delete kids[2];
    delete next;
}

StmtNode::StmtNode() {
    expr[0] = expr[1] = expr[2] = NULL;
    kids[0] = kids[1] = next = NULL;
    data = 0;
    //vtype = vtVoid;
}

StmtNode::StmtNode(StmtType st) {
    //this->StmtNode::StmtNode();
    new (this) StmtNode();
    stmt = st;
}

StmtNode::~StmtNode() {
    if (kids[0])
        kids[0]->DeleteList();
    if (kids[1])
        kids[1]->DeleteList();
    delete expr[0];
    delete expr[1];
    delete expr[2];
}

void StmtNode::DeleteList() {
    StmtNode* iter = this;
    while (iter) {
        StmtNode* next = iter->next;
        delete iter;
        iter = next;
    }
}

void StmtTree::AddStmt(StmtNode* stmt) {
    if (lastNode == NULL) {
        tree = stmt;
    } else {
        lastNode->next = stmt;
    }
    while (stmt->next)
        stmt = stmt->next; // this might be a list
    lastNode = stmt;
}

void StmtTree::AddExprStmt(ExprNode* expr) {
    StmtNode* stmt = new StmtNode;
    stmt->stmt = stExpr;
    stmt->expr[0] = expr;
    AddStmt(stmt);
}

void StmtTree::PrependStmt(StmtNode* stmt) {
    StmtNode* end = stmt;
    while (end->next)
        end = end->next;
    end->next = tree;
    tree = stmt;
    if (lastNode == NULL)
        lastNode = end;
}

void StmtTree::PrependExprStmt(ExprNode* expr) {
    StmtNode* stmt = new StmtNode;
    stmt->stmt = stExpr;
    stmt->expr[0] = expr;
    PrependStmt(stmt);
}

void StmtTree::RemoveStmt(StmtNode* stmt) {
    if (stmt == NULL) return;
    StmtNode* iter = tree;

    while (iter) {
        if (iter->next == stmt) {
            iter->next = stmt->next;
            stmt->next = NULL;
            break;
        }
        iter = iter->next;
    }
    delete stmt;
}

void StmtTree::RemoveExprStmt(ExprNode* expr) {
    if (expr == NULL) return;
    StmtNode* iter = tree;

    while (iter) {
        if (iter->next && iter->next->expr[0] == expr) {
            StmtNode* temp = iter->next;
            iter->next = temp->next;
            temp->next = NULL;
            delete temp;
            return;
        }
        iter = iter->next;
    }
    // not found
    delete expr;
}

void StmtTree::Delete() {
    if (tree)
        tree->DeleteList();
    tree = lastNode = NULL;
}
