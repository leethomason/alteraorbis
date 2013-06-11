copy .\visstudio\Debug\builder.exe .
mkdir .\resin\scaled
rem builder.exe .\resin\default.xml .\res\lumos.db -d
builder.exe .\resin\default.xml .\res\lumos.db
 
rem copy .\res\uforesource.db .\android\ufoattack_1\res\raw\uforesource.png
