@echo off
setlocal
cd /d "%~dp0"
echo ====================================================
echo   PULSO-IoT - Publicacion inicial en GitHub
echo ====================================================
echo.
where git >nul 2>nul
if errorlevel 1 (
  echo ERROR: Git no esta instalado o no esta en PATH.
  echo Instala Git for Windows desde https://git-scm.com/download/win y vuelve a ejecutar.
  pause
  exit /b 1
)
set /p REPO_URL= Pega la URL HTTPS del repositorio vacio de GitHub: 
if "%REPO_URL%"=="" (
  echo ERROR: No se proporciono URL.
  pause
  exit /b 1
)
set /p GIT_NAME= Nombre para los commits: 
set /p GIT_EMAIL= Correo asociado a GitHub (puedes usar tu correo noreply): 
if not "%GIT_NAME%"=="" git config user.name "%GIT_NAME%"
if not "%GIT_EMAIL%"=="" git config user.email "%GIT_EMAIL%"
if not exist ".git" git init
 git branch -M main
 git add .
git status --short
echo.
echo Se creara el commit inicial con los archivos de esta carpeta.
git commit -m "Initial commit: PULSO-IoT CDIO project"
if errorlevel 1 (
  echo.
  echo El commit no se pudo crear. Revisa identidad Git o cambios pendientes.
  pause
  exit /b 1
)
git remote get-url origin >nul 2>nul
if errorlevel 1 (
  git remote add origin "%REPO_URL%"
) else (
  git remote set-url origin "%REPO_URL%"
)
git push -u origin main
if errorlevel 1 (
  echo.
  echo No se pudo completar el push. Revisa acceso/autenticacion y la URL.
  pause
  exit /b 1
)
echo.
echo Publicacion inicial completada.
pause
