lexer grammar FluidLoomSimulationLexer;

import FluidLoomKernelLexer;

// Simulation-specific keywords
IMPORT: 'import';
FIELD: 'field';
INITIAL_CONDITION: 'initial_condition';
TIME_LOOP: 'time_loop';
FINAL_OUTPUT: 'final_output';
ADAPT_MESH: 'adapt_mesh';
REBALANCE_MESH: 'rebalance_mesh';
WRITE_VTK: 'write_vtk';
WRITE_HDF5: 'write_hdf5';
PRINT: 'print';
MAX_TIMESTEPS: 'max_timesteps';
TIMESTEP: 'timestep';

// Geometry keywords
GEOMETRY: 'geometry';
PLACE_GEOMETRY: 'place_geometry';
TYPE: 'type';
SOURCE: 'source';
MATERIAL_ID: 'material_id';
TRANSLATE: 'translate';
SCALE: 'scale';
ROTATE: 'rotate';
STL: 'stl';
IMPLICIT: 'implicit';
BOX: 'box';
SPHERE: 'sphere';
CYLINDER: 'cylinder';
RADIUS: 'radius';
HEIGHT: 'height';
WIDTH: 'width';
LENGTH: 'length';
