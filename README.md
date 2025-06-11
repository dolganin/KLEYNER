# Cleaner Utility

Cross platform command line tool for removing temporary files.

## Build

```bash
# Linux/WSL
./scripts/build_nix.sh
# Windows (MSVC or MinGW)
./scripts/build_win.bat
```

The scripts create a statically linked binary in `bin/`.

## Usage

```
cleaner --help
```

Options:
- `--dry-run`  show what would be deleted
- `--json`     machine readable output for the size report
- `--exclude p1,p2` exclude paths from scanning

Run with administrator/root privileges; the program will try to elevate
permissions automatically.
