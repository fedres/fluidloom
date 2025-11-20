parser grammar FluidLoomSimulationParser;

options { tokenVocab=FluidLoomSimulationLexer; }

import FluidLoomKernelParser;

simulationFile
    : importSection* fieldDeclaration* kernelDefinition* codeBlock+ EOF
    ;

importSection
    : IMPORT STRING SEMI?
    ;

fieldDeclaration
    : FIELD IDENTIFIER COLON fieldType SEMI?
    ;

fieldType
    : IDENTIFIER
    | INT_TYPE
    | FLOAT_TYPE
    | DOUBLE_TYPE
    | BOOL_TYPE
    | VECTOR
    | SCALAR
    ;

codeBlock
    : INITIAL_CONDITION COLON LBRACE scriptStatement* RBRACE
    | TIME_LOOP COLON LBRACE scriptStatement* RBRACE
    | FINAL_OUTPUT COLON LBRACE scriptStatement* RBRACE
    ;

// Extends scriptStatement from kernel grammar
scriptStatement
    : runStatement
    | reduceStatement
    | placeGeometryStatement
    | adaptMeshStatement
    | rebalanceMeshStatement
    | writeStatement
    | printStatement
    | forStatement
    | whileStatement
    | ifStatement
    | assignmentStatement
    ;

whileStatement
    : WHILE expression blockStatement
    ;

adaptMeshStatement
    : ADAPT_MESH LPAREN RPAREN SEMI?
    ;

rebalanceMeshStatement
    : REBALANCE_MESH LPAREN RPAREN SEMI?
    ;

writeStatement
    : (WRITE_VTK | WRITE_HDF5) LPAREN fieldList (COMMA expression)? RPAREN SEMI?
    ;

printStatement
    : PRINT LPAREN expression RPAREN SEMI?
    ;
