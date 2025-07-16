#!/usr/bin/env python3
import os
import re
import glob

def find_member_variables():
    """Find all member variables starting with 'm' followed by uppercase letter or hungarian notation"""
    member_vars = set()
    
    # Search all header and source files in Framework directory
    patterns = [
        "Framework/**/*.h",
        "Framework/**/*.hpp", 
        "Framework/**/*.cpp",
        "Framework/**/*.inl",
    ]
    
    files = []
    for pattern in patterns:
        files.extend(glob.glob(pattern, recursive=True))
    
    print(f"Scanning {len(files)} files...")
    
    # Regex to find member variables: 
    # 1. mVariableName (standard: m + UpperCase)
    # 2. mbVariableName, mpVariableName, etc. (hungarian: m + lowercase prefix + UpperCase)
    member_var_pattern = re.compile(r'\bm(?:[A-Z]|[a-z][A-Z])[a-zA-Z0-9_]*\b')
    
    for file_path in files:
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                matches = member_var_pattern.findall(content)
                if matches:
                    print(f"Found in {file_path}: {matches}")
                member_vars.update(matches)
        except Exception as e:
            print(f"Error reading {file_path}: {e}")
    
    return sorted(member_vars)

def replace_in_file(file_path, replacements):
    """Replace member variables in a single file"""
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        original_content = content
        
        # Sort replacements by length descending to avoid partial replacements
        sorted_replacements = sorted(replacements.items(), key=lambda x: len(x[0]), reverse=True)
        
        for old_name, new_name in sorted_replacements:
            # Use word boundary to ensure we only replace complete words
            # This pattern ensures we don't replace inside strings or comments partially
            pattern = r'\b' + re.escape(old_name) + r'\b'
            content = re.sub(pattern, new_name, content)
        
        if content != original_content:
            with open(file_path, 'w', encoding='utf-8', newline='') as f:
                f.write(content)
            return True
        return False
    except Exception as e:
        print(f"Error processing {file_path}: {e}")
        return False

def main():
    print("Finding member variables...")
    member_vars = find_member_variables()
    
    print(f"Found {len(member_vars)} member variables:")
    for var in member_vars:
        print(f"  {var}")
    
    # Create mapping from mVariable to m_Variable
    replacements = {}
    for var in member_vars:
        if var.startswith('m') and len(var) > 1:
            # Handle standard naming: mVariableName -> m_VariableName
            if var[1].isupper():
                new_name = 'm_' + var[1:]
                replacements[var] = new_name
            # Handle hungarian notation: mbVariableName -> m_bVariableName
            elif len(var) > 2 and var[1].islower() and var[2].isupper():
                new_name = 'm_' + var[1:]
                replacements[var] = new_name
    
    print(f"\nRenaming {len(replacements)} variables...")
    for old, new in replacements.items():
        print(f"  {old} -> {new}")
    
    # Find all files to process
    patterns = [
        "Framework/**/*.h",
        "Framework/**/*.hpp", 
        "Framework/**/*.cpp",
        "Framework/**/*.inl"
    ]
    
    files = []
    for pattern in patterns:
        files.extend(glob.glob(pattern, recursive=True))
    
    modified_files = 0
    for file_path in files:
        if replace_in_file(file_path, replacements):
            print(f"Modified: {file_path}")
            modified_files += 1
    
    print(f"\nCompleted! Modified {modified_files} files.")

if __name__ == "__main__":
    main()
