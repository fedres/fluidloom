parser grammar FluidLoomExpressionParser;

options { tokenVocab=FluidLoomExpressionLexer; }

// Entry point for expressions
expression
    : logicalOrExpression
    ;

logicalOrExpression
    : logicalAndExpression (OR logicalAndExpression)*
    ;

logicalAndExpression
    : equalityExpression (AND equalityExpression)*
    ;

equalityExpression
    : relationalExpression ((EQ | NE) relationalExpression)*
    ;

relationalExpression
    : additiveExpression ((LT | LE | GT | GE) additiveExpression)*
    ;

additiveExpression
    : multiplicativeExpression ((PLUS | MINUS) multiplicativeExpression)*
    ;

multiplicativeExpression
    : powerExpression ((STAR | SLASH | PERCENT) powerExpression)*
    ;

powerExpression
    : unaryExpression (CARET unaryExpression)?
    ;

unaryExpression
    : (PLUS | MINUS | NOT) unaryExpression
    | postfixExpression
    ;

postfixExpression
    : primaryExpression (DOT IDENTIFIER | LBRACK expression RBRACK | LPAREN argumentList? RPAREN)*
    ;

primaryExpression
    : IDENTIFIER
    | literal
    | TIMESTEP
    | NUM_CELLS
    | SIN | COS | TAN | ASIN | ACOS | ATAN | ATAN2
    | SQRT | POW | EXP | LOG | LOG10
    | ABS | FLOOR | CEIL | MIN | MAX
    | LPAREN expression RPAREN
    | lambdaExpression
    ;

literal
    : FLOAT
    | INTEGER
    | STRING
    | TRUE
    | FALSE
    | vectorLiteral
    ;

vectorLiteral
    : LPAREN expression (COMMA expression)* RPAREN
    ;

argumentList
    : argument (COMMA argument)*
    ;

argument
    : (IDENTIFIER ASSIGN)? expression
    ;

lambdaExpression
    : LAMBDA parameterList? COLON expression
    ;

parameterList
    : IDENTIFIER (COMMA IDENTIFIER)*
    ;
