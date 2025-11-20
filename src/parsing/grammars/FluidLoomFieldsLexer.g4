lexer grammar FluidLoomFieldsLexer;

// Keywords
FIELD      : 'field';
VIEW       : 'view';
SCALAR     : 'scalar';
VECTOR     : 'vector';
FLOAT      : 'float';
DOUBLE     : 'double';
INT        : 'int';
UINT8      : 'uint8';

// Property names
DEFAULT_VAL: 'default_value';
HALO_DEPTH : 'halo_depth';
VISUAL     : 'visualization';
SOLID_SCHEM: 'solid_scheme';
AVG_RULE   : 'averaging_rule';
IS_MASK    : 'is_mask';
EXPR       : 'expression';

// Averaging rule values
ARITHMETIC : 'arithmetic';
VOLUME_WEIGHTED : 'volume_weighted';
MIN_VALUE  : 'min_value';
MAX_VALUE  : 'max_value';

// Delimiters
LBRACE     : '{';
RBRACE     : '}';
COLON      : ':';
COMMA      : ',';
LBRACK     : '[';
RBRACK     : ']';
LPAREN     : '(';
RPAREN     : ')';
EQ         : '=';
TRUE       : 'true';
FALSE      : 'false';

// Values
INTEGER    : [0-9]+;
FLOAT_NUM  : [0-9]+ '.' [0-9]* ([eE] [+-]? [0-9]+)? 
           | '.' [0-9]+ ([eE] [+-]? [0-9]+)?;
STRING     : '"' (~["\r\n] | '\\' .)* '"';
IDENTIFIER : [a-zA-Z_][a-zA-Z0-9_]*;

// Skip whitespace
WS         : [ \t\r\n]+ -> skip;

// Comments
LINE_COMMENT : '#' ~[\r\n]* -> skip;
BLOCK_COMMENT : '/*' .*? '*/' -> skip;
