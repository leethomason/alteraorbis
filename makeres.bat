rem mkdir res
rem mkdir .\android\ufoattack_1\res\raw

copy .\visstudio\Debug\builder.exe .
builder.exe .\resin\default.xml .\res\lumos.db -d
 
rem copy .\res\uforesource.db .\android\ufoattack_1\res\raw\uforesource.png
