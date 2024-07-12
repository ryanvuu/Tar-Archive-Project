# Tar-Archive-Project
# About
The mytar program is a more minimal, but still synonymous, version of the Unix tar program.
mytar is a file archiving program that supports creating, extracting, and listing an archive.

## Usage

    mytar [ ctxvS ]f tarname [ path [ ... ] ]

in which one of the c, t, or x, options are required as well as the f option. 

## Options

    c - Create archive
    t - List archive
    x - Extract archive
    v - Enable verbosity 
        - Verbosity when creating and extracting will list out the names of each file added or extracted from the archive as it occurs; 
        - Verbosity when listing will print out extra information such as file permissions and timestamps. 
    f - Specifies archive name
    S - Enables strict interpretation of the standard



    
