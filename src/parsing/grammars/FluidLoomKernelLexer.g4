lexer grammar FluidLoomKernelLexer;

import FluidLoomExpressionLexer;

// Kernel-specific keywords
KERNEL: 'kernel';
READS: 'reads';
WRITES: 'writes';
HALO: 'halo';
EXECUTION_MASK: 'execution_mask';
INLINE: 'inline';
SCRIPT: 'script';
OPPOSITE: 'opposite';
REDUCE_SUM: 'reduce_sum';
REDUCE_MIN: 'reduce_min';
REDUCE_MAX: 'reduce_max';
RUN: 'run';
NO_FUSE: 'no_fuse';
PLACE_GEOMETRY: 'place_geometry';
GET_NEIGHBOR: 'get_neighbor_index';

// Script block delimiters
PIPE: '|';
