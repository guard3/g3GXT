# g3GXT
g3GXT is a simple gxt builder for GTAIII and GTAVC (and consequently for LCS and VCS too).

## How to build
Just run the .cmd for your Visual Studio version and let premake5 generate a solution for you.

## How to run
### For III
Use g3GXT_iii.exe, drag n' drop a source TXT file and a GXT file will be generated with the same filename as the input text file.
### For VC, LCS or VCS
Use g3GXT_vc.exe, drag n' drop a folder that contains source TXTs and a GXT file will be generated with the same name as the input folder.
## Format
```
[LABEL1]
This is a ~r~very fancy ~w~label!!! {And this is a comment}

[LABEL2]
This is also another {fancy, nah let's comment that} ~g~cute ~w~label...

{
Comments are also multiline
[LABEL3]
No text for you.
}

[LABEL4]
Don't forget that label names must be 7 characters maximum; latin lettes, number or underscores.
```

GTAIII's gxt source is one big TXT file.
GTAVC's source is split up into multiple TXTs, one for each script.

## Getting TXT sources
GTAIII's gxt source (along with an official builder called 'gxtmaker') is leaked in the mobile version (albeit modified a bit).
GTAVC/LCS/VCS gxt source can be recreated with **gxttotxt**.
Drag n' drop a GXT file with VC format, and a folder will be created with source TXTs.
