parser grammar FluidLoomLatticesParser;

options { tokenVocab=FluidLoomLatticesLexer; }

lattices_file : export_definition+ EOF ;

export_definition
    : EXPORT LATTICE IDENTIFIER LBRACE export_params RBRACE
    | EXPORT FLOAT_T IDENTIFIER EQ number SEMI
    | EXPORT INT_T IDENTIFIER EQ INTEGER SEMI
    ;

export_params
    : export_param (COMMA export_param)* SEMI?
    ;

export_param
    : NAME COLON STRING
    | STENCIL COLON vector_list
    | WEIGHTS COLON number_list
    | CS2 COLON number
    | NUM_POP COLON INTEGER
    ;

vector_list
    : VEC_START vector (COMMA vector)* VEC_END
    ;

vector
    : TUPLE_START number COMMA number COMMA number TUPLE_END
    ;

number_list
    : VEC_START number (COMMA number)* VEC_END
    ;

number
    : FLOAT_NUM
    | FRACTION
    | INTEGER
    ;
