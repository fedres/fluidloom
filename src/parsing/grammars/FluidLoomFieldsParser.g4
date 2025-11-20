parser grammar FluidLoomFieldsParser;

options { tokenVocab=FluidLoomFieldsLexer; }

// Entry point
fields_file : field_definition+ EOF ;

field_definition
    : (FIELD | VIEW) name=IDENTIFIER COLON type_spec LBRACE field_params RBRACE
    ;

type_spec
    : SCALAR data_type
    | VECTOR LBRACK num_components=INTEGER RBRACK data_type
    ;

data_type
    : FLOAT
    | DOUBLE
    | INT
    | UINT8
    ;

field_params
    : field_param (COMMA field_param)*
    ;

field_param
    : DEFAULT_VAL COLON default_value            # DefaultValueParam
    | HALO_DEPTH COLON INTEGER                   # HaloDepthParam
    | VISUAL COLON boolean                       # VisualizationParam
    | SOLID_SCHEM COLON STRING                   # SolidSchemeParam
    | AVG_RULE COLON averaging_rule              # AveragingRuleParam
    | IS_MASK COLON boolean                      # IsMaskParam
    | EXPR COLON STRING                          # ExpressionParam
    ;

averaging_rule
    : ARITHMETIC
    | VOLUME_WEIGHTED
    | MIN_VALUE
    | MAX_VALUE
    ;

default_value
    : scalar_value
    | vector_value
    ;

scalar_value
    : FLOAT_NUM
    | INTEGER
    | STRING  // For special values like "use_lattice_default"
    ;

vector_value
    : LPAREN scalar_value (COMMA scalar_value)* RPAREN
    ;

boolean
    : TRUE
    | FALSE
    ;
