for %%i in (*) do (
	if not %%i == rebuild.bat (
		if not %%i == build_script.bat (
			del %%i
		)
	)
)
for /D %%f in (*) do (
	rd /S /Q %%f
)
cmake .. -G "MinGW Makefiles"
mingw32-make
mingw32-make install