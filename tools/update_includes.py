import os
import re

PROJECT_ROOT = "/Users/karthikt/Ideas/FluidLoom/fluidloom"
INCLUDE_ROOT = os.path.join(PROJECT_ROOT, "include")
SRC_ROOT = os.path.join(PROJECT_ROOT, "src")

def resolve_path(current_file_path, include_path):
    """
    Resolves the include path relative to the current file.
    Returns the absolute path if it exists in include/fluidloom, else None.
    """
    current_dir = os.path.dirname(current_file_path)
    resolved_abs_path = os.path.normpath(os.path.join(current_dir, include_path))
    
    # Check if this resolved path corresponds to a file in include/fluidloom
    # We need to map the resolved path to the include structure.
    # If the file was in src, we map src/X to include/fluidloom/X.
    # If the file was in include, it is already in include.
    
    rel_path_in_include = None
    
    if resolved_abs_path.startswith(INCLUDE_ROOT):
        rel_path_in_include = os.path.relpath(resolved_abs_path, INCLUDE_ROOT)
    elif resolved_abs_path.startswith(SRC_ROOT):
        rel_path_from_src = os.path.relpath(resolved_abs_path, SRC_ROOT)
        rel_path_in_include = os.path.join("fluidloom", rel_path_from_src)
    else:
        # Maybe it's a relative path that goes out of src/include?
        # E.g. if current is src/foo.cpp and include is "bar.h", resolved is src/bar.h
        # If src/bar.h doesn't exist (because we moved it), we should check if include/fluidloom/bar.h exists.
        pass

    # The file at resolved_abs_path might NOT exist anymore because we moved it.
    # So we should construct the EXPECTED path in include/fluidloom.
    
    # If current file is in src/A/B.cpp and includes "C.h", resolved is src/A/C.h.
    # We expect this to be at include/fluidloom/A/C.h.
    
    # If current file is in include/fluidloom/A/B.h and includes "C.h", resolved is include/fluidloom/A/C.h.
    # We expect this to be at include/fluidloom/A/C.h.
    
    candidate_include_path = ""
    if current_file_path.startswith(SRC_ROOT):
        # current: src/A/B.cpp
        # include: "C.h" -> src/A/C.h
        # target: include/fluidloom/A/C.h
        
        # include: "../C.h" -> src/C.h
        # target: include/fluidloom/C.h
        
        rel_dir_from_src = os.path.relpath(current_dir, SRC_ROOT)
        # if rel_dir_from_src is ".", it's src root.
        
        target_rel_path = os.path.normpath(os.path.join(rel_dir_from_src, include_path))
        candidate_include_path = os.path.join(INCLUDE_ROOT, "fluidloom", target_rel_path)
        
    elif current_file_path.startswith(INCLUDE_ROOT):
        # current: include/fluidloom/A/B.h
        # include: "C.h" -> include/fluidloom/A/C.h
        
        rel_dir_from_include = os.path.relpath(current_dir, INCLUDE_ROOT)
        target_rel_path = os.path.normpath(os.path.join(rel_dir_from_include, include_path))
        candidate_include_path = os.path.join(INCLUDE_ROOT, target_rel_path)
    
    if os.path.exists(candidate_include_path):
        # Return the path relative to include/
        return os.path.relpath(candidate_include_path, INCLUDE_ROOT)
    
    # Fallback: Check if the include path was relative to SRC_ROOT (e.g. "common/Logger.h")
    # This handles cases where 'src' was implicitly or explicitly an include directory.
    # We check if include/fluidloom/<include_path> exists.
    
    # Handle "src/..." prefix
    clean_include_path = include_path
    if include_path.startswith("src/"):
        clean_include_path = include_path[4:] # Strip "src/"
        
    fallback_path = os.path.join(INCLUDE_ROOT, "fluidloom", clean_include_path)
    if os.path.exists(fallback_path):
        return os.path.relpath(fallback_path, INCLUDE_ROOT)

    return None

def process_file(file_path):
    with open(file_path, 'r') as f:
        content = f.read()
    
    lines = content.splitlines()
    new_lines = []
    modified = False
    
    for line in lines:
        match = re.match(r'(\s*#include\s*)"([^"]+)"(.*)', line)
        if match:
            prefix = match.group(1)
            include_path = match.group(2)
            suffix = match.group(3)
            
            # Don't touch if already fluidloom/
            if include_path.startswith("fluidloom/"):
                new_lines.append(line)
                continue
                
            resolved = resolve_path(file_path, include_path)
            if resolved:
                new_line = f'{prefix}"{resolved}"{suffix}'
                new_lines.append(new_line)
                modified = True
                # print(f"Updated {file_path}: {include_path} -> {resolved}")
            else:
                new_lines.append(line)
        else:
            new_lines.append(line)
            
    if modified:
        with open(file_path, 'w') as f:
            f.write('\n'.join(new_lines) + '\n')
        print(f"Modified: {file_path}")

def main():
    # Scan src
    for root, dirs, files in os.walk(SRC_ROOT):
        for file in files:
            if file.endswith(('.cpp', '.h', '.hpp', '.c', '.cc')):
                process_file(os.path.join(root, file))
                
    # Scan include
    for root, dirs, files in os.walk(INCLUDE_ROOT):
        for file in files:
            if file.endswith(('.h', '.hpp')):
                process_file(os.path.join(root, file))

    # Scan tests
    TESTS_ROOT = os.path.join(PROJECT_ROOT, "tests")
    for root, dirs, files in os.walk(TESTS_ROOT):
        for file in files:
            if file.endswith(('.cpp', '.h', '.hpp', '.c', '.cc')):
                process_file(os.path.join(root, file))

if __name__ == "__main__":
    main()
