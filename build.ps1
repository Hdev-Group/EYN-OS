# EYN-OS PowerShell Build Script
# This is a PowerShell equivalent of the Makefile

param(
    [Parameter(Position=0)]
    [string]$Target = "all"
)

# Set up toolchain variables
$COMPILER = "gcc"
$LINKER = "ld"
$ASSEMBLER = "nasm"
$EMULATOR = "qemu-system-i386"

# Set up flags
$CFLAGS = "-m32 -c -ffreestanding -w -fcommon -Oz -fno-stack-protector -I include/"
$ASFLAGS = "-f win32"
$LDFLAGS = "-m i386pe -T src/boot/link.ld"

# Object files list
$OBJS = @(
    "obj/kasm.o", "obj/kc.o", "obj/idt.o", "obj/isr.o", "obj/syscall.o",
    "obj/kb.o", "obj/string.o", "obj/system.o", "obj/util.o", "obj/shell.o",
    "obj/math.o", "obj/vga.o", "obj/fat32.o", "obj/ata.o", "obj/eynfs.o",
    "obj/rei.o", "obj/shell_commands.o", "obj/fs_commands.o", "obj/fdisk_commands.o",
    "obj/format_command.o", "obj/write_editor.o", "obj/tui.o", "obj/help_tui.o",
    "obj/assemble.o", "obj/instruction_set.o", "obj/run_command.o", "obj/history.o",
    "obj/game_engine.o", "obj/subcommands.o", "obj/predictive_memory.o",
    "obj/predictive_commands.o", "obj/zero_copy.o", "obj/zero_copy_commands.o"
)
$OUTPUT = "tmp/boot/kernel.bin"

# Function to check if a command exists
function Test-Command {
    param([string]$Command)
    try {
        Get-Command $Command -ErrorAction Stop | Out-Null
        return $true
    }
    catch {
        return $false
    }
}

# Function to create directory if it doesn't exist
function New-DirectoryIfNotExists {
    param([string]$Path)
    if (-not (Test-Path $Path)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
        Write-Host "Created directory: $Path"
    }
}

# Function to compile C files
function Compile-CFile {
    param([string]$Source, [string]$Output)
    Write-Host "Compiling $Source..."
    $result = & $COMPILER $CFLAGS.Split(' ') $Source -o $Output 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to compile $Source"
        Write-Error $result
        exit 1
    }
}

# Function to assemble ASM files
function Assemble-AsmFile {
    param([string]$Source, [string]$Output)
    Write-Host "Assembling $Source..."
    $result = & $ASSEMBLER $ASFLAGS.Split(' ') -o $Output $Source 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to assemble $Source"
        Write-Error $result
        exit 1
    }
}

# Check if required tools are available
Write-Host "Checking for required tools..."

$tools = @($COMPILER, $ASSEMBLER, $LINKER)
$missingTools = @()

foreach ($tool in $tools) {
    if (-not (Test-Command $tool)) {
        $missingTools += $tool
    }
}

if ($missingTools.Count -gt 0) {
    Write-Error "Missing required tools: $($missingTools -join ', ')"
    Write-Host "Please install:"
    Write-Host "  - MinGW-w64 (for gcc and ld)"
    Write-Host "  - NASM (for nasm)"
    exit 1
}

Write-Host "All required tools found."

# Main build functions
function Build-All {
    Write-Host "Building all object files..."
    New-DirectoryIfNotExists "obj"

    # Compile assembly files
    Assemble-AsmFile "src/boot/kernel.asm" "obj/kasm.o"
    Assemble-AsmFile "src/cpu/syscall.asm" "obj/syscall.o"

    # Compile C files
    $cFiles = @(
        @("src/entry/kernel.c", "obj/kc.o"),
        @("src/cpu/idt.c", "obj/idt.o"),
        @("src/drivers/kb.c", "obj/kb.o"),
        @("src/cpu/isr.c", "obj/isr.o"),
        @("src/utilities/shell/string.c", "obj/string.o"),
        @("src/cpu/system.c", "obj/system.o"),
        @("src/utilities/util.c", "obj/util.o"),
        @("src/utilities/shell/shell.c", "obj/shell.o"),
        @("src/utilities/basic/math.c", "obj/math.o"),
        @("src/drivers/vga.c", "obj/vga.o"),
        @("src/drivers/fat32.c", "obj/fat32.o"),
        @("src/drivers/ata.c", "obj/ata.o"),
        @("src/drivers/eynfs.c", "obj/eynfs.o"),
        @("src/drivers/rei.c", "obj/rei.o"),
        @("src/utilities/shell/shell_commands.c", "obj/shell_commands.o"),
        @("src/utilities/shell/fs_commands.c", "obj/fs_commands.o"),
        @("src/utilities/shell/fdisk_commands.c", "obj/fdisk_commands.o"),
        @("src/utilities/shell/format_command.c", "obj/format_command.o"),
        @("src/utilities/shell/write_editor.c", "obj/write_editor.o"),
        @("src/utilities/shell/run_command.c", "obj/run_command.o"),
        @("src/utilities/shell/history.c", "obj/history.o"),
        @("src/utilities/tui/tui.c", "obj/tui.o"),
        @("src/utilities/shell/help_tui.c", "obj/help_tui.o"),
        @("src/utilities/shell/assemble.c", "obj/assemble.o"),
        @("src/utilities/shell/instruction_set.c", "obj/instruction_set.o"),
        @("src/utilities/games/game_engine.c", "obj/game_engine.o"),
        @("src/utilities/shell/subcommands.c", "obj/subcommands.o"),
        @("src/utilities/predictive_memory.c", "obj/predictive_memory.o"),
        @("src/utilities/shell/predictive_commands.c", "obj/predictive_commands.o"),
        @("src/utilities/zero_copy.c", "obj/zero_copy.o"),
        @("src/utilities/shell/zero_copy_commands.c", "obj/zero_copy_commands.o")
    )

    foreach ($file in $cFiles) {
        Compile-CFile $file[0] $file[1]
    }

    # Create output directories and link
    New-DirectoryIfNotExists "tmp"
    New-DirectoryIfNotExists "tmp/boot"
    Write-Host "Linking kernel..."
    $result = & $LINKER $LDFLAGS.Split(' ') -o $OUTPUT $OBJS 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Linking failed"
        Write-Error $result
        exit 1
    }
    Write-Host "Build completed successfully."
}

