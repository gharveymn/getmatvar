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

reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && set OS=32BIT || set OS=64BIT

if %OS%==32BIT cmake .. -DCMAKE_GENERATOR_PLATFORM=x86
if %OS%==64BIT cmake .. -DCMAKE_GENERATOR_PLATFORM=x64