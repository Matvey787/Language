#include <exception>
#include <iostream>

import ast;

int main() try
{
    ast::AnyNode node1(ast::Lit<int>(1));
    ast::AnyNode node2(ast::Lit<int>(10));
    ast::AnyNode node3(ast::Lit<int>(22));
    ast::AnyNode node4(ast::Lit<int>(100));


    ast::AnyNode node5(ast::BinOp(std::move(node1), std::move(node2), ast::BinOp::binOpType::Add));
    ast::AnyNode node6(ast::BinOp(std::move(node3), std::move(node4), ast::BinOp::binOpType::Div));

    ast::AnyNode node7(ast::Assign(std::move(node5), std::move(node6)));

    ast::to_mmd(node7, "ast.mmd");
}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
}


