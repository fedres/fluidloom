lexer grammar FluidLoomLatticesLexer;

EXPORT     : 'export';
LATTICE    : 'lattice';
FLOAT_T    : 'float';
INT_T      : 'int';

// Property names
NAME       : 'name';
STENCIL    : 'stencil';
WEIGHTS    : 'weights';
CS2        : 'cs2';
NUM_POP    : 'num_populations';

// Lattice syntax
VEC_START  : '[';
VEC_END    : ']';
TUPLE_START: '(';
TUPLE_END  : ')';
COMMA      : ',';
SEMI       : ';';
EQ         : '=';
LBRACE     : '{';
RBRACE     : '}';
COLON      : ':';

// Numbers
INTEGER    : [0-9]+;
FLOAT_NUM  : [0-9]+ '.' [0-9]* ([eE] [+-]? [0-9]+)? 
           | '.' [0-9]+ ([eE] [+-]? [0-9]+)?;
FRACTION   : INTEGER '/' INTEGER;

// Identifiers
IDENTIFIER : [a-zA-Z_][a-zA-Z0-9_]*;
STRING     : '"' (~["\r\n] | '\\' .)* '"';

// Skip
WS         : [ \t\r\n]+ -> skip;
COMMENT    : '#' ~[\r\n]* -> skip;
