# Tar-Archive-Project

Project completed as of 2/26, backed up to GitHub as of 3/26.

# About
The mytar program is a more minimal, but still synonymous, version of the Unix tar program.
mytar is a file archiving program that supports creating, extracting, and listing an archive.

The program's usage is as follows: 

    mytar [ ctxvS ]f tarname [ path [ ... ] ]

in which one of the c, t, or x, options are required as well as the f option. 

Each option represents:

    c - Create archive
    t - List archive
    x - Extract archive
    v - Enable verbosity 
        - Verbosity when creating and extracting will list out the names of each file added or extracted from the archive as it occurs; 
        - Verbosity when listing will print out extra information such as file permissions and timestamps. 
    f - Specifies archive name
    S - Enables strict interpretation of the standard



    