function Build-Docs {
    Write-Host "Generating documentation..."
    if (Test-Command "python") {
        $result = & python "devtools/generate_command_docs.py" "src/" 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "Documentation generation failed: $result"
        }
    } else {
        Write-Warning "Python not found. Documentation generation skipped."
    }
}

function Build-EynfsImg {
    Write-Host "Building EYNFS format tool..."
    $result = & $COMPILER -o "eynfs_format.exe" "eynfs_format.c" 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to build eynfs_format tool"
        Write-Error $result
        exit 1
    }

    Write-Host "Creating EYNFS disk image..."
    if (Test-Path "eynfs.img") {
        Remove-Item "eynfs.img" -Force
    }

    # Create 10MB file
    $null = New-Item -ItemType File -Path "eynfs.img" -Force
    $file = [System.IO.File]::OpenWrite("eynfs.img")
    $file.SetLength(10485760)  # 10MB
    $file.Close()

    Write-Host "Formatting EYNFS image..."
    $result = & "./eynfs_format.exe" "eynfs.img" 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to format EYNFS image"
        Write-Error $result
        exit 1
    }

    Write-Host "Copying test directory to EYNFS..."
    if (Test-Command "python") {
        $result = & python "devtools/copy_testdir_to_eynfs.py" 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "Failed to copy test directory: $result"
        }
    } else {
        Write-Warning "Python not found. Test directory copy skipped."
    }
}

function Build-Complete {
    Write-Host "Building complete OS..."
    Build-All
    Build-EynfsImg
    Build-Docs

    Write-Host "Creating GRUB configuration..."
    New-DirectoryIfNotExists "tmp/grub_ultra_minimal/boot/grub"
    Copy-Item $OUTPUT "tmp/grub_ultra_minimal/boot/" -Force

    $grubConfig = @"
set default=0
set timeout=0
set gfxmode=text

menuentry "EYN-OS" {
    multiboot /boot/kernel.bin
    boot
}
"@
    $grubConfig | Out-File -FilePath "tmp/grub_ultra_minimal/boot/grub/grub.cfg" -Encoding ASCII

    Write-Host "Creating ISO image..."
    if (Test-Command "grub-mkrescue") {
        $result = & grub-mkrescue --modules="multiboot multiboot2 part_msdos ext2 iso9660" --locales="" --themes="" --fonts="" --compress=xz -o "EYNOS.iso" "tmp/grub_ultra_minimal/" 2>&1
        if ($LASTEXITCODE -ne 0) {
            Write-Error "Failed to create ISO"
            Write-Error $result
            exit 1
        }
        Write-Host "Ultra-minimal ISO created: EYNOS.iso"
        Get-ChildItem "EYNOS.iso" | Select-Object Name, Length
    } else {
        Write-Warning "grub-mkrescue not found. ISO creation skipped."
        Write-Host "Please install GRUB tools to create bootable ISO."
    }
}

function Clean-Build {
    Write-Host "Cleaning build artifacts..."
    $itemsToRemove = @(
        "obj/*.o",
        "tmp/boot/kernel.bin",
        "eynfs.img",
        "eynfs_format.exe",
        "EYNOS.iso",
        "userland/*.o",
        "userland/*.bin"
    )

    foreach ($pattern in $itemsToRemove) {
        if (Test-Path $pattern) {
            Remove-Item $pattern -Force -Recurse
        }
    }

    $dirsToRemove = @("tmp/grub_minimal", "tmp/grub_ultra_minimal")
    foreach ($dir in $dirsToRemove) {
        if (Test-Path $dir) {
            Remove-Item $dir -Force -Recurse
        }
    }

    Write-Host "Clean completed."
}

function Run-OS {
    Write-Host "Building and running OS..."
    Build-Complete
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed, cannot run"
        exit 1
    }

    if (-not (Test-Command $EMULATOR)) {
        Write-Error "$EMULATOR not found. Please install QEMU."
        exit 1
    }

    Write-Host "Starting QEMU..."
    & $EMULATOR -cdrom "EYNOS.iso" -hda "eynfs.img" -boot d -m 16M
}

function Test-OS {
    if (-not (Test-Command $EMULATOR)) {
        Write-Error "$EMULATOR not found. Please install QEMU."
        exit 1
    }

    Write-Host "Running OS test..."
    & $EMULATOR -cdrom "EYNOS.iso" -hda "eynfs.img" -boot d -m 64M
}

# Main execution
switch ($Target.ToLower()) {
    "all" { Build-All }
    "build" { Build-Complete }
    "clean" { Clean-Build }
    "run" { Run-OS }
    "test" { Test-OS }
    "docs" { Build-Docs }
    "eynfsimg" { Build-EynfsImg }
    default {
        Write-Error "Unknown target: $Target"
        Write-Host "Available targets: all, build, clean, run, test, docs, eynfsimg"
        exit 1
    }
}
