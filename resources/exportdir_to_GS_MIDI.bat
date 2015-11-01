@echo off
for %%f in (%1\*.smd) do ( 
	ppmd_audioutil.exe -cvinfo "pmd2eos_cvinfo.xml" -fl 2 -log "%%f.txt" "%%f"
)