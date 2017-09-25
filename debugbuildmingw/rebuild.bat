@echo off

for %%i in (*) do (
	if not %%i == rebuild.bat (
		if not %%i == build.bat (
			del %%i
		)
	)
)
for /D %%f in (*) do (
	rd /S /Q %%f
)
build