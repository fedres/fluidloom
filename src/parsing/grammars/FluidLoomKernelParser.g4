parser grammar FluidLoomKernelParser;

options { tokenVocab=FluidLoomKernelLexer; }

import FluidLoomExpressionParser;

kernelFile
    : (inlineFunction | kernelDefinition)* EOF
    ;

inlineFunction
    : INLINE returnType IDENTIFIER LPAREN parameterList? RPAREN COLON functionBody
    ;

returnType
    : VOID
    | FLOAT_TYPE
    | DOUBLE
    | INT_TYPE
    | BOOL_TYPE
    | VECTOR LBRACK INTEGER RBRACK
    ;

functionBody
    : expression
    | blockStatement
    ;

kernelDefinition
    : KERNEL IDENTIFIER COLON LBRACE kernelParameters scriptBlock RBRACE
    ;

kernelParameters
    : (readsClause | writesClause | haloClause | executionMaskClause | COMMA)*
    ;

readsClause
    : READS COLON fieldList
    ;

writesClause
    : WRITES COLON fieldList
    ;

haloClause
    : HALO COLON INTEGER
    ;

executionMaskClause
    : EXECUTION_MASK COLON STRING
    ;

fieldList
    : IDENTIFIER (COMMA IDENTIFIER)*
    ;

scriptBlock
    : SCRIPT COLON PIPE scriptStatement* PIPE
    ;

scriptStatement
    : assignmentStatement
    | forStatement
    | ifStatement
    | runStatement
    | reduceStatement
    | placeGeometryStatement
    ;

assignmentStatement
    : IDENTIFIER (DOT IDENTIFIER)? (LBRACK expression RBRACK)? ASSIGN expression SEMI?
    ;

forStatement
    : FOR IDENTIFIER IN rangeExpression blockStatement
    ;

rangeExpression
    : expression RANGE expression
    ;

ifStatement
    : IF expression blockStatement (ELSE blockStatement)?
    ;

blockStatement
    : LBRACE scriptStatement* RBRACE
    | scriptStatement
    ;

runStatement
    : RUN IDENTIFIER LPAREN argumentList? RPAREN SEMI?
    ;

reduceStatement
    : (REDUCE_SUM | REDUCE_MIN | REDUCE_MAX) LPAREN expression RPAREN SEMI?
    ;

placeGeometryStatement
    : PLACE_GEOMETRY LPAREN (STRING | implicitGeometry) COMMA LBRACE transformParams RBRACE RPAREN SEMI?
    ;

implicitGeometry
    : IMPLICIT COLON lambdaExpression
    ;

transformParams
    : transformParam (COMMA transformParam)*
    ;

transformParam
    : AT COLON vectorLiteral
    | SCALE COLON vectorLiteral
    | ROTATION COLON vectorLiteral
    | SURFACE_MATERIAL COLON IDENTIFIER
    ;
