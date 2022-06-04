@echo on
set curpath=%~dp0

	git status
	git add *
	git status
	git commit -m wip
	git push
	git status

pause