lexer grammar FluidLoomExpressionLexer;

// Literals
FLOAT: DIGIT+ '.' DIGIT+ ([eE] [-+]? DIGIT+)? | DIGIT+ [eE] [-+]? DIGIT+;
INTEGER: DIGIT+;
STRING: '"' (~["\r\n] | '\\' .)* '"';

// Operators
PLUS: '+';
MINUS: '-';
STAR: '*';
SLASH: '/';
PERCENT: '%';
CARET: '**';
LPAREN: '(';
RPAREN: ')';
LBRACK: '[';
RBRACK: ']';
LBRACE: '{';
RBRACE: '}';
COMMA: ',';
DOT: '.';
COLON: ':';
SEMI: ';';
EQ: '==';
NE: '!=';
LT: '<';
LE: '<=';
GT: '>';
GE: '>=';
AND: '&&';
OR: '||';
NOT: '!';
ASSIGN: '=';

// Keywords (shared)
TRUE: 'true';
FALSE: 'false';
LAMBDA: 'lambda';
IF: 'if';
ELSE: 'else';
FOR: 'for';
WHILE: 'while';
IN: 'in';
RANGE: '..';

// Type keywords
VOID: 'void';
FLOAT_TYPE: 'float';
DOUBLE: 'double';
INT_TYPE: 'int';
BOOL_TYPE: 'bool';
VECTOR: 'vector';
SCALAR: 'scalar';

// Geometry keywords
IMPLICIT: 'implicit';
AT: 'at';
SCALE: 'scale';
ROTATION: 'rotation';
SURFACE_MATERIAL: 'surface_material';

// Built-in variables
TIMESTEP: 'timestep';
NUM_CELLS: 'num_cells';

// Math functions (for kernels)
SIN: 'sin';
COS: 'cos';
TAN: 'tan';
ASIN: 'asin';
ACOS: 'acos';
ATAN: 'atan';
ATAN2: 'atan2';
SQRT: 'sqrt';
POW: 'pow';
EXP: 'exp';
LOG: 'log';
LOG10: 'log10';
ABS: 'abs';
FLOOR: 'floor';
CEIL: 'ceil';
MIN: 'min';
MAX: 'max';

// Identifiers
IDENTIFIER: [a-zA-Z_][a-zA-Z0-9_]*;

// Whitespace and comments
WS: [ \t\r\n]+ -> skip;
COMMENT: '#' ~[\r\n]* -> skip;
LINE_COMMENT: '//' ~[\r\n]* -> skip;
BLOCK_COMMENT: '/*' .*? '*/' -> skip;

fragment DIGIT: [0-9];
