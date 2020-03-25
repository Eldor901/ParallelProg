@echo off

(echo. @echo off

::echo BlurbmpFile.exe nature.bmp blurNature.bmp 1 1 
 for /L %%n in (1,1,16) do BlurbmpFile.exe nature.bmp blurNature.bmp %%n 4                                                

)>result.txt



